/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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
 * Author(s): Elijah Roberts, Max Klein
 */

#ifndef LM_EXCEPTIONS_H_
#define LM_EXCEPTIONS_H_

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <string>

namespace lm
{

// macro that passes the file:line from which it is raised to an exception's constructor
#define THROW_EXCEPTION(exception, ...) throw exception(__LINE__, __FILE__, __VA_ARGS__);

class Exception : public std::exception
{
public:
    Exception(const char * message="")                                                      {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s", message);}
    Exception(const char * message, const int arg)                                          {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d", message, arg);}
    Exception(const char * message, const int arg1,    const int arg2)                      {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d, %d", message, arg1, arg2);}
    Exception(const char * message, const int arg1,    const char * arg2)                   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d, %s", message, arg1, arg2);}
    Exception(const char * message, const int arg1,    const char* arg2,  const char* arg3) {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d, %s, %s", message, arg1, arg2, arg3);}
    Exception(const char * message, const int arg1,    const int arg2,    const int arg3)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d, %d, %d", message, arg1, arg2, arg3);}
    Exception(const char * message, const char * arg)                                       {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s", message, arg);}
	Exception(const char * message, const char * arg1, const char* arg2)                    {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s", message, arg1, arg2);}
    Exception(const char * message, const char * arg1, const char* arg2,  const char* arg3) {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s, %s", message, arg1, arg2, arg3);}
    Exception(const char * message, const char * arg1, const int arg2)                      {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %d", message, arg1, arg2);}
    Exception(const char * message, const char * arg1, const int arg2,    const int arg3)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %d, %d", message, arg1, arg2, arg3);}
    Exception(const char * message, const int arg,     const char * file, const int line)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d (%s:%d)", message, arg, file, line);}
    Exception(const char * message, const char * arg,  const char * file, const int line)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s (%s:%d)", message, arg, file, line);}

    // variadic constructors
    template <typename T> Exception(T format, va_list args) {printException(format, args);}
    template <typename T> Exception(int line, const char *file, T format, va_list args) {printExceptionWithLine(file, line, format, args);}

    virtual ~Exception() throw() {}

    virtual const char* preamble() const throw() {return "Exception";}
    virtual const char* what() const throw() {return messageBuffer;}

protected:
    // overloaded function that converts std::strings to c-strings and leaves c-strings untouched
    static const char* cstr(const char* s) {return s;}
    static const char* cstr(const std::string s) {return s.c_str();}

    // format can be either char* or std::string
    template <typename T> void printException(T format, va_list args)
    {
        int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s:\n", preamble());
        vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, cstr(format), args);
    }

    template <typename T> void printExceptionWithLine(const char* file, int line, T format, va_list args)
    {
        int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s (%s:%d):\n", preamble(), file, line);
        vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, cstr(format), args);
    }

protected:
    static const int MAX_MESSAGE_SIZE = 1025;
    char messageBuffer[MAX_MESSAGE_SIZE];
};

class CommandLineArgumentException : public Exception
{
public:
    CommandLineArgumentException(const char* message) : Exception(message) {}
    CommandLineArgumentException(const char* message, const char* arg1) : Exception(message, arg1) {}
//    virtual ~CommandLineArgumentException() throw() {}
};

class ConsistencyException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Consistency exception";}

    template <typename T> ConsistencyException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> ConsistencyException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class InputException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Input exception";}

    template <typename T> InputException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> InputException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class InvalidArgException : public Exception
{
public:
    InvalidArgException(const char* argMessage) : Exception("Invalid argument", argMessage) {}
    InvalidArgException(const char* arg, const char* argMessage) : Exception("Invalid argument", arg, argMessage) {}
    InvalidArgException(const char* arg, const char* argMessage, const char * argMessageParameter) : Exception("Invalid argument", arg, argMessage, argMessageParameter) {}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter) : Exception("Invalid argument", arg, argMessage, argMessageParameter) {}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter1, const int argMessageParameter2) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d)", "Invalid argument", arg, argMessage, argMessageParameter1, argMessageParameter2);}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter1, const int argMessageParameter2, const int argMessageParameter3) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d,%d)", "Invalid argument", arg, argMessage, argMessageParameter1, argMessageParameter2, argMessageParameter3);}
    InvalidArgException(const char* arg, const char* argMessage, const int a1, const int a2, const int a3, const int a4, const int a5) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d,%d,%d,%d)", "Invalid argument", arg, argMessage, a1, a2, a3, a4, a5);}
    InvalidArgException(const char* arg, const char* argMessage, const int a1, const int a2, const int a3, const int a4, const int a5, const int a6) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d,%d,%d,%d,%d)", "Invalid argument", arg, argMessage, a1, a2, a3, a4, a5, a6);}
//    virtual ~InvalidArgException() throw() {}
};

class IOException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "IO exception";}

    template <typename T> IOException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> IOException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class NotFoundException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Not found exception";}

    template <typename T> NotFoundException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> NotFoundException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class NullPointerException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Null pointer exception -> attempted to dereference a pointer to NULL";}

    template <typename T> NullPointerException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> NullPointerException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class RuntimeException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Runtime exception";}

    template <typename T> RuntimeException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> RuntimeException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class UnimplementedException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Unimplemented exception";}

    template <typename T> UnimplementedException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}
    template <typename T> UnimplementedException(int line, const char *file, T format, ...) : Exception(line, file, format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class ZlibException : public Exception
{
public:
    ZlibException(const int errorNumber) : Exception("ZLib exception", errorNumber) {}
};

#define ZLIB_EXCEPTION_CHECK(zlib_call) {int _zlib_ret_=zlib_call; if (_zlib_ret_ != Z_OK) throw lm::ZlibException(_zlib_ret_);}

class PosixException : public Exception
{
public:
    PosixException(const int errorNumber) : Exception("Posix exception", errorNumber) {}
};

#define POSIX_EXCEPTION_CHECK(posix_call) {int _posix_ret_=posix_call; if (_posix_ret_ != 0) throw lm::PosixException(_posix_ret_);}

}

#endif
