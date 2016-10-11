from __future__ import division

from collections import OrderedDict
from google.protobuf.descriptor import FieldDescriptor
from itertools import chain
try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest
import numpy as np
import zlib

import lm
from robertslab.pbuf.NDArray_pb2 import NDArray as NDArrayMsg
from robertslab.sfile import SFileRecordSeekable, SFileSeekable

__all__ = ['SFileLM', 'Deserialize', 'DeserializeAndMerge']

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

def GetFieldNumpyType(fieldDesc):
    ''' figures out the closest equivalent numpy dtype for any protobuf field

    :param fieldDesc: the field descriptor we want to get the numpy type for
    :return: a numpy dtype
    '''
    cppType = GetFieldCPPType(fieldDesc).split('_')[1]

    if cppType=='MESSAGE' or cppType=='STRING':
        return np.dtype('O')
    elif cppType=='ENUM':
        return np.dtype('int32')
    else:
        return np.dtype(cppType.lower())

def GetNDArrayDataType(ndarrayMsg):
    return NDArrayMsg.DataType.Name(ndarrayMsg.data_type)

# Functions for deserializing data into protobuf messages

def DecompressNDArrayData(ndarrayMsg):
    '''Decompresses the .data field in a standard NDArray message

    :param ndarrayMsg: an NDArray message instance
    :return: ndarrayMsg.data, decompressed by zlib if necessary
    '''

    # return the data, decompressing if necessary
    if ndarrayMsg.compressed_deflate:
        return zlib.decompress(ndarrayMsg.data)
    else:
        return bytes(ndarrayMsg.data)

def DecompressMergedNDArrayData(ndarrayMsg):
    '''Decompresses the .data field in an NDArray message when .data is the result of merging .data from several NDArray message instances

    :param ndarrayMsg: an NDArray message instance with a merged .data field
    :return: ndarrayMsg.data, decompressed by zlib if necessary
    '''

    # return the data, decompressing if necessary
    if ndarrayMsg.compressed_deflate:
        unused_data = ndarrayMsg.data

        data = b''
        while unused_data:
            unzipper = zlib.decompressobj()
            data += unzipper.decompress(unused_data)
            unused_data = unzipper.unused_data

        return data
    else:
        return bytes(ndarrayMsg.data)

def UnpackNDArray(ndarrayMsg, count=1):
    '''Unpacks a single NDArray message into a single numpy array

    :param ndarrayMsg: an NDArray message instance
    :param count: if ndarrayMsg (specifically its .shape and .data fields) is the result of merging several NDArray messages, indicate how many with this parameter
    :return: a numpy array
    '''

    if count > 1:
        rank = len(ndarrayMsg.shape)//count
        shape = [sum(ndarrayMsg.shape[::rank])] + ndarrayMsg.shape[1:rank]
        data = DecompressMergedNDArrayData(ndarrayMsg)
    else:
        shape = ndarrayMsg.shape
        data = DecompressNDArrayData(ndarrayMsg)

    # Convert the data to a numpy array.
    return np.reshape(np.fromstring(data, dtype=GetNDArrayDataType(ndarrayMsg)), shape)

def UnpackAndMergeNDArrays(ndarrayMsgs):
    '''Merges an iterable of NDArray messages into a single numpy array

    :param ndarrayMsgs: the NDArray instances to be merged
    :return: a numpy array
    '''

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

def UnpackMsg(msg, count=1, recursive=True, _prefix='', _retDict=None):
    '''Simple function for unpacking any NDArray fields in a protobuf msg into standard numpy arrays

    :param msg: any protobuf msg
    :param recursive: flag that controls whether the function descends into any subMsgs
    :return: the input msg. msg.nparrays contains a dict with the unpacked numpy version of any NDArrays that were found
    '''
    if _retDict is None:
        _retDict = msg.__dict__['nparrays'] = {}

    for desc,val in msg.ListFields():
        if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
            if desc.message_type.name=='NDArray':
                if GetFieldLabel(desc)=='LABEL_REPEATED':
                    nparrays = []
                    for ndarray in val:
                        nparrays.append(UnpackNDArray(ndarray, count=count))
                    _retDict[_prefix + desc.name] = nparrays
                else:
                    _retDict[_prefix + desc.name] = UnpackNDArray(val, count=count)
            elif recursive:
                if GetFieldLabel(desc)=='LABEL_REPEATED':
                    for subMsg in val:
                        UnpackMsg(msg=subMsg, count=count, recursive=recursive, _retDict=_retDict, _prefix=desc.name + '.',)
                else:
                    UnpackMsg(msg=val, count=count, recursive=recursive, _retDict=_retDict, _prefix=desc.name + '.',)
    return msg

def UnpackAndMergeMsgs(msgs, recursive=True, _prefix='', _retDict=None):
    '''An NDArray aware implementation of the standard protobuf merging function

    :param msgs: the protobuf messages to merge
    :param recursive: whether to descend into subMsgs and merge those too
    :return: the merged message. msg.nparrays contains a dict with the unpacked numpy version of any NDArrays that were found
    '''
    if len(msgs) < 1:
        return None

    if _retDict is None:
        _retDict = msgs[0].__dict__['nparrays'] = {}

    for desc in msgs[0].DESCRIPTOR.fields:
        if GetFieldLabel(desc)=='LABEL_REPEATED':
            if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
                if desc.message_type.name=='NDArray':
                    # desc describes a repeated ndarray
                    _retDict[_prefix + desc.name] = [UnpackAndMergeNDArrays(ndarrays) for msg in msgs for ndarrays in zip_longest(getattr(msg, desc.name))]
                elif recursive:
                    # desc describes a repeated subMsg. This behavior is different from the protobuf standard (for now, anyway)
                    UnpackAndMergeMsgs(msgs=[subMsgs for msg in msgs for subMsgs in zip_longest(getattr(msg, desc.name))], _retDict=_retDict, _prefix=desc.name + '.', recursive=recursive)
            else:
                # desc describes a repeated pod
                getattr(msgs[0], desc.name).extend([getattr(msg, desc.name) for msg in msgs[1:]])
        elif GetFieldLabel(desc)=='LABEL_OPTIONAL':
            if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
                filteredVals = [getattr(msg, desc.name) for msg in msgs if msg.HasField(desc.name)]

                if filteredVals:
                    if desc.message_type.name=='NDArray':
                        # desc describes an optional singular ndarray
                        _retDict[_prefix + desc.name] = UnpackAndMergeNDArrays(filteredVals)
                    elif recursive:
                        # desc describes an optional singular subMsg
                        UnpackAndMergeMsgs(msgs=filteredVals, _retDict=_retDict, _prefix=desc.name + '.', recursive=recursive)
            else:
                # desc describes an optional singular pod
                for lastMsg in reversed(msgs):
                    if lastMsg.HasField(desc.name):
                        break
                setattr(msgs[0], desc.name, getattr(lastMsg, desc.name))
        else:
            if GetFieldCPPType(desc)=='CPPTYPE_MESSAGE':
                if desc.message_type.name=='NDArray':
                    # desc describes a required singular ndarray
                    _retDict[_prefix + desc.name] = UnpackAndMergeNDArrays([getattr(msg, desc.name) for msg in msgs])
                elif recursive:
                    # desc describes a required singular subMsg
                    UnpackAndMergeMsgs(msgs=[getattr(msg, desc.name) for msg in msgs], _retDict=_retDict, _prefix=desc.name + '.', recursive=recursive)
            else:
                # desc describes a required singular pod
                setattr(msgs[0], desc.name, getattr(msgs[-1], desc.name))
    return msgs[0]

def Deserialize(msgStr, msgType, unpackNDArray=True):
    '''Deserializes a serialized protobuf message into a protobuf Python object

    :param msgStr: a serialized protobuf message
    :param msgType: the protobuf message class that corresponds to msgStr
    :param unpackNDArray: if True, any NDArrays found in the deserialized msg are converted to numpy arrays and stored in msg.nparrays
    :return: an instance of a protobuf message object
    '''
    msg = msgType()
    msg.ParseFromString(msgStr)

    if unpackNDArray:
        UnpackMsg(msg)

    return msg

def DeserializeAndMerge(msgStrs, msgType, _recursive=True):
    '''Deserializes and merges an iterable of serialized protobuf messages into a single protobuf Python object

    :param msgStrs: an iterable of serialized protobuf messages
    :param msgType: the protobuf message class that corresponds to msgStrs
    :return: an instance of a protobuf message object
    '''
    msgs = [Deserialize(msgStr=msgStr, msgType=msgType, unpackNDArray=False) for msgStr in msgStrs]
    UnpackAndMergeMsgs(msgs=msgs, recursive=_recursive)

    return msgs[0]

class SFileRecordLM(SFileRecordSeekable):
    @property
    def msgType(self):
        # if .dataTypeSuffix is not a known protobuf type, this will raise a KeyError
        return lm.GetMsgType(self.dataTypeSuffix)

    def deserializeAndMerge(self, others, recursive=True):
        # deserialize the records without unpacking the ndarrays
        msgs = [record.msg(unpackNDArray=False) for record in chain([self], others)]

        # merge all of the ndarrays
        UnpackAndMergeMsgs(msgs=msgs, recursive=recursive)

        return msgs[0]

    # more efficient version that fails because of how protobuf merges bytes fields
    #
    # @staticmethod
    # def deserializeAndMerge(records):
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

    def msg(self, unpackNDArray=True):
        return Deserialize(msgStr=self.readData(), msgType=self.msgType, unpackNDArray=unpackNDArray)

class SFileLM(SFileSeekable):
    recordType = SFileRecordLM

    @staticmethod
    def mergeRecordDict(recordDict, byType=False):
        if byType:
            for typeDict in recordDict.values():
                for name,records in typeDict.items():
                    typeDict[name] = records[0].deserializeAndMerge(records[1:])
        else:
            for name,records in recordDict.items():
                recordDict[name] = records[0].deserializeAndMerge(records[1:])

        return recordDict

    def msgs(self, unpackNDArray=True):
        for record in self.records():
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
            self._recordDict = self.genRecordDict()
            return self._recordDict

    def genRecordDict(self, byType=False, merge=False):
        self.reset()

        if byType:
            recordDict = {}

            for record in self.records():
                try:
                    recordDict[record.dataTypeSuffix][record.name].append(record)
                except KeyError:
                    try:
                        recordDict[record.dataTypeSuffix][record.name] = [record]
                    except KeyError:
                        recordDict[record.dataTypeSuffix] = OrderedDict(((record.name, [record]),))
        else:
            recordDict = OrderedDict()

            for record in self.records():
                try:
                    recordDict[record.name].append(record)
                except KeyError:
                    recordDict[record.name] = [record]

        if merge:
            recordDict = self.mergeRecordDict(recordDict, byType=byType)

        return recordDict
