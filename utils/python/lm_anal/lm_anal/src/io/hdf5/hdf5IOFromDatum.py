from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.io.hdf5.hdf5IO import HDF5IO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

__all__ = ['HDF5IOFromDatum', 'HDF5IOFromDatumMetaclass']

datumTypeToHDF5TypeDict = {'alias': None,
                           'fieldAlias': None,
                           'array': 'dataset',
                           'embedded': 'embedded',
                           'histogram': 'histogram',
                           'scalar': 'attribute',
                           'subData': 'subData',
                           'special': 'special'}

def GetHDF5IOSpecsFromDatumSpecs(datumSpecs, dct):
    newHdf5Specs = HDF5IOSpecs()
    for datumSpec in datumSpecs.getReals():
        hdf5SpecKwargs = {}
        hdf5SpecKwargs['name'] = datumSpec['name']
        hdf5SpecKwargs['fullOnly'] = False
        hdf5SpecKwargs['subKey'] = CamelCaseUpper(datumSpec['name'])
        if datumSpec['type'] not in datumTypeToHDF5TypeDict:
            raise KeyError('while trying to generate an HDF5IOSpec from a DatumSpec, the type of the DatumSpec could not be matched to any of the known HDF5IOSpec types.\n\
                            datumSpec.map: %s, datumTypeToHDF5TypeDict: %s' % (datumSpec.map, datumTypeToHDF5TypeDict))
        else:
            hdf5SpecKwargs['type'] = datumTypeToHDF5TypeDict[datumSpec['type']]

        newHdf5Specs[hdf5SpecKwargs['name']] = HDF5IOSpec(**hdf5SpecKwargs)
    return newHdf5Specs

class HDF5IOFromDatumMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        if 'datumType' in dct and dct['datumType'] is not None:
            datumType = dct['datumType']
        elif 'dataType' in dct and dct['dataType'].datumType is not None:
            datumType = dct['dataType'].datumType
        else:
            datumType = None

        if datumType is not None:
            hdf5SpecsFromDatum = GetHDF5IOSpecsFromDatumSpecs(datumSpecs=datumType.combinedPropertySpecs, dct=dct)
            hdf5SpecsFromDatum.update(dct.get('hdf5Specs', HDF5IOSpecs()))
            dct['hdf5Specs'] = hdf5SpecsFromDatum
        return super().__new__(cls, clsname, bases, dct)

class HDF5IOFromDatum(HDF5IO, metaclass=HDF5IOFromDatumMetaclass):
    datumType = None