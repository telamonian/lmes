import os

from lm_anal.src.datum.trajectory.trajectories import Trajectories
from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory

class OParamTrajectories(Trajectories):
    datumType = OParamTrajectory