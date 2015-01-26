from ..main.sim import Sim
from toggleReplicate import ToggleReplicate

class ToggleSim(Sim):
    replicateType = ToggleReplicate
    
    def TransitionCount(self):
        '''
        return the count of replicates in this .lm file that transitioned from one basin to another
        '''
        if self.oparam==None:
            self.Pdf()
        transitionCount = np.sum([sim.TransitionProbability() for sim in self.sims])
        return int(transitionCount)