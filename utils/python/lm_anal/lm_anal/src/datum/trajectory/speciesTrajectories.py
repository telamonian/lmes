import os

from lm_anal.src.datum.trajectory.trajectories import Trajectories
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO

__all__ = ['SpeciesTrajectories']

class SpeciesTrajectories(Trajectories):
    datumType = SpeciesTrajectory
    Hdf5IOType = BruteForceTrajectoriesIO
    SFileType = None