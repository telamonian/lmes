#!/usr/bin/env python

import h5py
import numpy as np
from collections import namedtuple

Interface = namedtuple('Interface', ['orderParameterID','binBorders'])
OrderParameter = namedtuple('OrderParameter', ['id','type','speciesIDs','speciesCoefficients'])

class FFluxParameters(object):
    def __init__(self, fname):
        self.f = h5py.File('.'.join((fname,'lm')), 'a')
        if 'Parameters' in self.f.keys():
            if 'FFlux' in self.f['Parameters'].keys():
                del self.f['Parameters/FFlux']
        self.ifaceContainingGroup = self.f.create_group('Parameters/FFlux/Interface')
        self.opContainingGroup = self.f.create_group('Parameters/FFlux/OrderParameter')
        
    def AddInterface(self, iface):
        ifaceGroup = self.f.create_group("Parameters/FFlux/Interface/%07d" % len(self.ifaceContainingGroup.keys()))
        ifaceGroup.attrs['OrderParameterID'] = iface.orderParameterID
        binBorders = ifaceGroup.create_dataset("BinBorders", (len(iface.binBorders),), dtype=np.dtype('d'))
        binBorders[...] = iface.binBorders
            
    def AddInterfaces(self, ifaces):
        for iface in ifaces:
            self.AddInterface(iface)
    
    def AddOrderParameter(self, op):
        opGroup = self.f.create_group("Parameters/FFlux/OrderParameter/%07d" % op.id)
        opGroup.attrs['Type'] = op.type
        opGroup.attrs['ID'] = op.id
        speciesIDs = opGroup.create_dataset("SpeciesIDs", (len(op.speciesIDs),), dtype=np.dtype('uint32'))
        speciesIDs[...] = op.speciesIDs
        speciesCoefficients = opGroup.create_dataset("SpeciesCoefficients", (len(op.speciesCoefficients),), dtype=np.dtype('d'))
        speciesCoefficients[...] = op.speciesCoefficients
            
    def AddOrderParameters(self, ops):
        for op in ops:
            self.AddOrderParameter(op)
        
if __name__=="__main__":
    iface = Interface(orderParameterID = 0,
                      binBorders = np.linspace(-25,25,13))
    op = OrderParameter(type = 0,
                        id = 0,
                        speciesIDs = [1,2,3,4,5,6],
                        speciesCoefficients = [1,2,2,-1,-2,-2])
    ffluxParameters = FFluxParameters('biphasic_switch')
    ffluxParameters.AddInterface(iface)
    ffluxParameters.AddOrderParameter(op)