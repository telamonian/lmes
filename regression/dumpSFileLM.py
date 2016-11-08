#!/usr/bin/env python3
from argparse import ArgumentParser, SUPPRESS
from google.protobuf.descriptor import FieldDescriptor
import numpy as np
import re
from six import print_
import sys
import zlib

import lm
from lm.sfileLM import SFileLM
# from lma.src.datum.trajectory import SpeciesTrajectories
from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayMsg

np.set_printoptions(edgeitems=int(1e4), threshold=int(1e4), linewidth=int(1e3))

# Helper functions for easier access to Protobuf reflection

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

# Functions for deserializing data into protobuf messages

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

# def DeserializeAsLMAData(data, msgTypeFullName):
#     specTrajs = SpeciesTrajectories()
#     specTrajs.deserialize(data)
#     for tid,traj in specTrajs.items():
#         print_(tid)
#         print_(traj.time)
#         print_(traj.species_count)

# Printing functions

def PrintRecord(record, data=None):
    print_(record)
    if data is not None:
        msg,msgType = DeserializeAsMsg(data, record.dataTypeSuffix)
        PrintMsg(msg)

def PrintRecordIfInclude(includeRe, record, data=None):
    if includeRe.search(record.name):
        PrintRecord(record=record, data=data)

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

# Main function

def Main():
    parser = ArgumentParser()   #"Usage: ./dumpSFileLM.py path-to-sfile [-l]")

    parser.add_argument('sfilePath',                              help='path to sfile to dump')
    parser.add_argument('-i', '--include', default=SUPPRESS,      help='only show data from records that match the given regex pattern')
    parser.add_argument('-l', '--list-only', action='store_true', help='if the --list-only flag is set, dump only the record metadata without the actual record data')
    parser.add_argument('-s', '--sort', action='store_true',      help='if set, sort the records before outputting them')

    kwargs = vars(parser.parse_args())

    f = SFileLM.fromFilename(kwargs['sfilePath'])

    if 'include' in kwargs:
        includeRe = re.compile(kwargs['include'])
    else:
        includeRe = None

    items = sorted(f.items()) if kwargs['sort'] else f.items()

    # loop over all of the records, printing out either the metadata, or the metadata and the deserialized data
    if kwargs['list_only']:
        if includeRe is not None:
            for record,data in items: PrintRecordIfInclude(includeRe=includeRe, record=record)
        else:
            for record,data in items: PrintRecord(record=record)
    else:
        if includeRe is not None:
            for record,data in items: PrintRecordIfInclude(includeRe=includeRe, record=record, data=data)
        else:
            for record,data in items: PrintRecord(record=record, data=data)

    f.close()

if __name__=='__main__':
    Main()
