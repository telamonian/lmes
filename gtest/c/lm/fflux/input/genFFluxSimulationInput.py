#!/usr/bin/env python3

from lma import Sim

if __name__=='__main__':
    sim = Sim(fpath='ffluxSimulationInput.sfile', mode='w')

    tags = frozenset((('stage', 'Custom'),))
    ffluxSimulationInput = sim.ffluxSimulationInputs[tags]

    fflux_phase_output = ffluxSimulationInput.fflux_phase_output_list.fflux_phase_outputs.add()

    fflux_phase_output.tiling_id = 0
    fflux_phase_output.basin_index = 0
    fflux_phase_output.fflux_phase_index = 4

    end_point = fflux_phase_output.successful_trajectory_end_points.add()
    end_point.species_coordinates = [0,1,2,3,4,5,6]
    end_point.count = 1
    end_point.times = [0.0]

    sim.ffluxSimulationInputs.wtf(mode='w')