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

/*
### how `RESOLVE` works
    The basic idea is that while the preprocessor cannot distinguish between defined and undefined tokens (since it doesn't distinguish between undefined tokens and regular text), it can distinguish between single tokens and tuples.
*/

    #include <stdio.h>

    #define BLANK

    #define RESOLVE(token, default_token, ...) default_token

    #define QUOTE(str) #str
    #define QUOTE0(str) QUOTE(str) // need the intermediate QUOTE0 function here or else you get output like `RESOLVE(0, HelloWorld, , )` instead of `HelloWorld`
    #define EXPAND_AND_QUOTE(str, ...) QUOTE0(RESOLVE(str, BLANK, __VA_ARGS__)) // the BLANK token is optional here, can also be a literal blank

    #define MACRO 0, HelloWorld

    int main() {
        printf("%s\n", EXPAND_AND_QUOTE(MACRO));
        printf("%s\n", QUOTE(MACRO));

    #undef MACRO
        printf("%s\n", EXPAND_AND_QUOTE(MACRO));
        printf("%s\n", QUOTE(MACRO));

        printf("%s\n", EXPAND_AND_QUOTE(BLANK));
        printf("%s\n", QUOTE(BLANK));
    }