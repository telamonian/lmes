#!/usr/bin/env python3
from argparse import ArgumentParser
from google.protobuf.descriptor import FieldDescriptor
import numpy as np
from six import print_
import sys
import zlib

import lm
# from lm.io.SpeciesTimeSeries_pb2 import SpeciesTimeSeries as SpeciesTimeSeriesMsg
from lma.src.datum.trajectory import SpeciesTrajectories
from robertslab.sfile import *

np.set_printoptions(edgeitems=int(1e4), threshold=int(1e4), linewidth=int(1e3))

cppTypeDict = {getattr(FieldDescriptor, attrName):attrName for attrName in dir(FieldDescriptor) if attrName.startswith('CPPTYPE_')}
labelDict = {getattr(FieldDescriptor, attrName):attrName for attrName in dir(FieldDescriptor) if attrName.startswith('LABEL_')}

ndDtypeDict = {0: np.dtype('int8'),
               1: np.dtype('int16'),
               2: np.dtype('int32'),
               3: np.dtype('int64'),
               4: np.dtype('uint8'),
               5: np.dtype('uint16'),
               6: np.dtype('uint32'),
               7: np.dtype('uint64'),
               8: np.dtype('float16'),
               9: np.dtype('float32'),
              10: np.dtype('float64'),
              11: np.dtype('complex64'),
              12: np.dtype('complex128'),
              13: np.dtype('S8'),
              14: np.dtype('S16'),
              15: np.dtype('S32'),
              16: np.dtype('S64'),
              17: np.dtype('S128')}

def FieldIsMsg(fieldDesc):
    return GetFieldCPPType(fieldDesc)=='CPPTYPE_MESSAGE'

def GetFieldCPPType(fieldDesc):
    return cppTypeDict[fieldDesc.cpp_type]

def GetFieldLabel(fieldDesc):
    return labelDict[fieldDesc.label]

def DeserializeNDArrayAsMsg(ndarrayMsg):
    # Convert the data to a numpy array.
    if ndarrayMsg.compressed_deflate:
        nparray = np.reshape(np.fromstring(zlib.decompress(ndarrayMsg.data), dtype=ndDtypeDict[ndarrayMsg.data_type]), ndarrayMsg.shape)
    else:
        nparray = np.reshape(np.fromstring(ndarrayMsg.data, dtype=ndDtypeDict[ndarrayMsg.data_type]), ndarrayMsg.shape)

    return nparray

def DeserializeAsMsg(data, dataTypeFullName):
    msgType = lm.GetMsgType(dataTypeFullName)
    msg = msgType()

    msg.ParseFromString(data)

    return msg,msgType
    
    # if msg.counts.shape[0] == 0:
    #     species_counts = np.array([])
    #     times = np.array([])
    #
    # # Convert the data to a numpy array.
    # species_counts = DeserializeNDArrayAsMsg(msg.counts)
    # times = DeserializeNDArrayAsMsg(msg.times)
    # # if buf.counts.compressed_deflate:
    # #     species_counts=np.reshape(np.fromstring(zlib.decompress(buf.counts.data), dtype=np.int32), buf.counts.shape)
    # # else:
    # #     species_counts=np.reshape(np.fromstring(buf.counts.data, dtype=np.int32), buf.counts.shape)
    # # if buf.times.compressed_deflate:
    # #     times=np.reshape(np.fromstring(zlib.decompress(buf.times.data), dtype=np.float64), buf.times.shape)
    # # else:
    # #     times=np.reshape(np.fromstring(buf.times.data, dtype=np.float64), buf.times.shape)
    #
    # print_(times.astype)
    # print_(species_counts)

def DeserializeAsData(data, msgTypeFullName):
    specTrajs = SpeciesTrajectories()
    specTrajs.deserialize(data)
    for tid,traj in specTrajs.items():
        print_(tid)
        print_(traj.time)
        print_(traj.species_count)

def DumpRecord(record, data):
    print_(record)
    if data is not None:
        msg,msgType = DeserializeAsMsg(data, record.dataTypeFullName)
        PrintMsg(msg)

def PrintMsg(msg):
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
                if desc.message_type.full_name=='robertslab.pbuf.NDArray':
                    # val is a ndarray msg
                    print_(DeserializeNDArrayAsMsg(val))
                else:
                    # val is any other kind of msg
                    PrintMsg(val)
            else:
                # val is a single pod
                print_(desc.name, ': ', val)

def Main():
    parser = ArgumentParser()   #"Usage: ./dumpSFileLM.py path-to-sfile [-l]")

    parser.add_argument('sfilePath',  help='path to sfile to dump')
    parser.add_argument('-l', '--list-only', action='store_true', help='if the --list-only flag is set, dump only the record metadata without the actual record data')

    kwargs = vars(parser.parse_args())

    f = SFile.fromFilename(sys.argv[1], 'rb')

    # loop over all of the records, printing out either the metadata, or the metadata and the deserialized data
    if kwargs['list_only']:
        for record in f.records():
            print_(record)
    else:
        for record,data in f.items():
            DumpRecord(record, data)

    f.close()

if __name__=='__main__':
    Main()
