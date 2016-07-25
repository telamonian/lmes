#!/usr/bin/env python3
import numpy as np
from six import print_
import sys
import zlib

from lm.io.SpeciesTimeSeries_pb2 import SpeciesTimeSeries as SpeciesTimeSeriesMsg
from lma.src.datum.trajectory import SpeciesTrajectories
from robertslab.sfile import *

np.set_printoptions(edgeitems=int(1e4), threshold=int(1e4), linewidth=int(1e3))

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
              13: np.dtype('S8 '),
              14: np.dtype('S16'),
              15: np.dtype('S32'),
              16: np.dtype('S64'),
              17: np.dtype('S128')}

def DeserializeNDArrayAsMsg(ndarrayMsg):
    # Convert the data to a numpy array.
    if ndarrayMsg.compressed_deflate:
        nparray = np.reshape(np.fromstring(zlib.decompress(ndarrayMsg.data), dtype=ndDtypeDict[ndarrayMsg.data_type]), ndarrayMsg.shape)
    else:
        nparray = np.reshape(np.fromstring(ndarrayMsg.data, dtype=ndDtypeDict[ndarrayMsg.data_type]), ndarrayMsg.shape)

    return nparray

def DeserializeAsMsg(data):
    msg = SpeciesTimeSeriesMsg()
    msg.ParseFromString(data)
    
    if msg.counts.shape[0] == 0:
        species_counts = np.array([])
        times = np.array([])

    # Convert the data to a numpy array.
    species_counts = DeserializeNDArrayAsMsg(msg.counts)
    times = DeserializeNDArrayAsMsg(msg.times)
    # if buf.counts.compressed_deflate:
    #     species_counts=np.reshape(np.fromstring(zlib.decompress(buf.counts.data), dtype=np.int32), buf.counts.shape)
    # else:
    #     species_counts=np.reshape(np.fromstring(buf.counts.data, dtype=np.int32), buf.counts.shape)
    # if buf.times.compressed_deflate:
    #     times=np.reshape(np.fromstring(zlib.decompress(buf.times.data), dtype=np.float64), buf.times.shape)
    # else:
    #     times=np.reshape(np.fromstring(buf.times.data, dtype=np.float64), buf.times.shape)
    
    print_(times.astype)
    print_(species_counts)

def DeserializeAsData(data):
    specTrajs = SpeciesTrajectories()
    specTrajs.deserialize(data)
    for tid,traj in specTrajs.items():
        print(tid)
        print(traj.time)
        print(traj.species_count)

def Main():
    # Make sure we have the correct command line arguments.
    if len(sys.argv) < 2:
        print_("Usage: ./dumpSFileLM.py <path-to-.sfile>")
        quit()
    
    f = SFile.fromFilename(sys.argv[1], 'rb')
    
    # Open the file.
    while True:
        r = f.readNextRecord()
        if r==None:
            break
        print_(r)
        data = f.readDataRaw(r.dataSize)
        DeserializeAsData(data)
    f.close()

if __name__=='__main__':
    Main()
