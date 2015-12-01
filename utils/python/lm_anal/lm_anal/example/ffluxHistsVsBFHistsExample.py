from collections import OrderedDict
from copy import deepcopy
from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.helper import ContainerEval, timewith
from lm_anal.src.magicDict.magicDict import MagicDict
from lm_anal.src.main import Sim, Sims
from lm_anal.src.plottable import Plottable

# fix some issues with deepcopy() caused by the monkeypatching this script does
Plottable.doNotCopyAttrs+=['hist1D']

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
inputFilePath = (thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()
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
        self.inputFilePath = inputFilePath
        self.inputSim = Sim(fPath=self.inputFilePath)
        
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

        self.assignBFHist(normalizeBF=False)
        # self.genKLDiv()


    def assignBFHist(self, normalizeBF=True):
        '''
        "assign" the proper bfHist to each of the ffluxHists
        '''
        for dim in (1,2):
            for ffluxKey in self.ffluxSims.keys():
                theta = MagicDict.getElemFromKey('theta', ffluxKey)
                bfAKey = self.ffluxBFConversionDict[theta]

                bfHist = self.getBFHistByKeyDim(bfAKey, dim)
                if normalizeBF:
                    bfHist.normalize()
                ffluxHist = self.getFFluxHistByKeyDim(ffluxKey, dim)
                ffluxHist.bfHist = bfHist

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

    def gen1DBFHists(self):
        '''
        gen the 1D equivalent for every 2D brute force hist in the dataset, then add them to the 2D hist as .hist1D
        '''
        for bfKey,bfSim in self.bfSims.items():
            bfHist2D = bfSim.oparamHists['Sum']
            bfHist1D = OParamHist()
            bfHist1D.setTilings(self.inputSim.oparams, self.inputSim.tilings, tilingIDs=[3])
            for i,edge in enumerate(bfHist1D.h_edges):
                # an off-by-one error is introduced by the tracing procedure, fix it by indexing bfHist1D with i+1
                bfHist1D.h_raw[i+1] = bfHist2D.h.trace(offset=int(edge))
            bfHist2D.hist1D = bfHist1D
        self._ran_gen1DBFHists = True

    def genKLDiv(self, dim=1, mask1DExtremes=35, normalize='mask'):
        for ffluxKey,ffluxSim in self.ffluxSims.items():
            theta = MagicDict.getElemFromKey('theta', ffluxKey)
            keySetA = self.ffluxBFConversionDict[theta]
            bfHist = self.getBFHistByKeyDim(keySetA, dim)
            ffluxHist = self.getFFluxHistByKeyDim(ffluxKey, dim)

            # zeroMask = np.logical_or(ffluxHist.h==0, bfHist.h==0)
            # bfHist.remask(zeroMask)
            # bfHist.normalize()
            # ffluxHist.remask(zeroMask)
            # ffluxHist.normalize()
            if dim==1 and mask1DExtremes:
                extremesMask = np.ones(bfHist.h.shape, dtype=bool)
                extremesMask[mask1DExtremes:-mask1DExtremes] = 0

                bfHistForCalc = deepcopy(bfHist)
                bfHistForCalc.remask(extremesMask)
                bfHistForCalc.normalize()

                ffluxHistForCalc = deepcopy(ffluxHist)
                ffluxHistForCalc.remask(extremesMask)
                ffluxHistForCalc.normalize()
            else:
                bfHistForCalc = bfHist
                ffluxHistForCalc = ffluxHist

            ffluxHist.klDiv = bfHistForCalc.getKLDivergence(ffluxHistForCalc, normalize=normalize)

    def genKLDivVariants(self):
        '''
        gen variant kl div values, such as over only the transition region
        '''
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

    def genFFluxBFConversionDict(self):
        bfKeys = []
        for key in self.bfSims[('startingInBasin', 'A')].keys():
            bfKeys.append(sorted(tuple(key)))
        bfKeys.sort()

        ffluxThetas = ['%.1e' % num for num in np.logspace(-1, 1, 11, 10)]

        return OrderedDict(zip(ffluxThetas, bfKeys))

    def genStdKLDiv(self, dim=1, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        keySetA = self.ffluxBFConversionDict[theta]
        bfHist = self.getBFHistByKeyDim(keySetA, dim)

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(bfHist.getDownsampleFromRaw(nSample=n))

        self.klDivDict = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [bfHist])):
            bfHistWithoutSamples = bfHist.getCopy()
            bfHistWithoutSamples.h_raw-=hist.h_raw
            bfHistWithoutSamples.h_cache_dirty = True
            print(bfHistWithoutSamples.h.sum())
            print(hist.h.sum())
            self.klDivDict[name] = bfHistWithoutSamples.getKLDivergence(hist, normalize='mask')
        print('klDivDict contents:')
        print(list(self.klDivDict.keys()))
        print(list(self.klDivDict.values()))

        return self.klDivDict

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

        self.klDivDict = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [self.bfHist])):
            self.klDivDict[name] = bfHistB.getKLDivergence(hist, normalize='mask')
        print('klDivDict contents:')
        print(list(self.klDivDict.keys()))
        print(list(self.klDivDict.values()))

    def genStdKLDivWithSplit(self, dim=1, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        keySetA = self.ffluxBFConversionDict[theta]
        bfHist = self.getBFHistByKeyDim(keySetA, dim)

        testHist,sampleHist = bfHist.split()
        testHist.reweight(1)
        sampleHist.reweight(1)

        print(testHist.h.sum())
        print(sampleHist.h.sum())

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(sampleHist.getDownsampleFromRaw(nSample=n))

        self.klDivDict = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [sampleHist])):
            self.klDivDict[name] = testHist.getKLDivergence(hist, normalize='mask')
        print('klDivDict contents:')
        print(list(self.klDivDict.keys()))
        print(list(self.klDivDict.values()))

    def getBFHistByKeyDim(self, key, dim):
        if dim==1:
            if not hasattr(self, '_ran_gen1DBFHists') or not self._ran_gen1DBFHists:
                self.gen1DBFHists()
            bfHist = self.bfSims[key].oparamHists['Sum'].hist1D
        elif dim==2:
            bfHist = self.bfSims[key].oparamHists['Sum']
        else:
            raise ValueError('Got %s for dim. Please choose either 1 or 2' % dim)

        return bfHist

    def getFFluxHistByKeyDim(self, key, dim):
        if dim==1:
            histKeyTup = (('InterfaceTilingID', 0), ('BinTilingIDs', (3,)))
        elif dim==2:
            histKeyTup = (('InterfaceTilingID', 0), ('BinTilingIDs', (1,2)))
        else:
            raise ValueError('Got %s for dim. Please choose either 1 or 2' % dim)

        return self.ffluxSims[key].ffluxHists[histKeyTup]

if __name__=='__main__':
    FFluxHistsVsBFHistsExample()