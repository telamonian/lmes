import numpy as np
import zlib

from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayBuf
from lm.io.SpeciesTimeSeries_pb2 import SpeciesTimeSeries as SpeciesTimeSeriesBuf

__all__ = ['SpeciesTrajectoriesSrlz']

class SpeciesTrajectoriesSrlz(object):
    def deserialize(self, data, full=True):
        # Deserialize the data.
        buf = SpeciesTimeSeriesBuf()
        buf.ParseFromString(str(data))

        # Make sure the data is consistent.
        if len(buf.counts.shape) != 2 or len(buf.times.shape) != 1:
            raise ValueError("Invalid array shape.")
        if buf.counts.shape[0] != buf.times.shape[0]:
            raise ValueError("Inconsistent array sizes.")
        if buf.counts.data_type != NDArrayBuf.int32 or buf.times.data_type != NDArrayBuf.float64:
            raise TypeError("Invalid array data types.")
        
        # get the trajectoryID
        trajID = buf.trajectory_id
        
        # initialize a new Datum from the container
        subCon = self.initDatum(key=trajID, full=full)
        
        # set .trajectory_id
        subCon.setScalar(name='trajectory_id', val=trajID)
        
        # Convert the serialized NDArray data in the buf to proper numpy arrays
        subCon.setArray(name='species_count', val=self.deserializeArr(buf.counts, dtype=np.int32))
        subCon.setArray(name='time', val=self.deserializeArr(buf.times, dtype=np.float64))

        # if buf.counts.compressed_deflate:
        #     np.reshape(np.fromstring(zlib.decompress(buf.counts.data), dtype=np.int32), buf.counts.shape))
        # else:
        #     subCon.setArray(name='species_count',
        #                     val=np.reshape(np.fromstring(buf.counts.data, dtype=np.int32), buf.counts.shape))
        # if buf.times.compressed_deflate:
        #     subCon.setArray(name='time',
        #                     val=np.reshape(np.fromstring(zlib.decompress(buf.times.data), dtype=np.float64), buf.times.shape))
        # else:
        #     subCon.setArray(name='time',
        #                     val=np.reshape(np.fromstring(buf.times.data, dtype=np.float64), buf.times.shape))

    def serialize(self, keys=None):
        container = self if keys is None else self.sliceByKeys(keys)

        bufs = []
        for subCon in container.values():
            buf = SpeciesTimeSeriesBuf()
            buf.trajectory_id = subCon.trajectory_id

            buf.counts = self.serializeArr(subCon.species_count, bufDType=NDArrayBuf.int32)
            buf.times = self.serializeArr(subCon.time, bufDType=NDArrayBuf.float64)

            bufs.append(buf)
        return bufs

    @staticmethod
    def deserializeArr(bufArr, dtype):
        if bufArr.compressed_deflate:
            return np.reshape(np.fromstring(zlib.decompress(bufArr.data), dtype=dtype), bufArr.shape)
        else:
            return np.reshape(np.fromstring(bufArr.data, dtype=dtype), bufArr.shape)

    @staticmethod
    def serializeArr(arr, bufDtype):
        buf = NDArrayBuf()
        buf.data_type = bufDtype #NDArrayBuf.float32
        buf.shape.extend(arr.shape)
        buf.data = arr.tobytes()

        return buf.SerializeToString()