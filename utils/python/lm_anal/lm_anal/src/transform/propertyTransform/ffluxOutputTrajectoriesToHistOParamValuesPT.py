from lm_anal.src.datum.hist import HistBase
from lm_anal.src.datum.fflux import FFluxBase
from lm_anal.src.transform.propertyTransform import BasePT

class FFluxOutputTrajectoriesToHistOParamValuesPT(BasePT):
    srcType = FFluxBase
    dstType = HistBase
    srcProp = 'trajectories'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparams, specTrajs, tilings, **kwargs):
        self.oparams = oparams
        self.specTrajs = specTrajs
        self.tilings = tilings
    
    def __call__(self, srcDatum, dstDatum):
        try:
            dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
            for trajID,traj in self.specTrajs:
                try:
                    trajPhase = srcDatum.trajectories['FORWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    dstDatum.addObservations(dstDatum.oparam.calc(traj.species_count))*srcDatum.basins['FORWARD'].probability_i_to_i_plus_one[trajPhase]
                except KeyError:
                    trajPhase = srcDatum.trajectories['BACKWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    dstDatum.addObservations(dstDatum.oparam.calc(traj.species_count))*srcDatum.basins['BACKWARD'].probability_i_to_i_plus_one[trajPhase]
        except AttributeError:
            pass