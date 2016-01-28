from lm_anal.src.datum.dataSingleton import DataSingleton
from lm_anal.src.datum.model.model import Model
import lm_anal.src.helper as hlp

lzModelsIO = hlp.LazyClass(modName='lm_anal.src.io.hdf5.model', clsName='ModelsIO')

__all__ = ['Models']

class Models(DataSingleton):
    datumType = Model
    hdf5IOType = lzModelsIO
    sfileType = None