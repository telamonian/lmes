from collections import OrderedDict
from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.helper import ContainerEval, timewith
from lm_anal.src.magicDict.magicDict import MagicDict
from lm_anal.src.main import Sim, Sims

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
inputFilePath = (thisScriptDir / Path('../test/testData/biphasic_switch.lm')).resolve()
bfRootPath = (thisScriptDir / Path('../../../../../regression/genetic_toggle_switch_test_data')).resolve()
# ffluxRootPath = Path('/Users/tel/temp_data/gts_fflux_barrier_height_vs_crossingsPerPhase_sweep_long').resolve()
# ffluxRootPath = Path('../../../../../regression/biphasic_switch.lm')
ffluxRootPath = Path('/Users/tel/temp_data/gts_-_fflux_-_crossingsPerPhase_-_phaseZeroTime_-_theta_-_replicate').resolve()

class FFluxHistsVsBFHistsExample(object):
    def __init__(self, inputFilePath=inputFilePath, bfRootPath=bfRootPath, ffluxRootPath=ffluxRootPath, clearLmint=False, lmintOnly=False):
        if clearLmint:
            self.clearLmint()
        self.lmintOnly = lmintOnly
        # inputSim stuff
#         self.inputFilePath = inputFilePath
#         self.inputSim = Sim(fPath=self.inputFilePath)
        
        # bfSims stuff
        self.bfRootPath = bfRootPath
        self.bfSims = Sims(rootPath=self.bfRootPath)

        self.ffluxBFConversionDict = self.genFFluxBFConversionDict()

        # combine the histograms from runs that started in basin A with those from basin B
        for keySetA,simA in self.bfSims[('startingInBasin', 'A')].items():
            keySetB = (keySetA - {('startingInBasin', 'A')}) | {('startingInBasin', 'B')}
            simB = self.bfSims[keySetB]
            opHA = simA.oparamHists['Sum']
            opHB = simB.oparamHists['Sum']

            # the masks for this data set are the inverse of the standard I eventually decided on, so fix that
            for opH in [opHA, opHB]:
                opH.remask(np.zeros(opH.h_mask.shape, dtype=bool))
                opH.resliceH(np.s_[:101,:101])

            opHA.combine(opHB, inPlace=True)
            # for opH in [opHA, opHB]:
            #     print(opH.h.sum())

        self.bfHist = self.bfSims[(('degradation', '0.25000'), ('production', '1.00000'), ('startingInBasin', 'A'), ('name', 'biphasic_switch'))].oparamHists['Sum']

        # self.bfHist.resliceH(np.s_[:101,:101])
        
        # ffluxSims stuff
        self.ffluxRootPath = ffluxRootPath
        if self.lmintOnly:
            self.ffluxSims = Sims(rootPath=self.ffluxRootPath, fType='lmint')
        else:
            self.ffluxSims = Sims(rootPath=self.ffluxRootPath)
        for ffluxSim in self.ffluxSims.values():
            ffluxSim.ffluxHists.transformKwargs = {'tilingIDs':((1,2),3)}
#             ffluxSim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),3)}

        self.ffluxHist = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (3,)))]
        self.ffluxHist2D = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (1,2)))]
        try:
            self.ffluxHist = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (3,)))]
            self.ffluxHist2D = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (1,2)))]
        except:
            print('could not make FFluxHist objects')
            pass

        self.genKLDiv()
        self.assignBFHist()

    def assignBFHist(self):
        '''
        "assign" the proper bfHist to each of the ffluxHists
        '''
        for ffluxKey,ffluxSim in self.ffluxSims.items():
            ffluxHist2D = ffluxSim.ffluxHists[('InterfaceTilingID', 0), ('BinTilingIDs', (1,2))]
            theta = MagicDict.getElemFromKey('theta', ffluxKey)
            ffluxHist2D.bfHist = self.bfSims[self.ffluxBFConversionDict[theta]].oparamHists['Sum']

    def genKLDiv(self):
        for ffluxKey,ffluxSim in self.ffluxSims.items():
            ffluxHist2D = ffluxSim.ffluxHists[('InterfaceTilingID', 0), ('BinTilingIDs', (1,2))]
            theta = MagicDict.getElemFromKey('theta', ffluxKey)
            bfHist = self.bfSims[self.ffluxBFConversionDict[theta]].oparamHists['Sum']

            # zeroMask = np.logical_or(ffluxHist2D.h==0, bfHist.h==0)
            # bfHist.remask(zeroMask)
            # bfHist.normalize()
            # ffluxHist2D.remask(zeroMask)
            # ffluxHist2D.normalize()
            ffluxHist2D.klDiv = bfHist.getKLDivergence(ffluxHist2D, normalize='mask')

    def genKLDivTransition(self):
        for ffluxKey,ffluxSim in self.ffluxSims.items():
            ffluxHist2D = ffluxSim.ffluxHists[('InterfaceTilingID', 0), ('BinTilingIDs', (1,2))]
            theta = MagicDict.getElemFromKey('theta', ffluxKey)
            bfHist = self.bfSims[self.ffluxBFConversionDict[theta]].oparamHists['Sum']

            modFFluxHist2D = ffluxHist2D.basin_n_order_parameter_values[('basin_id', 0)].combine(ffluxHist2D.basin_n_order_parameter_values[('basin_id', 1)])
            modBFHist = bfHist.getCopy()
            
            # kl div in the region covered by forward flux
            zeroMask = np.logical_or(modFFluxHist2D.h==0, modBFHist.h==0)
            modBFHist.remask(zeroMask)
            modBFHist.normalize()
            modFFluxHist2D.remask(zeroMask)
            modFFluxHist2D.normalize()
            ffluxHist2D.klDivTransition = modBFHist.getKLDivergence(modFFluxHist2D)
            
            # kl div limited to the reasonably probable regions
            cutoff = 40
            probableMask = np.zeros(modFFluxHist2D.h.shape, dtype=int)
            for iArr in np.rollaxis(np.mgrid[:probableMask.shape[0],:probableMask.shape[1]].reshape(2,-1), 1):
                probableMask[tuple(iArr)] = iArr.sum()>=40
            probableMask = np.logical_or(probableMask, zeroMask)
            modBFHist.remask(probableMask)
            modBFHist.normalize()
            modFFluxHist2D.remask(probableMask)
            modFFluxHist2D.normalize()
            ffluxHist2D.klDivProbable = modBFHist.getKLDivergence(modFFluxHist2D)

    def clearLmint(self):
        if ffluxRootPath.is_file():
            lmintList = [ffluxRootPath.with_suffix('.lmint')]
        else:
            lmintList = ffluxRootPath.rglob('*.lmint')
            for lmintPath in lmintList:
                try:
                    lmintPath.unlink()
                except FileNotFoundError:
                    pass

    def genFFluxBFConversionDict(self):
        bfKeys = []
        for key in self.bfSims[('startingInBasin', 'A')].keys():
            bfKeys.append(sorted(tuple(key)))
        bfKeys.sort()

        ffluxThetas = ['%.1e' % num for num in np.logspace(-1, 1, 11, 10)]

        return OrderedDict(zip(ffluxThetas, bfKeys))

    def genStdKLDiv(self, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        bfHist = self.bfSims[self.ffluxBFConversionDict[theta]].oparamHists['Sum']

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(bfHist.getDownsampleFromRaw(nSample=n))

        self.klDivDict2D = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [self.bfHist])):
            bfHistWithoutSamples = bfHist.getCopy()
            bfHistWithoutSamples.h_raw-=hist.h_raw
            bfHistWithoutSamples.h_cache_dirty = True
            print(bfHistWithoutSamples.h.sum())
            print(hist.h.sum())
            self.klDivDict2D[name] = bfHistWithoutSamples.getKLDivergence(hist, normalize='mask')
        print('2D klDivDict contents:')
        print(list(self.klDivDict2D.keys()))
        print(list(self.klDivDict2D.values()))

    def genStdKLDivBetweenBasins(self, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        keySetA = self.ffluxBFConversionDict[theta]
        keySetB = (set(keySetA) - {('startingInBasin', 'A')}) | {('startingInBasin', 'B')}
        bfHistA = self.bfSims[keySetA].oparamHists['Sum']
        bfHistB = self.bfSims[keySetB].oparamHists['Sum']

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(bfHistA.getDownsampleFromRaw(nSample=n))

        self.klDivDict2D = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [self.bfHist])):
            self.klDivDict2D[name] = bfHistB.getKLDivergence(hist, normalize='mask')
        print('2D klDivDict contents:')
        print(list(self.klDivDict2D.keys()))
        print(list(self.klDivDict2D.values()))

    def genStdKLDivWithSplit(self, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        keySetA = self.ffluxBFConversionDict[theta]
        bfHist = self.bfSims[keySetA].oparamHists['Sum']

        testHist,sampleHist = bfHist.split()

        print(testHist.h.sum())
        print(sampleHist.h.sum())

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(sampleHist.getDownsampleFromRaw(nSample=n))

        self.klDivDict2D = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [sampleHist])):
            self.klDivDict2D[name] = testHist.getKLDivergence(hist, normalize='mask')
        print('2D klDivDict contents:')
        print(list(self.klDivDict2D.keys()))
        print(list(self.klDivDict2D.values()))

if __name__=='__main__':
    FFluxHistsVsBFHistsExample()