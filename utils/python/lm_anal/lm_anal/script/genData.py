from argparse import ArgumentParser
import os
from pathlib import Path

from lm_anal.src.helper import CamelCaseLower
from lm_anal.src.main import Sim, Sims

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))

class GenData(object):
    def __init__(self, dataType, simsRootPath, filterRules=None, inputFilePath=None):
        if inputFilePath is not None:
            self.inputFilePath = inputFilePath
            self.inputSim = Sim(fPath=self.inputFilePath)
        
        self.simsRootPath = simsRootPath
        self.sims = Sims(rootPath=self.simsRootPath)

        for key,sim in self.sims.items():
            simName = str(tuple(key))

            print('starting generation of %s from Sim %s' % (dataType, simName))
            # sim.ffluxHists.transformKwargs = {'tilingIDs':((1,2),3)}
#             sim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),3)}
            try:
                sim.__getattribute__(CamelCaseLower(dataType)).map
                print('finished %s' % simName)
                del sim
                del self.sims[key]
            except: #AttributeError:
                print("%s didn't finish" % simName)
                del sim
                del self.sims[key]

def Main():
    parser = ArgumentParser('example script that will take Forward Flux simulation output stored in hdf5 .lm files and create .lmint files with multidimensional histograms of the epigenetic landscape')
    parser.add_argument('dataType', help='name of a Lattice Microbes Analysis dataType that you want to generate')
    parser.add_argument('simsRootPath', help='path to single .lm file with simulation data, or to root of dir tree containing many such .lm files')
    parser.add_argument('-f', '--filterRules', nargs='+', help="filter rules. '+<regex>' -> include, '-<regex>' -> exclude, first rule that applies to a file is used, files are included by default\n" +
                                                               "example: -f '-b\wb.*' '+.+boo' -> this will include george and faboo, and exclude bob and baboo")
    parser.add_argument('-i', '--inputFilePath', default=None, help='path to .lm file with appropriate inputs (oparams, tilings) if these are lacking from your data files')

    kwargs = vars(parser.parse_args())

    _Main(**kwargs)

def _Main(dataType, simsRootPath, filterRules=None, inputFilePath=None):
    GenData(dataType=dataType, simsRootPath=simsRootPath, filterRules=filterRules, inputFilePath=inputFilePath)

if __name__=='__main__':
    Main()
