/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012 Roberts Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 * 
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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

#ifndef LM_TUNE_H_
#define LM_TUNE_H_

/**
 * The compression level used for gzip when writing out the lattice frames. 1=lowest, 9=highest.
 */
#ifndef TUNE_LATTICE_GZIP_COMPRESSION_LEVEL
#define TUNE_LATTICE_GZIP_COMPRESSION_LEVEL     1
#endif

/**
 * The chunk size to use when writing gzipped lattice frames.
 */
#ifndef TUNE_LATTICE_GZIP_CHUNK_SIZE
#define TUNE_LATTICE_GZIP_CHUNK_SIZE            16
#endif


#ifndef TUNE_PARAMETER_VALUES_BUFFER_SIZE
#define TUNE_PARAMETER_VALUES_BUFFER_SIZE       1000
#endif

#ifndef TUNE_LOCAL_RNG_CACHE_SIZE
#define TUNE_LOCAL_RNG_CACHE_SIZE               2048
#endif


/**
 * The next reaction method CME sampler can reuse existing propensities for dependent reaction changes, but in some
 * cases it may be faster to just generate a new random number. Set this flag to 0 to disable reuse of propensities.
 */
#ifndef TUNE_NRM_REUSE_PROPENSITIES
#define TUNE_NRM_REUSE_PROPENSITIES             1
#endif

/**
 * The number of objects the lattice builder will buffer before writing them to the file.
 */
#ifndef TUNE_LATTICE_BUILDER_WRITE_BUFFER_SIZE
#define TUNE_LATTICE_BUILDER_WRITE_BUFFER_SIZE  1000
#endif

/**
 * The number of objects in the spatial model that will be read from the file in one read operation.
 */
#ifndef TUNE_SPATIAL_MODEL_OJBECT_READ_BUFFER_SIZE
#define TUNE_SPATIAL_MODEL_OJBECT_READ_BUFFER_SIZE  1000
#endif


//
// Tuning parameters for the MpdRdmeSolver.
//

#if !defined TUNE_MPD_X_BLOCK_MAX_X_SIZE
#define TUNE_MPD_X_BLOCK_MAX_X_SIZE             256
#endif
#if !defined TUNE_MPD_Y_BLOCK_X_SIZE
#define TUNE_MPD_Y_BLOCK_X_SIZE                 32
#endif
#if !defined TUNE_MPD_Y_BLOCK_Y_SIZE
#define TUNE_MPD_Y_BLOCK_Y_SIZE                 4
#endif
#if !defined TUNE_MPD_Z_BLOCK_X_SIZE
#define TUNE_MPD_Z_BLOCK_X_SIZE                 32
#endif
#if !defined TUNE_MPD_Z_BLOCK_Z_SIZE
#define TUNE_MPD_Z_BLOCK_Z_SIZE                 4
#endif

/**
 * Parameters for the reaction kernel thread block size.
 */
#if !defined TUNE_MPD_REACTION_BLOCK_X_SIZE
#define TUNE_MPD_REACTION_BLOCK_X_SIZE          32
#endif
#if !defined TUNE_MPD_REACTION_BLOCK_Y_SIZE
#define TUNE_MPD_REACTION_BLOCK_Y_SIZE          4
#endif

#if !defined TUNE_MPD_MAX_PARTICLE_OVERFLOWS
#define TUNE_MPD_MAX_PARTICLE_OVERFLOWS         512
#endif

#if !defined TUNE_MPD_MAX_OVERFLOW_REPLACEMENT_DIST
#define TUNE_MPD_MAX_OVERFLOW_REPLACEMENT_DIST         4
#endif

#endif
