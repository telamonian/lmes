from lm_anal.src.datum.model.models import Models
from lm_anal.src.datum.model.reactionModel import ReactionModel
import lm_anal.src.helper as hlp

lzReactionModelsIO = hlp.LazyClass(modName='lm_anal.src.io.hdf5.model', clsName='ReactionModelsIO')

__all__ = ['ReactionModels']

class ReactionModels(Models):
    singletonKey = 'Reaction'

    datumType = ReactionModel
    hdf5IOType = lzReactionModelsIO
    sfileType = None