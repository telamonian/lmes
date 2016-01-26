import os

from lm_anal.src.datum.trajectory.trajectories import Trajectories
from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory
from lm_anal.src.io.hdf5.trajectory import OParamTrajectoriesIO

__all__ = ['OParamTrajectories']

class OParamTrajectories(Trajectories):
    datumType = OParamTrajectory
    hdf5IOType = OParamTrajectoriesIO
    sfileType = None