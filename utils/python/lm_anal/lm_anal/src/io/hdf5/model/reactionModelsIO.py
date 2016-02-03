from lm_anal.src.datum.model.reactionModels import ReactionModels
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs
from lm_anal.src.io.hdf5.model.modelsIO import ModelsIO

__all__ = ['ReactionModelsIO']

class ReactionModelsIO(ModelsIO):
    dataType = ReactionModels