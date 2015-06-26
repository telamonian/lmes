import os

from src.datum.trajectory.trajectories import Trajectories
from src.datum.trajectory.speciesTrajectory import SpeciesTrajectory

class SpeciesTrajectories(Trajectories):
    datumType = SpeciesTrajectory