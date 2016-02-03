import numpy as np

from lm_anal.src.datumABC import GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['FFluxPropToPCloudPropPT']

class DataCaddy(object):
    @property
    def ffluxOutputDatum(self):
        return self.srcDict['FFluxOutput']

    @property
    def ffluxBasinDatum(self):
        return self.srcDict['FFluxBasin']

    @property
    def direction(self):
        return self.ffluxBasinDatum.direction

    @property
    def edges(self):
        return np.asarray(self.tiling.getEdges()[0])[::self.stride]

    @property
    def stride(self):
        return 1 if self.direction==0 else -1

    @property
    def tiling(self):
        return self.kwargs['tilings'][self.ffluxOutputDatum.tiling_id]

    def __init__(self, srcDict, dstDict, **kwargs):
        self.srcDict = srcDict
        self.dstDict = dstDict
        self.kwargs = kwargs

class FFluxPropToPCloudPropPT(BasePT):
    '''
    base property transform class for going from FFlux.something -> PCloud.something
    '''
    dataCaddyType = DataCaddy
    
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('pcloud')

    def getDstDatum(self, dataCaddy):
        pass

    def getFieldDict(self, dataCaddy):
        pass

    def getPointsShape(self, fieldDict):
        return np.max([len(val) for val in fieldDict.values()])

    def ptfd(self, srcDict, dstDict, **kwargs):
        dataCaddy = self.dataCaddyType(srcDict=srcDict, dstDict=dstDict, **kwargs)
        self.setPoints(dataCaddy=dataCaddy)

    def setPoints(self, dataCaddy):
        fieldDict = self.getFieldDict(dataCaddy=dataCaddy)
        shape =  self.getPointsShape(fieldDict=fieldDict)
        pcloudDatum = self.getDstDatum(dataCaddy=dataCaddy)

        points = np.zeros(shape, dtype=pcloudDatum.propertySpecs['points']['dtype'])
        for field,vals in fieldDict.items():
            points[field][...] = vals

        pcloudDatum.setArray('points', points)