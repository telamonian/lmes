from lm_anal.src.datum.data import Data

__all__ = ['DataSingleton']

class DataSingleton(Data):
    '''
    for representing Data for which there can really and truly only be one Datum per Sim,
    such as ReactionModels and SimulationParameters
    '''
    singletonKey = None

    @property
    def singleton(self):
        '''
        property that returns the single Datum belonging to this Data
        '''
        return self.map[self.singletonKey]

    def __getattr__(self, attr):
        '''
        pass calls to unknown attributes through to the .singleton Datum
        '''
        return self.singleton.__getattribute__(attr)