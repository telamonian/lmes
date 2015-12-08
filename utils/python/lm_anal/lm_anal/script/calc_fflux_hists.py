from argparse import ArgumentParser
from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sim, Sims

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
# inputFilePath =
# ffluxRootPath = Path('/Users/tel/temp_data/gts_-_fflux_-_barrier_height_-_crossingsPerPhase_-_phaseZeroTime').resolve()           #(thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()

class FFluxHistsExample(object):
    def __init__(self, ffluxRootPath, inputFilePath=None):
        if inputFilePath is not None:
            self.inputFilePath = inputFilePath
            self.inputSim = Sim(fPath=self.inputFilePath)
        
        self.ffluxRootPath = ffluxRootPath
        self.ffluxSims = Sims(rootPath=self.ffluxRootPath)
        
        for key,sim in self.ffluxSims.items():
            print('starting probability map of %s' % str(key))
            sim.ffluxHists.transformKwargs = {'tilingIDs':((1,2),3)}
#             sim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),3)}
            try:
                sim.ffluxHists.map
                print('finished %s' % str(key))
                del sim
                del self.ffluxSims[key]
            except: #AttributeError:
                print("%s didn't finish" % str(key))
                del sim
                del self.ffluxSims[key]

def Main():
    defaultIFP = (thisScriptDir / Path('../test/testData/biphasic_switch.lm')).resolve()
    defaultDP = (thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()

    parser = ArgumentParser('example script that will take Forward Flux simulation output stored in hdf5 .lm files and create .lmint files with multidimensional histograms of the epigenetic landscape')
    parser.add_argument('dataPath', nargs='?', default=defaultDP, help='path to single .lm file with fflux output, or to root of dir tree containing many such .lm files')
    parser.add_argument('-i', '--inputFilePath', default=None, help='path to .lm file with appropriate inputs (oparams, tilings) if these are lacking from your data files')

    args = vars(parser.parse_args())
    args['inputFilePath'] = defaultIFP if args['inputFilePath'] is 'default' else args['inputFilePath']

    _Main(**args)

def _Main(dataPath, inputFilePath=None):
    FFluxHistsExample(ffluxRootPath=dataPath, inputFilePath=inputFilePath)

if __name__=='__main__':
    Main()
