#!/usr/bin/env python3.4

import h5py
import numpy as np
import os,sys
import re
from six import print_

headers = ['param_name_0', 'param_val_0', 'param_name_1', 'param_val_1', 'switching_rate', 'flux_forward', 'probability_forward', 'flux_backward', 'probability_backward']

def GetFPTs(lmFPath, fptSpecies, fptCount):
    fpts = []
    with h5py.File(lmFPath) as lmF:
        for replicateID,data in lmF['/Simulations'].items():
            rowIndex = None
            for i,count in enumerate(data['FirstPassageTimes']['%02d' % fptSpecies]['Counts']):
                if count==fptCount:
                    rowIndex = i
                    break
        if rowIndex!=None:
            time = data['FirstPassageTimes']['%02d' % fptSpecies]['Times'][rowIndex]
            fpts.append([float(replicateID),float(time)])
    return np.array(fpts)

def GetFPTsFromLimitReplicates(lmFPath, fptSpecies, fptCount):
    fpts = []
    with h5py.File(lmFPath) as lmF:
        for replicateID,data in lmF['/Simulations'].items():
            fpts.append([float(replicateID), float(data['SpeciesCountTimes'][-1])])
    return np.array(fpts)

def GetSweepSwitch(lmRootPath, fptSpecies, fptCount):
    switches = []
    for tup in os.walk(lmRootPath):
        for fname in tup[2]:
            if fname[-3:]=='.lm':
                print_(tup, end='')
                sweepParamRe = re.search('([^\W\d_]+)([\d\.]+)_([^\W\d_]+)([\d\.]+)', tup[0])
#                 fpts = GetFPTs(os.path.join(tup[0],fname), fptSpecies, fptCount)
                fpts = GetFPTsFromLimitReplicates(os.path.join(tup[0],fname), fptSpecies, fptCount)
                print_(" found %d fpts" % fpts.shape[0])
                if fpts.shape[0] > 0:
                    switch = np.mean(fpts[:,1])
                    switches.append([sweepParamRe.group(1), sweepParamRe.group(2), sweepParamRe.group(3), sweepParamRe.group(4), switch])
    return switches

def GetSwitchFFlux(lmFPath, tilingID):
    with h5py.File(lmFPath) as lmF:
        backward = lmF['/Tilings']['%07d' % tilingID]['FFluxOutput'].attrs['SwitchingRateConstant_FromBasin0']
        forward = lmF['/Tilings']['%07d' % tilingID]['FFluxOutput'].attrs['SwitchingRateConstant_FromBasin1']
        switch_data_breakdown = []
        for direction in ['FORWARD', 'BACKWARD']:
            switch_data_breakdown.append(lmF['/Tilings']['%07d' % tilingID]['FFluxOutput'][direction].attrs['FluxOutOfTileZero'])
            switch_data_breakdown.append(np.product(lmF['/Tilings']['%07d' % tilingID]['FFluxOutput'][direction]['ProbabilityIToIPlusOne']['TileVals'][1:-1]))
        print_('forward rate %s backward rate %s' % (forward, backward))
        return [np.mean([float(backward), float(forward)])] + switch_data_breakdown

def GetSwitchFFlux_old(lmFPath, tilingID):
    with h5py.File(lmFPath) as lmF:
        backward = lmF['/Tilings']['%07d' % tilingID]['FFluxOutput']['BACKWARD'].attrs['SwitchingRateConstant']
        forward = lmF['/Tilings']['%07d' % tilingID]['FFluxOutput']['FORWARD'].attrs['SwitchingRateConstant']
        return np.mean([float(backward), float(forward)])

def GetSweepSwitchFFlux(lmRootPath, tilingID):
    switches = []
    for tup in os.walk(lmRootPath):
        for fname in tup[2]:
            if fname[-3:]=='.lm':
                print_(tup)
                sweepParamRe = re.search('([^\W\d_]+)([\d\.]+)_([^\W\d_]+)([\d\.]+)', tup[0])
                try:
                    switch_data_list = GetSwitchFFlux(os.path.join(tup[0],fname), tilingID)
                except KeyError:
                    continue
                switches.append([sweepParamRe.group(1), sweepParamRe.group(2), sweepParamRe.group(3), sweepParamRe.group(4)] + switch_data_list)
    return switches
                

if __name__=='__main__':
    fptSpecies = 4
    fptCount = 11
    tilingID = 0
    switchingTimes = GetSweepSwitch(lmRootPath=sys.argv[1], fptSpecies=fptSpecies, fptCount=fptCount)
#     switchingTimes = GetSweepSwitchFFlux(lmRootPath=sys.argv[1], tilingID=tilingID)
    for header in headers:
        print_(header, end='')
        print_(',', end='')
    print_('\n', end='')
    for row in sorted(switchingTimes, key=lambda st: float(st[1])):
        for col in row:
            print_(col, end='')
            print_(',', end='')
        print_('\n', end='')
    #print(GetFPTs(lmFPath=sys.argv[1], fptSpecies=fptSpecies, fptCount=fptCount))
