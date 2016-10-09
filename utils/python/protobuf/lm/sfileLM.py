from __future__ import division

from collections import OrderedDict, defaultdict
from google.protobuf.descriptor import FieldDescriptor
try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest
import numpy as np
import zlib

import lm
from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayMsg
from robertslab.sfile import SFileRecordSeekable, SFileSeekable

__all__=['SFileLM']

# Definition of a combined default and ordered dict
# class OrderedDefaultDict(OrderedDict, defaultdict):
#     def __init__(self, default_factory=None, *args, **kwargs):
#         super(OrderedDefaultDict, self).__init__(*args, **kwargs)
#         self.default_factory = default_factory

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

def DecompressNDArrayData(ndarrayMsg):
    # return the data, decompressing if necessary
    if ndarrayMsg.compressed_deflate:
        return zlib.decompress(ndarrayMsg.data)
    else:
        return bytes(ndarrayMsg.data)

def DecompressMergedNDArrayData(ndarrayMsg):
    # return the data, decompressing if necessary
    if ndarrayMsg.compressed_deflate:
        unused_data = ndarrayMsg.data

        data = b''
        while unused_data:
            unzipper = zlib.decompressobj()
            data += unzipper.decompress(unused_data)
            from IPython.core.debugger import Tracer; Tracer()()
            unused_data = unzipper.unused_data

        return data
    else:
        return bytes(ndarrayMsg.data)

def DeserializeNDArray(ndarrayMsg, count=1):
    if count > 1:
        rank = len(ndarrayMsg.shape)//count
        shape = [sum(ndarrayMsg.shape[::rank])] + ndarrayMsg.shape[1:rank]
        data = DecompressMergedNDArrayData(ndarrayMsg)
    else:
        shape = ndarrayMsg.shape
        data = DecompressNDArrayData(ndarrayMsg)

    # Convert the data to a numpy array.
    return np.reshape(np.fromstring(data, dtype=GetNDArrayDataType(ndarrayMsg)), shape)

def DeserializeNDArrays(ndarrayMsgs):
    # initialize the composite's properties based on the first ndarrayMsg
    shape = list(ndarrayMsgs[0].shape)
    dtype = GetNDArrayDataType(ndarrayMsgs[0])
    data = DecompressNDArrayData(ndarrayMsgs[0])

    for ndarrayMsg in ndarrayMsgs[1:]:
        # extend the composite's shape by adding together row counts
        shape[0] += ndarrayMsg.shape[0]

        # extend the data
        data += DecompressNDArrayData(ndarrayMsg)

    return np.reshape(np.fromstring(data, dtype=dtype), shape)

def UnpackNDArray(msg, count=1, recursive=True):
    ''' simple function for unpacking any NDArray fields in a protobuf msg into standard numpy arrays

    :param msg: any protobuf msg
    :param recursive: flag that controls whether the function descends into any subMsgs
    :return: the input msg, with additional attributes in which the unpacked numpy arrays are stored
             for every NDArray, the function creates an attribute 'field_name' + '_np'
    '''
    for desc,val in msg.ListFields():
        if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
            if desc.message_type.full_name=='robertslab.pbuf.NDArray':
                if GetFieldLabel(desc)=='LABEL_REPEATED':
                    nparrays = []
                    for ndarray in val:
                        nparrays.append(DeserializeNDArray(ndarray, count=count))
                    # msg.__setattr__(desc.name + '_np', nparrays)
                    msg.__dict__['%s_np' % desc.name] = nparrays
                else:
                    # msg.__setattr__(desc.name + '_np', DeserializeNDArray(val))
                    msg.__dict__['%s_np' % desc.name] = DeserializeNDArray(val, count=count)
            elif recursive:
                if GetFieldLabel(desc)=='LABEL_REPEATED':
                    for subMsg in val:
                        UnpackNDArray(msg=subMsg, count=count, recursive=recursive)
                else:
                    UnpackNDArray(msg=val, count=count, recursive=recursive)
    return msg

def MergeNDArrays(msgs, retDict=None, prefix='', recursive=True):
    if len(msgs) < 1:
        return None

    if retDict is None:
        retDict = msgs[0].__dict__['nparrays'] = {}

    for desc in msgs[0].DESCRIPTOR.fields:
        if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
            if desc.message_type.full_name=='robertslab.pbuf.NDArray':
                if GetFieldLabel(desc)=='LABEL_REPEATED':
                    retDict[prefix + desc.name] = [DeserializeNDArrays(ndarrays) for ndarrays in zip_longest([getattr(msg, desc.name) for msg in msgs])]
                else:
                    retDict[prefix + desc.name] = DeserializeNDArrays([getattr(msg, desc.name) for msg in msgs])
            elif recursive:
                if GetFieldLabel(desc)=='LABEL_REPEATED':
                    MergeNDArrays(msgs=[subMsg for msg in msgs for subMsg in getattr(msg, desc.name)], retDict=retDict, prefix=desc.name + '.', recursive=recursive)
                else:
                    MergeNDArrays(msgs=[getattr(msg, desc.name) for msg in msgs], retDict=retDict, prefix=desc.name + '.', recursive=recursive)
    return msgs[0]

class SFileRecordLM(SFileRecordSeekable):
    def combine(self, others):
        pass

    @property
    def msgType(self):
        # if .dataTypeSuffix is not a known protobuf type, this will raise a KeyError
        return lm.GetMsgType(self.dataTypeSuffix)

    def msg(self, unpackNDArray=True):
        msg = self.msgType()
        msg.ParseFromString(self.readData())

        if unpackNDArray:
            UnpackNDArray(msg)

        return msg

class SFileLM(SFileSeekable):
    @staticmethod
    def combineRecords(records):
        if len(records)==0:
            return None

        # deserialize the records without unpacking the ndarrays
        msgs = [record.msg(unpackNDArray=False) for record in records]

        # merge all of the ndarrays
        msg = MergeNDArrays(msgs)

        return msg

    # more efficient version that fails because of how protobuf merges bytes fields
    #
    # @staticmethod
    # def combineRecords(records):
    #     if len(records)==0:
    #         return None
    #
    #     # deserialize the first record
    #     msg = records[0].msg(unpackNDArray=False)
    #
    #     # merge in the rest of the records
    #     for record in records[1:]:
    #         msg.MergeFromString(record.readData())
    #
    #     UnpackNDArray(msg, count=len(records))
    #
    #     return msg

    recordType = SFileRecordLM

    def msgs(self, unpackNDArray=True):
        for record,msg in self.items(unpackNDArray=unpackNDArray):
            yield msg

    def items(self, unpackNDArray=True):
        while True:
            record = self.readNextRecord()
            if record is None:
                break

            try:
                # if the record describes a known protobuf type, deserialize and return it
                yield record,record.msg(unpackNDArray=unpackNDArray)
            except KeyError:
                # if the record did not describe a known protobuf type, just return the raw data
                yield record,self.readData(record.dataSize)


    @property
    def recordDict(self):
        try:
            return self._recordDict
        except AttributeError:
            self._recordDict = self._genRecordDictCombined()
            return self._recordDict

    def _genRecordDict(self):
        self.reset()

        recordDict = {}
        for record in self.records():
            try:
                recordDict[record.dataTypeSuffix][record.name].append(record)
            except KeyError:
                try:
                    recordDict[record.dataTypeSuffix][record.name] = [record]
                except KeyError:
                    recordDict[record.dataTypeSuffix] = OrderedDict(((record.name, [record]),))
        return recordDict

    def _genRecordDictCombined(self):
        self.reset()

        recordDict = self._genRecordDict()

        for typeDict in recordDict.values():
            for name,records in typeDict.items():
                typeDict[name] = self.combineRecords(records)
        return recordDict