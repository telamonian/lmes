from google.protobuf.descriptor import FieldDescriptor
import numpy as np
import zlib

import lm
from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayMsg
from robertslab.sfile import SFileRecordSeekable, SFileSeekable

__all__=['SFileLM']

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

def DeserializeNDArray(ndarrayMsg):
    # Convert the data to a numpy array.
    if ndarrayMsg.compressed_deflate:
        nparray = np.reshape(np.fromstring(zlib.decompress(ndarrayMsg.data), dtype=GetNDArrayDataType(ndarrayMsg)), ndarrayMsg.shape)
    else:
        nparray = np.reshape(np.fromstring(ndarrayMsg.data, dtype=GetNDArrayDataType(ndarrayMsg)), ndarrayMsg.shape)

    return nparray

def UnpackNDArray(msg, recursive=True):
    for desc,val in msg.ListFields():
        if GetFieldLabel(desc)=='LABEL_REPEATED':
            if desc.message_type.full_name=='robertslab.pbuf.NDArray':
                nparrays = []
                for ndarray in val:
                    nparrays.append(DeserializeNDArray(ndarray))
                val.__setattr__(desc.name, nparrays)
            elif GetFieldCPPType(desc)=='CPPTYPE_MESSAGE' and recursive:
                for subMsg in val:
                    UnpackNDArray(msg=subMsg, recursive=recursive)
        elif desc.message_type.full_name=='robertslab.pbuf.NDArray':
            val.__setattr__(desc.name, DeserializeNDArray(val))
        elif GetFieldCPPType(desc)=='CPPTYPE_MESSAGE' and recursive:
            UnpackNDArray(msg=val, recursive=recursive)

class SFileRecordLM(SFileRecordSeekable):
    @property
    def msgType(self):
        return lm.GetMsgType(self.dataTypeFullName)

    def msg(self, unpackNDArray=True):
        msg = self.msgType()
        msg.ParseFromString(self.readData())

        if unpackNDArray:
            UnpackNDArray(msg)

        return msg

class SFileLM(SFileSeekable):
    recordType = SFileRecordLM

    def msgs(self, unpackNDArray=True):
        for record,msg in self.items(unpackNDArray=unpackNDArray):
            yield msg

    def items(self, unpackNDArray=True):
        while True:
            record = self.readNextRecord()
            if record is None:
                break

            yield record,record.msg()
