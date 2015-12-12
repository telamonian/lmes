import numpy as np
import zlib

from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayBuf

__all__ = ['Serializable']

class Serializable(object):
    BufType = None

    @staticmethod
    def bufConsistnecyCheck(buf):
        pass

    @staticmethod
    def deserializeArr(arrString, dtype, shape, compressed=True):
        if compressed:
            return np.reshape(np.fromstring(zlib.decompress(arrString), dtype=dtype), shape)
        else:
            return np.reshape(np.fromstring(arrString, dtype=dtype), shape)

    @staticmethod
    def deserializeArrFromBuf(arrBuf):
        dtype = Serializable.getNPDtypeFromBufDType(arrBuf.data_type)
        return Serializable.deserializeArr(arrString=arrBuf.data, dtype=dtype, shape=arrBuf.shape, compressed=arrBuf.compressed_deflate)

        # if bufArr.compressed_deflate:
        #     return np.reshape(np.fromstring(zlib.decompress(bufArr.data), dtype=dtype), bufArr.shape)
        # else:
        #     return np.reshape(np.fromstring(bufArr.data, dtype=dtype), bufArr.shape)

    @staticmethod
    def getBufDtypeFromNPDType(npDtype):
        return NDArrayBuf.__getattribute__(npDtype.name)

    @staticmethod
    def getNPDtypeFromBufDType(bufDtype):
        return np.dtype(NDArrayBuf.DataType.Name(bufDtype))

    @staticmethod
    def serializeArr(arr, compressed=True):
        if compressed:
            return zlib.compress(arr.tobytes())
        else:
            return arr.tobytes()

    @staticmethod
    def serializeArrToBuf(arr):       #, bufDtype=NDArrayBuf.float64):
        buf = NDArrayBuf()
        buf.data_type = Serializable.getBufDtypeFromNPDType(arr.dtype)
        buf.shape.extend(arr.shape)
        buf.data = Serializable.serializeArr(arr)

        return buf
