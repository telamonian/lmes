/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef ROBERTSLAB_EXCEPTIONS_H
#define ROBERTSLAB_EXCEPTIONS_H

#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace robertslab
{

class Exception : public std::exception
{
public:
    Exception(const char * message="")                                                      {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s", message);}
    Exception(const char * message, const int arg)                                          {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d", message, arg);}
    Exception(const char * message, const int arg1,    const int arg2)                      {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d, %d", message, arg1, arg2);}
    Exception(const char * message, const int arg1,    const int arg2,    const int arg3)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d, %d, %d", message, arg1, arg2, arg3);}
    Exception(const char * message, const char * arg)                                       {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s", message, arg);}
    Exception(const char * message, const char * arg1, const char* arg2)                    {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s", message, arg1, arg2);}
    Exception(const char * message, const char * arg1, const int arg2)                      {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %d", message, arg1, arg2);}
    Exception(const char * message, const char * arg1, const int arg2,    const int arg3)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %d, %d", message, arg1, arg2, arg3);}
    Exception(const char * message, const int arg,     const char * file, const int line)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %d (%s:%d)", message, arg, file, line);}
    Exception(const char * message, const char * arg,  const char * file, const int line)   {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s (%s:%d)", message, arg, file, line);}

    template <typename T> Exception(T format, va_list args) {printException(format, args);}

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

class InvalidArgException : public Exception
{
public:
    virtual const char* preamble() const throw() {return "Invalid argument";}

    InvalidArgException(const char* argMessage) : Exception("Invalid argument", argMessage) {}
    InvalidArgException(const char* arg, const char* argMessage) : Exception("Invalid argument", arg, argMessage) {}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter) : Exception("Invalid argument", arg, argMessage, argMessageParameter) {}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter1, const int argMessageParameter2) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d)", "Invalid argument", arg, argMessage, argMessageParameter1, argMessageParameter2);}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter1, const int argMessageParameter2, const int argMessageParameter3) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d,%d)", "Invalid argument", arg, argMessage, argMessageParameter1, argMessageParameter2, argMessageParameter3);}

    template <typename T> InvalidArgException(T format, ...) : Exception(format, (va_start(_va, format), _va)) { va_end(_va);}

private:
    va_list _va;
};

class IOException : public Exception
{
public:
    IOException(const std::string message) : Exception("IO exception", message.c_str()) {}
    IOException(const char* message, const char* arg) : Exception("IO exception", message, arg) {}
    IOException(const char* message, const int arg) : Exception("IO exception", message, arg) {}
//    virtual ~IOException() throw() {}
};

class ZlibException : public Exception
{
public:
    ZlibException(const int errorNumber) : Exception("ZLib exception", errorNumber) {}
};

#define RL_ZLIB_EXCEPTION_CHECK(zlib_call) {int _zlib_ret_=zlib_call; if (_zlib_ret_ != Z_OK) throw robertslab::ZlibException(_zlib_ret_);}

}

#endif
