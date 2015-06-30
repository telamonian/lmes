import os

from lm_anal.src.datum.trajectory.trajectories import Trajectories
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory

class SpeciesTrajectories(Trajectories):
    datumType = SpeciesTrajectory