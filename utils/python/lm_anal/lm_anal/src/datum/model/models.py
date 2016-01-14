from lm_anal.src.datum.dataSingleton import DataSingleton
from lm_anal.src.datum.model.model import Model
from lm_anal.src.io.hdf5.model import ModelIO

__all__ = ['Models']

class Models(DataSingleton):
    datumType = Model
    Hdf5IOType = ModelIO
    SFileType = None