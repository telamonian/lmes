#!/usr/bin/env python3
import numpy as np
from six import print_
import sys
import zlib

from lm.io.SpeciesTimeSeries_pb2 import SpeciesTimeSeries as SpeciesTimeSeriesBuf
from lma.src.datum.trajectory import SpeciesTrajectories
from robertslab.sfile import *

np.set_printoptions(edgeitems=int(1e4), threshold=int(1e4), linewidth=int(1e3))

def DeserializeAsBuf(data):
    buf = SpeciesTimeSeriesBuf()
    buf.ParseFromString(data)
    
    if buf.counts.shape[0] == 0:
        species_counts = np.array([])
        times = np.array([])

    # Convert the data to a numpy array.
    if buf.counts.compressed_deflate:
        species_counts=np.reshape(np.fromstring(zlib.decompress(buf.counts.data), dtype=np.int32), buf.counts.shape)
    else:
        species_counts=np.reshape(np.fromstring(buf.counts.data, dtype=np.int32), buf.counts.shape)
    if buf.times.compressed_deflate:
        times=np.reshape(np.fromstring(zlib.decompress(buf.times.data), dtype=np.float64), buf.times.shape)
    else:
        times=np.reshape(np.fromstring(buf.times.data, dtype=np.float64), buf.times.shape)
    
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
