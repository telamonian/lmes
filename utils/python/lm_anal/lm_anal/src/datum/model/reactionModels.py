from lm_anal.src.datum.model.models import Models
from lm_anal.src.datum.model.reactionModel import ReactionModel
from lm_anal.src.io.hdf5.model import ReactionModelIO

__all__ = ['ReactionModels']

class ReactionModels(Models):
    singletonKey = 'Reaction'

    datumType = ReactionModel
    Hdf5IOType = ReactionModelIO
    SFileType = None