#!/usr/bin/env python3

from lma import Sim

if __name__=='__main__':
    sim = Sim(fpath='ffluxSupervisorInput.sfile', mode='w')

    tags = frozenset((('stage', 'Custom'),))
    ffluxSimulationInput = sim.ffluxSimulationInputs[tags]

    # setting the stage
    ffluxStage = ffluxSimulationInput.fflux_stage_list.fflux_stages.add()

    ffluxStage.basin_index = 0
    ffluxStage.tiling_id = 0

    # setting a phase
    ffluxPhase = ffluxStage.fflux_phases.add()

    ffluxPhase.fflux_phase_index = 1
    ffluxPhase.tile_index = 1
    ffluxPhase.basin_index = 0
    ffluxPhase.tiling_id = 0

    # setting a phase limit
    ffluxPhaseLimit = ffluxPhase.fflux_phase_limit

    ffluxPhaseLimit.stop_condition = 1 # TRAJECTORY_COUNT
    ffluxPhaseLimit.events_per_trajectory = 1
    ffluxPhaseLimit.trajectories_per_phase = 4

    ffluxPhaseLimit.uvalue = int(1e4)

    # setting a start point
    start_point = ffluxPhase.start_points.add()
    start_point.species_coordinates = [4, 8, 1, 0, 0, 0, 0]
    start_point.count = 1
    start_point.times = [0.0]

    sim.ffluxSimulationInputs.wtf(mode='w')