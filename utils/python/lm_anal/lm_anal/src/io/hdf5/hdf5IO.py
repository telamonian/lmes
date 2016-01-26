import ast
from collections import namedtuple
import h5py
import numpy as np
import os

from lm_anal.src.helper import CamelCaseUpper, FixedWidth, NumifyString
from lm_anal.src.io.hdf5.hdf5Spec import HDF5Spec
from lm_anal.src.io.hdf5.hdf5Specs import HDF5Specs
from lm_anal.src.io.io import IO
from lm_anal.src.spec.io.hdf5 import HistogramIOSpecs

__all__ = ['HDF5IO']

class HDF5IO(IO):
    suffix = '.lm'
    
    hdf5RootPath = None
    hdf5Specs = None
    
    def __init__(self, fPath, hdf5RootPath=None):
        '''
        fPath: path to the target HDF5 file
        hdf5RootPath: if specified, the starting path within the HDF5 file
        '''
        self.file = None
        if hdf5RootPath!=None:
            self.hdf5RootPath = hdf5RootPath
        self.fPath = fPath

    def delete(self, hdf5Path):
        del self.file[hdf5Path]

    def _dff(self, excludedFields, keys, raiseIfNotExists):
        if keys==None:
            keys = self.keys()

        for key in keys:
            try:
                self.delete(hdf5Path=os.path.join(self.hdf5RootPath, str(key)))
            except KeyError:
                try:
                    # the integer keys in Lattice Microbes hdf5 files are usually in %07d format, so if the key isn't found try putting the key into that format
                    key = '%07d' % key
                    self.delete(hdf5Path=os.path.join(self.hdf5RootPath, str(key)))
                except KeyError:
                    if raiseIfNotExists:
                        raise
                    else:
                        pass

    def dff(self, excludedFields=None, keys=None, raiseIfNotExists=False):
        self.wrapperHDF5(self._dff, mode='a', excludedFields=excludedFields, keys=keys, raiseIfNotExists=raiseIfNotExists)

    def input(self, excludedFields, hdf5Path, o, subCon):
        for spec in self.hdf5Specs:
            self.inputBySpec(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=spec, o=o, subCon=subCon)
    
    def inputBySpec(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        inclusionTests = [hdf5Spec.name not in excludedFields,
                          hdf5Spec.o is None or hdf5Spec.o<=o]
        if np.all(inclusionTests):
            if hdf5Spec.type=='attribute':
                self.inputAttribute(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)
            elif hdf5Spec.type=='dataset':
                self.inputArray(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)
            elif hdf5Spec.type=='embedded':
                self.inputEmbedded(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            elif hdf5Spec.type=='histogram':
                self.inputHist(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            elif hdf5Spec.type=='subData':
                self.inputSubData(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            elif hdf5Spec.type=='special':
                self.__getattribute__('input' + CamelCaseUpper(hdf5Spec.name))(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)
            elif hdf5Spec.type=='specialSubData':
                self.__getattribute__('input' + CamelCaseUpper(hdf5Spec.name))(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            else:
                raise
    
    def inputArray(self, hdf5Path, hdf5Spec, subCon):
        subCon.setArray(name=hdf5Spec.name, val=self.file[hdf5Path][hdf5Spec.subKey])
    
    def inputAttribute(self, hdf5Path, hdf5Spec, subCon):
        subCon.setScalar(name=hdf5Spec.name, val=self.file[hdf5Path].attrs[hdf5Spec.subKey])
    
    def inputEmbedded(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        subData = subCon.initEmbedded(name=hdf5Spec.name, DataType=hdf5Spec.DataType)
        subIO = hdf5Spec.IOType(fPath=self.fPath, hdf5RootPath=hdf5Path)
        subIO.rff(container=subData, excludedFields=excludedFields, o=o)
        return subData

    def inputHist(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        # TORM
        # cache = '_%s' % hdf5Spec.name
        # cache_dirty = '%s_cache_dirty' % hdf5Spec.name
        # dims = '%s_dims' % hdf5Spec.name
        # edges = '%s_edges' % hdf5Spec.name
        # init = 'init%s' % CamelCaseUpper(hdf5Spec.name)
        # mask = '%s_mask' % hdf5Spec.name
        # raw = '%s_raw' % hdf5Spec.name
        # threshold = '%s_threshold' % hdf5Spec.name
        # weight = '%s_weight' % hdf5Spec.name
        #
        # specs = HDF5Specs(HDF5Spec(fullOnly=False, name=edges, subKey=edges, type='dataset'),
        #                   HDF5Spec(fullOnly=True, name=mask, subKey=mask, type='dataset'),
        #                   HDF5Spec(fullOnly=True, name=raw, subKey=raw, type='dataset'),
        #                   HDF5Spec(fullOnly=False, name=threshold, subKey=threshold, type='attribute'),
        #                   HDF5Spec(fullOnly=False, name=weight, subKey=weight, type='attribute'))
        #

        histogramIOSpecs = HistogramIOSpecs(histogramName=hdf5Spec.name)
        for spec in histogramIOSpecs:
            self.inputBySpec(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=spec, o=o, subCon=subCon)
        subCon.setArray(name=histogramIOSpecs.dims, val=np.array(subCon.__getattribute__(histogramIOSpecs.raw).shape))

        # subCon.setArray(name=cache, val=np.zeros(subCon.__getattribute__(raw).shape))
        # subCon.setScalar(name=cache_dirty, val=True)
    
    def inputSubData(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        subData = subCon.getSubData(name=hdf5Spec.name)
        subHdf5RootPath = os.path.join(hdf5Path, hdf5Spec.subKey)
        subIO = hdf5Spec.IOType(fPath=self.fPath, hdf5RootPath=subHdf5RootPath)
        subIO.rff(container=subData, excludedFields=excludedFields, o=o)
        return subData
    
#     def _has(self):
#         if self.hdf5RootPath in self.file:
#             if len(self.file[self.hdf5RootPath].keys()) > 0:
#                 return True
#             else:
#                 return False
#         else:
#             return False
#         
#     def has(self):
#         '''
#         test if an hdf5 file has a non-empty group containing data relevant to this particular object
#         '''
#         return self.wrapperHDF5(self._has)

    def has(self):
        '''
        test if an hdf5 file has any entries relevant to the Data type that we're trying to read in/out
        '''
        try:
            return len(self.keys())!=0
        except KeyError:
            # if we got here, the hdf5 file doesn't contain the relevant group
            return False
        except TypeError:
            # if we got here, the hdf5 file doesn't even exist yet
            return False

    def _keys(self):
        '''
        basic hdf5 version of keys. Assumes that relevant data is located in each of the subgroups of self.hdf5RootPath
        '''
        return list(self.file[self.hdf5RootPath].keys())

    def keys(self):
        return self.wrapperHDF5(self._keys)

    def output(self, excludedFields, hdf5Path, o, subCon):
        for spec in self.hdf5Specs:
            self.outputBySpec(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=spec, o=o, subCon=subCon)
    
    def outputBySpec(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        inclusionTests = [hdf5Spec.name not in excludedFields,
                          hdf5Spec.o is None or hdf5Spec.o<=o]
        if np.all(inclusionTests):
            if hdf5Spec.type=='attribute':
                self.outputAttribute(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)
            elif hdf5Spec.type=='dataset':
                self.outputArray(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)
            elif hdf5Spec.type=='embedded':
                self.outputEmbedded(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            elif hdf5Spec.type=='histogram':
                self.outputHist(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            elif hdf5Spec.type=='subData':
                self.outputSubData(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            elif hdf5Spec.type=='special':
                self.__getattribute__('output' + CamelCaseUpper(hdf5Spec.name))(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)
            elif hdf5Spec.type=='specialSubData':
                self.__getattribute__('output' + CamelCaseUpper(hdf5Spec.name))(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
            else:
                raise
    
    def outputArray(self, hdf5Path, hdf5Spec, subCon):
        if hdf5Path not in self.file:
            group = self.file.create_group(hdf5Path)
        else:
            group = self.file[hdf5Path]
            if hdf5Spec.subKey in group:
                del group[hdf5Spec.subKey]
        group.create_dataset(hdf5Spec.subKey, data=subCon.getArray(name=hdf5Spec.name))
    
    def outputAttribute(self, hdf5Path, hdf5Spec, subCon):
        if hdf5Path not in self.file:
            self.file.create_group(hdf5Path)
        self.file[hdf5Path].attrs[hdf5Spec.subKey] = subCon.getScalar(name=hdf5Spec.name)
    
    def outputEmbedded(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        subData = subCon.getEmbedded(name=hdf5Spec.name)
        subIO = hdf5Spec.IOType(fPath=self.fPath, hdf5RootPath=hdf5Path)
        subIO.wtf(container=subData, excludedFields=excludedFields, o=o)
        return subData

    def outputHist(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        # TORM
        # cache = '%s' % hdf5Spec.name
        # edges = '%s_edges' % hdf5Spec.name
        # mask = '%s_mask' % hdf5Spec.name
        # raw = '%s_raw' % hdf5Spec.name
        # threshold = '%s_threshold' % hdf5Spec.name
        # weight = '%s_weight' % hdf5Spec.name
        #
        # specs = HDF5Specs(HDF5Spec(name=cache, subKey=cache, type='dataset'),
        #                   HDF5Spec(name=edges, subKey=edges, type='dataset'),
        #                   HDF5Spec(name=mask, subKey=mask, type='dataset'),
        #                   HDF5Spec(name=raw, subKey=raw, type='dataset'),
        #                   HDF5Spec(name=threshold, subKey=threshold, type='attribute'),
        #                   HDF5Spec(name=weight, subKey=weight, type='attribute'))
        
        for spec in HistogramIOSpecs(histogramName=hdf5Spec.name):
            self.outputBySpec(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=spec, o=o, subCon=subCon)

    def outputSubData(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        subData = subCon.getSubData(name=hdf5Spec.name)
        subHdf5RootPath = os.path.join(hdf5Path, hdf5Spec.subKey)
        subIO = hdf5Spec.IOType(fPath=self.fPath, hdf5RootPath=subHdf5RootPath)
        subIO.wtf(container=subData, excludedFields=excludedFields, o=o)
        return subData
    
    def _rff(self, container, excludedFields, keys, o):
        '''
        internal generic rff (read from file) for data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
    
        for key in keys:
            # try:
            #     datumKey = int(key)
            # except ValueError:
            #     try:
            #         datumKey = ast.literal_eval(key)
            #     except ValueError:
            #         datumKey = key

            # convert the key to a numeric type, if possible
            datumKey = NumifyString(key)
            subCon = container.initDatum(key=datumKey, o=o)
            try:
                self.input(excludedFields=excludedFields, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), o=o, subCon=subCon)
            except KeyError:
                # the integer keys in Lattice Microbes hdf5 files are usually in %07d format, so if the key isn't found try putting the key into that format
                del container[int(key)]
                key = '%07d' % key
                subCon = container.initDatum(key=int(key), o=o)
                self.input(excludedFields=excludedFields, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), o=o, subCon=subCon)
    
    def rff(self, container, excludedFields=None, keys=None, o=None):
        '''
        rff (read from file) for hdf5 files
        '''
        return self.wrapperHDF5(self._rff, container=container, excludedFields=excludedFields, keys=keys, o=o)
    
    def sff(self, keys=None, o=None):
        '''
        sff (stream from file) for data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
                
        for key in keys:
            try:
                self.rff(keys=[key], o=o)
            except KeyError:
                key = '%07d' % key
                self.rff(keys=[key], o=o)
            yield self[int(key)]
            del self[int(key)]
        
    def _wtf(self, container, excludedFields, keys, o):
        '''
        internal generic wtf (write to file) to store data in hdf5 files
        '''
        if keys==None:
            keys = container.keys()
            
        for key in keys:
            # if the key is an integer, write it in the file as standard %07d Lattice Microbes hdf5 output form
            outKey = str(FixedWidth(key))
            self.output(excludedFields=excludedFields, hdf5Path=os.path.join(self.hdf5RootPath, outKey), o=o, subCon=container[key])
        
    def wtf(self, container, excludedFields=None, keys=None, o=None):
        '''
        wtf (write to file) for hdf5 files
        '''
        self.wrapperHDF5(self._wtf, mode='a', container=container, excludedFields=excludedFields, keys=keys, o=o)

    def wrapperHDF5(self, func, mode='r', **kwargs):
        '''
        if self.file==None, run the function (with *args) inside a 'with' block that assigns the hdf5 file object to self.file, then resets self.file to None
        else (self.file already contains something (hopefully the relevant hdf5 file)), just run the function (with *args)
        '''
        if self.file==None:
            try:
                with h5py.File(self.fPath, mode) as self.file:
                    retVal = func(**kwargs)
            except OSError:
                self.file = None
                return False
            # not entirely sure why this has to be a finally: block, but without it there do seem to be cases where .file doesn't get Noned out
            finally:
                self.file = None
        else:
            retVal = func(**kwargs)
        return retVal