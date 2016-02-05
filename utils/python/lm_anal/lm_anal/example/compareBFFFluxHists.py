from collections import OrderedDict
from copy import deepcopy
from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.helper import ContainerEval, Depth, timewith
from lm_anal.src.magicDict.magicDict import MagicDict
from lm_anal.src.main import Sim, Sims
from lm_anal.src.plottable import Plottable

__all__ = ['CompareBFFFluxHists']

# fix some issues with deepcopy() caused by the monkeypatching this script does
Plottable.doNotCopyAttrs+=['hist1D']

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
inputFilePath = (thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()
bfRootPath = (thisScriptDir / Path('../../../../../regression/genetic_toggle_switch_test_data')).resolve()
# ffluxRootPath = Path('/Users/tel/temp_data/gts_fflux_barrier_height_vs_crossingsPerPhase_sweep_long').resolve()
# ffluxRootPath = Path('../../../../../regression/biphasic_switch.lm')
ffluxRootPath = Path('/Users/tel/temp_data/gts_-_fflux_-_crossingsPerPhase_-_phaseZeroTime_-_theta_-_replicate').resolve()

class CompareBFFFluxHists(object):
    def __init__(self, bfRootPath=bfRootPath, ffluxRootPath=ffluxRootPath, inputFilePath=inputFilePath, clearLmint=False, filterRules=None, lmintOnly=False):
        if clearLmint:
            self.clearLmint()
        self.lmintOnly = lmintOnly
        # inputSim stuff
        if inputFilePath is not None:
            self.inputFilePath = inputFilePath
            self.inputSim = Sim(fPath=self.inputFilePath)
        
        # bfSims stuff
        self.bfRootPath = bfRootPath
        bfSimsKwargs = {'rootPath': self.bfRootPath, 'filterRules': filterRules}
        if self.lmintOnly:
            bfSimsKwargs['fType'] = 'lmint'
        self.bfSims = Sims(**bfSimsKwargs)

        # self.ffluxBFConversionDict = self.genFFluxBFConversionDict()

        # combine the histograms from runs that started in basin A with those from basin B
        for samples in ['1e9', '1e11']:
            for keySetA,simA in self.bfSims[('samples', samples), ('basin', 'A')].items():
                keySetB = (keySetA - {('basin', 'A')}) | {('basin', 'B')}
                simB = self.bfSims[keySetB]
                opHA = simA.oparamHists['Sum']
                opHB = simB.oparamHists['Sum']

                if samples=='1e9':
                    # the masks for this data set are the inverse of the standard I eventually decided on, so fix that
                    for opH in [opHA, opHB]:
                        opH.remask(np.zeros(opH.h_mask.shape, dtype=bool))
                        opH.resliceH(np.s_[:101,:101])

                opHA.combine(opHB, inPlace=True)
                # for opH in [opHA, opHB]:
                #     print(opH.h.sum())

        self.bfHist = self.bfSims[(('theta', '1.0e+00'), ('samples', '1e11'), ('basin', 'A'))].peek().oparamHists['Sum']

        # self.bfHist.resliceH(np.s_[:101,:101])
        
        # ffluxSims stuff
        self.ffluxRootPath = ffluxRootPath
        ffluxSimsKwargs = {'rootPath': self.ffluxRootPath, 'filterRules': filterRules}
        if self.lmintOnly:
            ffluxSimsKwargs['fType'] = 'lmint'
        self.ffluxSims = Sims(**ffluxSimsKwargs)
        for ffluxSim in self.ffluxSims.values():
            ffluxSim.ffluxHists.transformKwargs = {'tilingIDs':((1,2),3)}
#             ffluxSim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),3)}

        self.ffluxHist = next(self.ffluxSims.values().__iter__()).ffluxHists(('BinTilingIDs', (3,)))
        self.ffluxHist2D = next(self.ffluxSims.values().__iter__()).ffluxHists(('BinTilingIDs', (1,2)))
        # try:
        #     self.ffluxHist = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (3,)))]
        #     self.ffluxHist2D = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (1,2)))]
        # except:
        #     print('could not make FFluxHist objects')
        #     pass

        self.assignBFHist(normalizeBF=False)
        # self.genKLDiv()


    def assignBFHist(self, normalizeBF=True):
        '''
        "assign" the proper bfHist to each of the ffluxHists
        '''
        for dim in (1,2):
            for ffluxKey in self.ffluxSims.keys():
                theta = MagicDict.getElemFromKey('theta', ffluxKey)
                keySetA = self.getKeySetA(theta)
                # try:
                #     bfAKey = self.ffluxBFConversionDict[theta]
                # except KeyError:
                #     bfAKey = self.ffluxBFConversionDict['1.0e+00']

                bfHist = self.getBFHistByKeyDim(keySetA, dim)
                if normalizeBF:
                    bfHist.normalize()
                try:
                    ffluxHist = self.getFFluxHistByKeyDim(ffluxKey, dim)
                    ffluxHist.bfHist = bfHist
                except (AttributeError, KeyError):
                    pass

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

    def genFFluxBFConversionDict(self):
        bfKeys = []
        for key in self.bfSims[('basin', 'A')].keys():
            bfKeys.append(sorted(tuple(key)))
        bfKeys.sort()

        ffluxThetas = ['%.1e' % num for num in np.logspace(-1, 1, 11, 10)]

        return OrderedDict(zip(ffluxThetas, bfKeys))

    def genKLDiv(self, dim=1, mask1DExtremes=35, normalize='mask', sliceDiagStart=None, sliceDiagEnd=None, sliceDiagInverse=False):
        for ffluxKey,ffluxSim in self.ffluxSims.items():
            theta = MagicDict.getElemFromKey('theta', ffluxKey)
            keySetA = self.getKeySetA(theta)
            bfHist = self.getBFHistByKeyDim(keySetA, dim)
            ffluxHist = self.getFFluxHistByKeyDim(ffluxKey, dim)

            # zeroMask = np.logical_or(ffluxHist.h==0, bfHist.h==0)
            # bfHist.remask(zeroMask)
            # bfHist.normalize()
            # ffluxHist.remask(zeroMask)
            # ffluxHist.normalize()
            if sliceDiagStart is not None and sliceDiagEnd is not None:
                bfHistForCalc = deepcopy(bfHist)
                bfHistForCalc.maskDiagonalSlice(start=sliceDiagStart, end=sliceDiagEnd, inverse=sliceDiagInverse, normalize='raw')

                ffluxHistForCalc = deepcopy(ffluxHist)
                ffluxHistForCalc.maskDiagonalSlice(start=sliceDiagStart, end=sliceDiagEnd, inverse=sliceDiagInverse, normalize='raw')
            elif dim==1 and mask1DExtremes:
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

    def genKLDivArrs(self, dim, simsKey):
        if dim==1:
            ffluxHists = self.ffluxSims[simsKey].ffluxHists(('BinTilingIDs', (3,)))
        elif dim==2:
            ffluxHists = self.ffluxSims[simsKey].ffluxHists(('BinTilingIDs', (1,2)))
        ffluxHistsCopy = deepcopy(ffluxHists)
        bfHists = ffluxHistsCopy.bfHist
        bfHists.normalize()

        def ConvertToKLDiv(ffluxHist):
            return ffluxHist.bfHist.getKLDivergenceArr(ffluxHist, normalize='mask')

        return ffluxHists.mapFunc(ConvertToKLDiv, doRaise=True)

    def genKLDivVariants(self):
        '''
        gen variant kl div values, such as over only the transition region
        '''
        for ffluxKey,ffluxSim in self.ffluxSims.items():
            ffluxHist2D = ffluxSim.ffluxHists[('InterfaceTilingID', 0), ('BinTilingIDs', (1,2))]
            theta = MagicDict.getElemFromKey('theta', ffluxKey)
            keySetA = self.getKeySetA(theta)
            bfHist = self.bfSims[keySetA].peek().oparamHists['Sum']

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

    def genKLDivStd(self, dim=1, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        keySetA = self.getKeySetA(theta)
        bfHist = self.getBFHistByKeyDim(keySetA, dim)

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(bfHist.getDownsampleFromRaw(nSample=n))

        self.divDict = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [bfHist])):
            bfHistWithoutSamples = bfHist.getCopy()
            bfHistWithoutSamples.h_raw-=hist.h_raw
            bfHistWithoutSamples.h_cache_dirty = True
            print(bfHistWithoutSamples.h.sum())
            print(hist.h.sum())
            self.divDict[name] = bfHistWithoutSamples.getKLDivergence(hist, normalize='mask')
        print('divDict contents:')
        print(list(self.divDict.keys()))
        print(list(self.divDict.values()))

        return self.divDict

    def genKLDivStdBetweenBasins(self, nDownsampled=7, theta='1.0e+00'):
        nSamples = np.logspace(2, 1+nDownsampled, base=10, num=nDownsampled, dtype=int)

        keySetA = self.getKeySetA(theta)
        keySetB = (set(keySetA) - {('basin', 'A')}) | {('basin', 'B')}
        bfHistA = self.bfSims[keySetA].oparamHists['Sum']
        bfHistB = self.bfSims[keySetB].oparamHists['Sum']

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(bfHistA.getDownsampleFromRaw(nSample=n))

        self.divDict = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [self.bfHist])):
            self.divDict[name] = bfHistB.getKLDivergence(hist, normalize='mask')
        print('divDict contents:')
        print(list(self.divDict.keys()))
        print(list(self.divDict.values()))

    def genDivStdWithSplit(self, divType='KL', dim=1, nDownsampled=7, edges=None, sliceStart=None, sliceEnd=None,
                           sliceCoordinate=True, sliceDiagonal=False, sliceInverse=True, theta='1.0e+00'):
        startSampleN,endSampleN = (2,nDownsampled+1) if Depth(nDownsampled)==0 else (nDownsampled[0],nDownsampled[1]+1)

        nSamples = np.logspace(startSampleN, endSampleN, base=10, num=(endSampleN - startSampleN + 1), dtype=int)

        keySetA = self.getKeySetA(theta)
        bfHist = self.getBFHistByKeyDim(keySetA, dim)

        testHist,sampleHist = bfHist.split()
        testHist.reweight(1)
        sampleHist.reweight(1)

        print(testHist.h.sum())
        print(sampleHist.h.sum())

        if divType=='KL':
            DivFunc = testHist.getKLDivergence
        elif divType=='JS':
            DivFunc = testHist.getJSDivergence

        self.opHistDownsamples = []
        for n in nSamples:
            with timewith('%.0e' % n) as tw:
                self.opHistDownsamples.append(sampleHist.getDownsampleFromRaw(nSample=n))
        
        divKwargs = {'edges':edges, 'sliceStart':sliceStart, 'sliceEnd':sliceEnd,
                     'sliceCoordinate':sliceCoordinate, 'sliceDiagonal':sliceDiagonal, 'sliceInverse':sliceInverse}

            # = {'coordinate':sliceCoordinate, 'diagonal':sliceDiagonal,
            #            'inverse':sliceInverse, 'normalize':'raw'}
        self.divDict = OrderedDict()
        for name,hist in zip(chain(nSamples, ['original']), chain(self.opHistDownsamples, [sampleHist])):
            self.divDict[name] = DivFunc(hist, **divKwargs)
            # if edges is not None:
            #     divs = []
            #     for start,end in zip(edges[:-1], edges[1:]):
            #         hist.maskSlice(start=[start], end=[end], **sliceKwargs)
            #         divs.append(testHist.getKLDivergence(hist, normalize='mask'))
            #     self.divDict[name] = divs
            # else:
            #     if sliceStart is not None and sliceEnd is not None:
            #         hist.maskSlice(start=sliceStart, end=sliceEnd, **sliceKwargs)
            #     self.divDict[name] = testHist.getKLDivergence(hist, normalize='mask')
        print('divDict contents:')
        for item in self.divDict.items():
            try:
                print('%.0e: %s' % item)
            except TypeError:
                print('%s: %s' % item)
        return self.divDict

    def genStdErrArrs(self, dim, simsKey):
        if dim==1:
            ffluxHists = self.ffluxSims[simsKey].ffluxHists(('BinTilingIDs', (3,)))
        elif dim==2:
            ffluxHists = self.ffluxSims[simsKey].ffluxHists(('BinTilingIDs', (1,2)))
        ffluxHistsCopy = deepcopy(ffluxHists)
        bfHists = ffluxHistsCopy.bfHist
        bfHists.normalize()

        def ConvertToStdErr(ffluxHist):
            return ffluxHist.bfHist.getStdErrArr(ffluxHist)
            # ffluxHist.h = (ffluxHist.bfHist.h - ffluxHist.h)/ffluxHist.bfHist.h
            # return ffluxHist

        return ffluxHists.mapFunc(ConvertToStdErr)

    def getBFHistByKeyDim(self, key, dim):
        if dim==1:
            if not hasattr(self, '_ran_gen1DBFHists') or not self._ran_gen1DBFHists:
                self.gen1DBFHists()
            try:
                return self.bfSims[key].peek().oparamHists['Sum'].hist1D
            except AttributeError:
                return self.bfSims[key].oparamHists['Sum'].hist1D
        elif dim==2:
            try:
                return self.bfSims[key].peek().oparamHists['Sum']
            except AttributeError:
                return self.bfSims[key].oparamHists['Sum']
        else:
            raise ValueError('Got %s for dim. Please choose either 1 or 2' % dim)

    def getFFluxHistByKeyDim(self, simsKey, dim):
        if dim==1:
            bTIDTup = ('BinTilingIDs', (3,))
        elif dim==2:
            bTIDTup = ('BinTilingIDs', (1,2))
        else:
            raise ValueError('Got %s for dim. Please choose either 1 or 2' % dim)

        for key,val in self.ffluxSims[simsKey].ffluxHists.items():
            if bTIDTup in key:
                return val

        raise KeyError("No key containing %s found in self.ffluxSims[%s].ffluxHists" % (bTIDTup, simsKey))

    @staticmethod
    def getKeySetA(theta, samples='1e11'):
        return (('theta', theta), ('samples', samples), ('basin', 'A'))

if __name__=='__main__':
    CompareBFFFluxHists()