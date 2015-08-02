import os

from lm_anal.src.datum.trajectory.trajectories import Trajectories
from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory

__all__ = ['OParamTrajectories']

class OParamTrajectories(Trajectories):
    datumType = OParamTrajectory