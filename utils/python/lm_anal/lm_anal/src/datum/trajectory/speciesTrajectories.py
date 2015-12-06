import os

from lm_anal.src.datum.trajectory.trajectories import Trajectories
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory
from lm_anal.src.io.hdf5.trajectory import SpeciesTrajectoriesIO
from lm_anal.src.serializable.trajectory import SpeciesTrajectoriesSrlz

__all__ = ['SpeciesTrajectories']

class SpeciesTrajectories(Trajectories, SpeciesTrajectoriesSrlz):
    datumType = SpeciesTrajectory
    Hdf5IOType = SpeciesTrajectoriesIO
    SFileType = None