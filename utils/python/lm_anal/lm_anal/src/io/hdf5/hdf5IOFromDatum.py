from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.io.hdf5.hdf5Spec import HDF5Spec
from lm_anal.src.io.hdf5.hdf5Specs import HDF5Specs
from lm_anal.src.io.hdf5.hdf5IO import HDF5IO

__all__ = ['HDF5IOFromDatum', 'HDF5IOFromDatumMetaclass']

class HDF5IOFromDatumMetaclass(type):
    datumTypeToHDF5TypeDict = {'alias': None,
                               'fieldAlias': None,
                               'array': 'dataset',
                               'embedded': 'embedded',
                               'histogram': 'histogram',
                               'scalar': 'attribute',
                               'subData': 'subData',
                               'special': 'special'}

    def __new__(cls, clsname, bases, dct):
        if 'datum' in dct:
            hdf5SpecsFromDatum = cls.getHDF5SpecsFromDatumSpecs(dct['datum'].combinedPropertySpecs, dct)
            dct['hdf5Specs'] = hdf5SpecsFromDatum.update(dct.get('hdf5Specs', HDF5Specs()))
        return super().__new__(cls, clsname, bases, dct)

    def getHDF5SpecsFromDatumSpecs(cls, datumSpecs, dct):
        newHdf5Specs = HDF5Specs()
        for datumSpec in datumSpecs:
            hdf5SpecKwargs = {}
            hdf5SpecKwargs['name'] = datumSpec['name']
            hdf5SpecKwargs['fullOnly'] = False
            hdf5SpecKwargs['subKey'] = CamelCaseUpper(datumSpec['name'])
            if datumSpec['type'] not in cls.datumTypeToHDF5TypeDict:
                raise KeyError('while trying to generate an HDF5Spec from a DatumSpec, the type of the DatumSpec could not be matched to any of the known HDF5Spec types.\n \
                                datumSpec.map: %s, datumTypeToHDF5TypeDict: %s' % (datumSpec.map, cls.datumTypeToHDF5TypeDict))
            else:
                hdf5SpecKwargs['type'] = cls.datumTypeToHDF5TypeDict[datumSpec['type']]
            newHdf5Specs[hdf5SpecKwargs['name']] = HDF5Spec(**hdf5SpecKwargs)
        return newHdf5Specs

class HDF5IOFromDatum(HDF5IO, metaclass=HDF5IOFromDatumMetaclass):
    datum = None