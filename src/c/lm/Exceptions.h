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

class Exception : public std::exception
{
protected:
    static const int MAX_MESSAGE_SIZE = 1025;
    char messageBuffer[MAX_MESSAGE_SIZE];
    
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
	virtual ~Exception() throw() {}
	virtual const char * what() const throw() {return messageBuffer;}
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
	ConsistencyException(const char * format, ...): Exception()
	{
		int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s: ", "Consistency exception");
		va_list args;
		va_start (args, format);
		vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, format, args);
		va_end (args);
	}
};

class InputException : public Exception
{
public:
	InputException(const char* format, ...): Exception()
	{
		int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s: ", "Input exception");
		va_list args;
		va_start (args, format);
		vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, format, args);
		va_end (args);
	}

    InputException(const char *file, int line, const char* format, ...): Exception()
    {
        int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s (%s:%d): ", "Input exception", file, line);
        va_list args;
        va_start (args, format);
        vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, format, args);
        va_end (args);
    }
};
#define THROW_LINE(eckception, arg) throw eckception(__FILE__, __LINE__, arg);

class InvalidArgException : public Exception
{
public:
	InvalidArgException(const char* argMessage) : Exception("Invalid argument", argMessage) {}
	InvalidArgException(const char* arg, const char* argMessage) : Exception("Invalid argument", arg, argMessage) {}
    InvalidArgException(const char* arg, const char* argMessage, const char * argMessageParameter) : Exception("Invalid argument", arg, argMessage, argMessageParameter) {}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter) : Exception("Invalid argument", arg, argMessage, argMessageParameter) {}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter1, const int argMessageParameter2) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d)", "Invalid argument", arg, argMessage, argMessageParameter1, argMessageParameter2);}
    InvalidArgException(const char* arg, const char* argMessage, const int argMessageParameter1, const int argMessageParameter2, const int argMessageParameter3) : Exception() {snprintf(messageBuffer,MAX_MESSAGE_SIZE,"%s: %s, %s (%d,%d,%d)", "Invalid argument", arg, argMessage, argMessageParameter1, argMessageParameter2, argMessageParameter3);}
//    virtual ~InvalidArgException() throw() {}
};

class IOException : public Exception
{
public:
    IOException(const std::string message) : Exception("IO exception", message.c_str()) {}
    IOException(const char* message, const char* arg) : Exception("IO exception", message, arg) {}
    IOException(const char* message, const int arg) : Exception("IO exception", message, arg) {}
//    virtual ~IOException() throw() {}
};

class NotFoundException : public Exception
{
public:
	NotFoundException(const char * format, ...): Exception()
	{
		int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s: ", "NotFound exception");
		va_list args;
		va_start (args, format);
	    vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, format, args);
		va_end (args);
	}
};

class NullPointerException : public Exception
{
public:
    NullPointerException(const char* format, ...): Exception()
    {
        int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s: ", "Exception-> attempted to derefrence a pointer to NULL");
        va_list args;
        va_start (args, format);
        vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, format, args);
        va_end (args);
    }
};


class UnimplementedException : public Exception
{
public:
    UnimplementedException(const char * format, ...): Exception()
    {
        int offset = snprintf(messageBuffer, MAX_MESSAGE_SIZE, "%s: ", "Unimplemented exception");
        va_list args;
        va_start (args, format);
        vsnprintf(messageBuffer + offset, MAX_MESSAGE_SIZE - offset, format, args);
        va_end (args);
    }
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
