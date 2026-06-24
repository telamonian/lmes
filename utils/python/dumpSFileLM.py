#!/usr/bin/env python3
from argparse import ArgumentParser, SUPPRESS
from gettext import gettext
from google.protobuf.descriptor import FieldDescriptor
import numpy as np
from pathlib import Path
import re
from six import print_
import sys
import zlib

import lm
from lm.sfileLM import SFileLM
# from lma.src.datum.trajectory import SpeciesTrajectories
from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayMsg

np.set_printoptions(edgeitems=int(1e4), threshold=int(1e4), linewidth=int(1e3))

#### Helper functions for easier access to Protobuf reflection

cppTypeDict = {getattr(FieldDescriptor, attrName):attrName for attrName in dir(FieldDescriptor) if attrName.startswith('CPPTYPE_')}
labelDict = {getattr(FieldDescriptor, attrName):attrName for attrName in dir(FieldDescriptor) if attrName.startswith('LABEL_')}

def FieldIsMsg(fieldDesc):
    return GetFieldCPPType(fieldDesc)=='CPPTYPE_MESSAGE'

def GetFieldCPPType(fieldDesc):
    return cppTypeDict[fieldDesc.cpp_type]

def GetFieldLabel(fieldDesc):
    return labelDict[fieldDesc.label]

def GetNDArrayDataType(ndarrayMsg):
    return NDArrayMsg.DataType.Name(ndarrayMsg.data_type)

#### Functions for deserializing data into protobuf messages

def DeserializeNDArrayAsMsg(ndarrayMsg):
    # Convert the data to a numpy array.
    if ndarrayMsg.compressed_deflate:
        nparray = np.reshape(np.fromstring(zlib.decompress(ndarrayMsg.data), dtype=GetNDArrayDataType(ndarrayMsg)), ndarrayMsg.shape)
    else:
        nparray = np.reshape(np.fromstring(ndarrayMsg.data, dtype=GetNDArrayDataType(ndarrayMsg)), ndarrayMsg.shape)

    return nparray

def DeserializeAsMsg(data, dataTypeFullName):
    msgType = lm.GetMsgType(dataTypeFullName)
    msg = msgType()

    msg.ParseFromString(data)

    return msg,msgType

# def GetMsgType(dataTypeFullName):


#### Printing functions

def PrintRecord(record, data, listOnly=False):
    print_(record)
    if not listOnly:
        msg,msgType = DeserializeAsMsg(data, record.dataTypeSuffix)
        PrintMsg(msg)

def PrintMsg(msg):
    ''' This function recursively walks over/prints the fields of a Protobuf message instance.
    If any fields are themselves messages, PrintMsg is recursively called on said submessage.

    Unlike the built-in message __print__() method, PrintMsg correctly unpacks the multidimensional arrays in NDArray messages.

    :param msg: The message to be walk over/printed
    :return:
    '''
    for desc,val in msg.ListFields():
        if GetFieldLabel(desc)=='LABEL_REPEATED':
            if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
                for i,subMsg in enumerate(val):
                    # val is repeated message
                    print_('%s_%d: ' % (desc.name, i), end='')
                    PrintMsg(subMsg)
            else:
                # val is repeated pod
                print_(desc.name, ': ', list(val))
        else:
            if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
                print_(desc.name, ': ', end='')
                if desc.message_type.name=='NDArray':
                    # val is a ndarray msg
                    print_(DeserializeNDArrayAsMsg(val))
                else:
                    # val is any other kind of msg
                    PrintMsg(val)
            else:
                # val is a single pod
                print_(desc.name, ': ', val)

#### other

def Include(name, res):
    for re in res:
        if re.search(name):
            return True
    return False

def GetItems(f, doSort=False, includeRes=None):
    """This function returns a generator expression that produces the appropriate record,data pairs
    Useful in avoiding having to hold the entire contents of an sfile in memory at any point

    :doSort: if True, sort the records before returning them (this will convert the genexp to a standard sequence)
    :includeRes: an optional list of compiled regexes against which desired record names can be matched
    :return: (record,data) pairs genexp
    """
    if includeRes is not None:
        items = ((record,data) for record,data in f.items() if Include(record.name, includeRes))
    else:
        items = f.items()

    if doSort:
        return sorted(items)
    else:
        return items

#### Main function

def Main():
    class ArgumentParserCustomError(ArgumentParser):
        def error(self, message):
            """error(message: string)
            An override of the default error handler that includes some more helpful/specific messages
            """
            self.print_help(sys.stderr)

            if message.count('the following arguments are required: sfilePath'):
                message += '\nBe careful not to put the path to the sfile after the -i flag, as it may get confused for a search pattern in this case'

            args = {'prog': self.prog, 'message': message}
            self.exit(2, gettext('%(prog)s: error: %(message)s\n') % args)

    parser = ArgumentParserCustomError()   #"Usage: ./dumpSFileLM.py path-to-sfile [-l]")

    parser.add_argument('sfilePath',                              help='path to sfile to dump')
    parser.add_argument('-i', '--include', nargs='+',             help='only show data from records that match (one of) the given regex pattern(s)')
    parser.add_argument('-l', '--list-only', action='store_true', help='if the --list-only flag is set, dump only the record metadata without the actual record data')
    parser.add_argument('-r', '--repack',                         help='repack the sfile, creating a new sfile at the provided path. If the --include argument is also used, the repacked sfile will only include the matching records')
    parser.add_argument('-s', '--sort', action='store_true',      help='if set, sort the records before outputting them')
    parser.add_argument('-v', '--verbose', action='store_true')

    kwargs = vars(parser.parse_args())

    verbose = kwargs['verbose'] or kwargs['repack'] is None

    f = SFileLM.fromFilename(kwargs['sfilePath'])

    if kwargs['include'] is not None:
        includeRes = [re.compile(pattern) for pattern in kwargs['include']]
    else:
        includeRes = None

    # items can only be iterated through once without reopening/resetting f
    items = GetItems(f, doSort=kwargs['sort'], includeRes=includeRes)

    # loop over all of the records
    if kwargs['repack'] is None:
        # just print contents
        for record,data in items:
            PrintRecord(record=record, data=data, listOnly=kwargs['list_only'])
    else:
        repacked = SFileLM.fromFilename(kwargs['repack'], mode='w')
        if kwargs['verbose']:
            # print contents and repack file
            for record,data in items:
                PrintRecord(record=record, data=data, listOnly=kwargs['list_only'])
                repacked.writeRecord(name=record.name, dataType=record.dataType, stringData=data)
        else:
            # just repack file
            for record,data in items:
                repacked.writeRecord(name=record.name, dataType=record.dataType, stringData=data)
        repacked.close()

    f.close()

if __name__=='__main__':
    Main()
