from lm_anal.src.datum.data import Data

__all__ = ['DataSingleton']

class DataSingleton(Data):
    '''
    for representing Data for which there can really and truly only be one Datum per Sim,
    such as ReactionModels and SimulationParameters
    '''
    singletonKey = None

    def __getattr__(self, attr):
        return self.map[self.singletonKey].__getattr__(attr)