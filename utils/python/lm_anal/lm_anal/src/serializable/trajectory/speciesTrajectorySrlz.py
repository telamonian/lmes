import numpy as np
import zlib

from lm_anal.src.serializable.serializable import Serializable
from lm.io.SpeciesTimeSeries_pb2 import SpeciesTimeSeries as SpeciesTimeSeriesBuf
from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayBuf

__all__ = ['SpeciesTrajectoriesSrlz']

class SpeciesTrajectoriesSrlz(Serializable):
    @staticmethod
    def bufConsistencyCheck(buf):
        # Make sure the data is consistent.
        if len(buf.counts.shape) != 2 or len(buf.times.shape) != 1:
            raise ValueError("Invalid array shape.")
        if buf.counts.shape[0] != buf.times.shape[0]:
            raise ValueError("Inconsistent array sizes.")
        if buf.counts.data_type != NDArrayBuf.int32 or buf.times.data_type != NDArrayBuf.float64:
            raise TypeError("Invalid array data types.")

    def deserialize(self, data, full=True, trajID=None):
        # Deserialize the data.
        buf = SpeciesTimeSeriesBuf()
        buf.ParseFromString(data)
        self.bufConsistnecyCheck(buf)

        # get the trajectoryID
        if trajID is None:
            trajID = buf.trajectory_id
        elif trajID=='uuid':
            trajID = np.random.randint(1e7, 1e8, 1)[0]
        
        # initialize a new Datum from the container
        subCon = self.initDatum(key=trajID, full=full)
        
        # set .trajectory_id
        subCon.setScalar(name='trajectory_id', val=trajID)
        
        # Convert the serialized NDArray data in the buf to proper numpy arrays
        subCon.catArray(name='species_count', val=self.deserializeArrFromBuf(buf.counts))
        subCon.catArray(name='time', val=self.deserializeArrFromBuf(buf.times))

    def serialize(self, keys=None):
        container = self if keys is None else self.sliceByKeys(keys)

        serializedBufs = []
        for subCon in container.values():
            buf = SpeciesTimeSeriesBuf()
            buf.trajectory_id = subCon.trajectory_id

            buf.counts = self.serializeArrToBuf(subCon.species_count)    #, bufDType=NDArrayBuf.int32)
            buf.times = self.serializeArrToBuf(subCon.time)     #, bufDType=NDArrayBuf.float64)

            serializedBufs.append(buf.SerializeToString())
        return serializedBufs