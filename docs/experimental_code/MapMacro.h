/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
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
 * Author(s): Max Klein
 */
#ifndef MAPMACRO
#define MAPMACRO

#include <iostream>

// MAP macro modified from https://github.com/swansontec/map-macro
// EVAL has been configured for a max recursion depth of 64
#define EVAL0(...) __VA_ARGS__
#define EVAL1(...) EVAL0 (EVAL0 (__VA_ARGS__))
#define EVAL2(...) EVAL1 (EVAL1 (__VA_ARGS__))
#define EVAL3(...) EVAL2 (EVAL2 (__VA_ARGS__))
#define EVAL4(...) EVAL3 (EVAL3 (__VA_ARGS__))
#define EVAL(...)  EVAL4 (EVAL4 (__VA_ARGS__))

#define MAP_END(...)
#define MAP_OUT

#define MAP_GET_END() 0, MAP_END
#define MAP_NEXT0(test, next, ...) next MAP_OUT
#define MAP_NEXT1(test, next) MAP_NEXT0 (test, next, 0)
#define MAP_NEXT(test, next)  MAP_NEXT1 (MAP_GET_END test, next)

#define MAP0(f, x, peek, ...) f(x) MAP_NEXT (peek, MAP1) (f, peek, __VA_ARGS__)
#define MAP1(f, x, peek, ...) f(x) MAP_NEXT (peek, MAP0) (f, peek, __VA_ARGS__)
#define MAP(f, ...) EVAL (MAP1 (f, __VA_ARGS__, (), 0))

#define MAP_PAIRS0(f, x, y, peek, ...) f(x, y) MAP_NEXT (peek, MAP_PAIRS1) (f, peek, __VA_ARGS__)
#define MAP_PAIRS1(f, x, y, peek, ...) f(x, y) MAP_NEXT (peek, MAP_PAIRS0) (f, peek, __VA_ARGS__)
#define MAP_PAIRS(f, ...) EVAL (MAP_PAIRS1 (f, __VA_ARGS__, (), 0))

#define ACCUMULATE_GET_END() 0, MAP_END
#define ACCUMULATE_NEXT0(test, next, ...) next MAP_OUT
#define ACCUMULATE_NEXT1(test, next) ACCUMULATE_NEXT0 (test, next, 0)
#define ACCUMULATE_NEXT(test, next)  ACCUMULATE_NEXT1 (ACCUMULATE_GET_END test, next)

#define ACCUMULATE0(f, x, y, peek, ...) ACCUMULATE_NEXT (peek, ACCUMULATE1) (f, f(x, y), peek, __VA_ARGS__) f(x, y)
#define ACCUMULATE1(f, x, y, peek, ...) ACCUMULATE_NEXT (peek, ACCUMULATE0) (f, f(x, y), peek, __VA_ARGS__) f(x, y)
#define ACCUMULATE(f, ...) EVAL (ACCUMULATE1 (f, __VA_ARGS__, (), 0))

// paster
#define _PASTER(part0, part1) part0 ## part1
#define PASTER(...) ACCUMULATE(_PASTER, __VA_ARGS__)

// stringify
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

// token substitution
#define NOSUB
#define GET_SUB_foo 0, subbedfoo
#define SUB0(test, sub, ...) sub
#define SUB1(test, sub) SUB0 (test, sub, 0)
#define SUB(tosub) SUB1 (GET_SUB_##tosub, NOSUB)

// helper macros for deducing information about wrapped fields
#define CATEGORY_float 0, numeric
#define CATEGORY_double 0, numeric
#define CATEGORY_int32_t 0, numeric
#define CATEGORY_int64_t 0, numeric
#define CATEGORY_uint32_t 0, numeric
#define CATEGORY_uint64_t 0, numeric
#define CATEGORY_TYPE0(test, sub, ...) sub
#define CATEGORY_TYPE1(test, sub) CATEGORY_TYPE0 (test, sub, 0)
#define CATEGORY_TYPE(type) CATEGORY_TYPE1 (CATEGORY_##type, embedded)

// print pair
#define PRINT_PAIR(str0, str1) \
    std::cout << STRINGIFY(str0) << ", " << STRINGIFY(str1) << '\n';

#define PRINT_PAIRS(...) \
    MAP_PAIRS(PRINT_PAIR, __VA_ARGS__)

#endif /* MAPMACRO */