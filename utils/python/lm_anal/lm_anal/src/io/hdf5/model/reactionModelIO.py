from lm_anal.src.datum.model.reactionModels import ReactionModels
from lm_anal.src.io.hdf5 import HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.model.modelIO import ModelIO

__all__ = ['ReactionModelIO']

class ReactionModelIO(ModelIO):
    dataType = ReactionModels