class OParam(object):
    def __init__(self, oparamBuf, hdf5OParamGroup=None):
        self.oparamBuf = oparamBuf
        if hdf5OParamGroup!=None:
            self.InitFromHdf5(hdf5OParamGroup)

        # add some attributes to the OParam instance that allow for direct access to the underlying OParamBuf
        self.type = self.oparamBuf.type
        self.id = self.oparamBuf.id
        self.species_coefficients = self.oparamBuf.species_coefficients
        self.species_ids = self.oparamBuf.species_ids

    def InitFromHdf5(self, hdf5OParamGroup):
            # initialize the data storage container underlying this OParam instance, which is in turn a OParamBuf instance
            self.oparamBuf.type = int(hdf5OParamGroup.attrs['Type'])
            self.oparamBuf.id = int(hdf5OParamGroup.attrs['ID'])
            for val in hdf5OParamGroup['SpeciesCoefficients']:
                self.oparamBuf.species_coefficients.append(val)
            for val in hdf5OParamGroup['SpeciesIDs']:
                self.oparamBuf.species_ids.append(int(val))