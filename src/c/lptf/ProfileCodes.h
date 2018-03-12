/*
 * University of Illinois Open Source License
 * Copyright 2010 Luthey-Schulten Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts
 */
#ifndef TIMINGCONSTANTS_H_
#define TIMINGCONSTANTS_H_

#define PROF_THREAD_VARIABLE_START                  3

#define PROF_MAIN_RUN                               1
#define PROF_SIM_RUN                                2

#define PROF_MASTER_READ_STATIC_MSG                 3
#define PROF_MASTER_READ_FINISHED_MSG               4
#define PROF_MASTER_FINISHED_THREAD                 5
#define PROF_MASTER_SLEEP                           6

#define PROF_REPLICATE_EXECUTE                      10
#define PROF_REPLICATE_WRITE_DATASET                11

#define PROF_MESSAGE_SEND                           50
#define PROF_MESSAGE_SERIALIZE                      51
#define PROF_MESSAGE_RECEIVE                        55
#define PROF_MESSAGE_PARSE                          56
#define PROF_MESSAGE_RECEIVE_SPURIOUS_WAKEUP        57

#define PROF_WORK_UNIT_RUN                          60
#define PROF_WORK_UNIT_RUN_PART                     61
#define PROF_WORK_UNIT_SAVE_PART                    62

#define PROF_DATAOUTPUT_RUN                         100
#define PROF_DATAOUTPUT_WRITE_DATASET               101
#define PROF_DATAOUTPUT_HDF_WRITE_COUNTS            110
#define PROF_DATAOUTPUT_HDF_WRITE_FPT               111
#define PROF_DATAOUTPUT_HDF_WRITE_PV                112
#define PROF_DATAOUTPUT_HDF_WRITE_LATTICE           113

#define PROF_SLAVE_SLEEP                            200

#define PROF_SIM_EXECUTE                            299

#define PROF_SERIALIZE_COUNTS                       300
#define PROF_SERIALIZE_FPT                          301
#define PROF_DESERIALIZE_COUNTS                     302
#define PROF_DESERIALIZE_FPT                        303
#define PROF_SERIALIZE_PV                           304
#define PROF_DESERIALIZE_PV                         305
#define PROF_SERIALIZE_LATTICE                      306
#define PROF_DESERIALIZE_LATTICE                    307
#define PROF_DETERMINE_COUNTS                       308

#define PROF_INIT_XORWOW_RNG                        324
#define PROF_GENERATE_XORWOW_RNG                    325
#define PROF_CACHE_RNG                              326
#define PROF_LAUNCH_XORWOW_RNG                      327
#define PROF_COPY_XORWOW_RNG                        328
#define PROF_COPY_XORWOW_EXP_RNG                    329
#define PROF_COPY_XORWOW_NORM_RNG                   330
#define PROF_CACHE_EXP_RNG                          331
#define PROF_CACHE_NORM_RNG                         332

#define PROF_MPD_TIMESTEP                           500
#define PROF_MPD_X_DIFFUSION                        501
#define PROF_MPD_Y_DIFFUSION                        502
#define PROF_MPD_Z_DIFFUSION                        503
#define PROF_MPD_REACTION                           504
#define PROF_MPD_SYNCHRONIZE                        505
#define PROF_MPD_OVERFLOW                           506

#define PROF_NSM_INIT_QUEUE                         600
#define PROF_NSM_BUILD_QUEUE                        601
#define PROF_NSM_LOOP		                        602
#define PROF_NSM_PERFORM_SUBVOLUME_EVENT		    603
#define PROF_NSM_UPDATE_SUBVOLUME_PROPENSITY		604
#define PROF_NSM_CALCULATE_SUBVOLUME_PROPENSITY		605
#define PROF_NSM_UPDATE_QUEUE               		606
#define PROF_NSM_SERIALIZE_LATTICE                  607

#define PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT      700
#define PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ZERO 701
#define PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ONE 702
#define PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_N 703

#define PROF_WUR_RUN_WORK_UNITS                     800
#define PROF_WUR_GENERATE_TRAJECTORY                801

#define PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED     900
#define PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_ONE 901
#define PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_TWO 902
#define PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_THREE 903

#define PROF_SLOTS_WORK_UNIT_FINISHED               1001

#define PROF_MENV_RUN_SIM                           1100
#define PROF_MENV_RUN_PHASE                         1101
#define PROF_MENV_START_REPLICATE                   1102
#define PROF_MENV_CONT_REPLICATE                    1103
#define PROF_MENV_ASSIGN_WORK                       1104
#define PROF_MENV_BUILD_WORK_UNIT                   1105

#define PROF_PDE_EXECUTE                            1200

#define PROF_NDARRAY_SERIALIZE                      1300
#define PROF_NDARRAY_DESERIALIZE                    1301

#endif /* TIMINGCONSTANTS_H_ */
