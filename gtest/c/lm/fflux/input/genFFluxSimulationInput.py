#!/usr/bin/env python3

from lma import Sim

if __name__=='__main__':
    sim = Sim(fpath='ffluxSimulationInput.sfile', mode='w')

    tags = frozenset((('stage', 'Custom'),))
    ffluxSimulationInput = sim.ffluxSimulationInputs[tags]

    # setting the stage
    ffluxStage = ffluxSimulationInput.fflux_stage_list.fflux_stages.add()

    ffluxStage.basin_index = 0
    ffluxStage.tiling_id = 0

    # setting a phase
    ffluxPhase = ffluxStage.fflux_phases.add()

    ffluxPhase.fflux_phase_index = 4
    ffluxPhase.tile_index = 4
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
    start_point.species_coordinates = [0,1,2,3,4,5,6]
    start_point.count = 1
    start_point.times = [0.0]

    # fflux_phase_output = ffluxSimulationInput.fflux_phase_output_list.fflux_phase_outputs.add()
    #
    # fflux_phase_output.tiling_id = 0
    # fflux_phase_output.basin_index = 0
    # fflux_phase_output.fflux_phase_index = 4
    #
    # end_point = fflux_phase_output.successful_trajectory_end_points.add()
    # end_point.species_coordinates = [0,1,2,3,4,5,6]
    # end_point.count = 1
    # end_point.times = [0.0]

    sim.ffluxSimulationInputs.wtf(mode='w', excludedFields=('tiling', 'pilot_stage'))