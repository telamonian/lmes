from collections import namedtuple, OrderedDict
import numpy as np
import re

from robertslab.sfile import SFileProto

dataPath = 'genetic_toggle_switch_-_out.sfile'
lowBound,highBound = -27,27

stageinfotup = namedtuple('stageinfotup', ['rep', 'tiling', 'basin', 'stage'])
# phaseinfotup = namedtuple('phaseinfotup', ['rep', 'tiling', 'basin', 'stage', 'phase'])

repRe    = re.compile('Simulations/(\d*)')
tilingRe = re.compile('Tilings/(\d*)')
basinRe  = re.compile('Basins/(\d*)')
stageRe  = re.compile('Stages/(\w*)')
phaseRe  = re.compile('Phases/(\d*)')

def oparamDelta(count):
    delta = sum(count[id]*coeff for id,coeff in zip((0,1,2,3,4,5), (-1,-2,-2,1,2,2)))

    return delta

def test_trajectory_id_coordination():
    raws = OrderedDict()
    summaries = OrderedDict()
    specTSByStage = OrderedDict()

    with SFileProto.open(dataPath, progress=True) as f:
        for rec in f.records():
            rep = int(repRe.search(rec.name)[1])
            tiling = int(tilingRe.search(rec.name)[1])
            basin = int(basinRe.search(rec.name)[1])
            stage = stageRe.search(rec.name)[1]

            stagetup = stageinfotup(rep=rep, tiling=tiling, basin=basin, stage=stage)

            if rec.dataType.find('StageOutputRaw') > -1:
                raws[stagetup] = rec.msg()

            if rec.dataType.find('StageOutputSummary') > -1:
                summaries[stagetup] = rec.msg()

            if rec.dataType.find('SpeciesTimeSeries') > -1:
                phase = int(phaseRe.search(rec.name)[1])

                specTSByStage[stagetup] = specTSByPhase = specTSByStage.get(stagetup, OrderedDict())
                specTSByPhase[phase] = specTSByPhase.get(phase, []) + [(rec, rec.msg(unpackNDArray=True))]

    minTSByStage = {}
    maxTSByStage = {}
    for stagetup in raws.keys():
        print(stagetup)
        raw = raws[stagetup]
        summary = summaries[stagetup]

        # initialEdge = summary.edges[0]

        specTSByPhase = specTSByStage[stagetup]
        minTSByPhase = minTSByStage[stagetup] = {}
        maxTSByPhase = maxTSByStage[stagetup] = {}

        for i,(firstTrajID,finalTrajID) in enumerate(zip(raw.first_trajectory_ids, raw.final_trajectory_ids)):
            if i not in specTSByPhase:
                print("No species count samples from phase %d" % i)
                continue

            # goalEdge = summary.edges[i]
            # lowBound,highBound = (initialEdge,goalEdge) if goalEdge >= initialEdge else (goalEdge,initialEdge)

            minTS,maxTS = None,None
            for specTS in specTSByPhase[i]:
                trajID = specTS[1][0].trajectory_id

                assert firstTrajID <= trajID <= finalTrajID
                opval = np.array([oparamDelta(row) for row in specTS[1][1]['counts']])
                minTSNew,maxTSNew = opval.ravel().min(), opval.ravel().max()

                minTS = minTSNew if (minTS is None or minTSNew < minTS) else minTS
                maxTS = maxTSNew if (maxTS is None or maxTSNew > maxTS) else maxTS

                if i>0:
                    if minTSNew < lowBound - 1 or maxTSNew >= highBound + 1:
                        print("trajectory %d out of bounds. low: %.1f, high: %.1f, start: %.1f, stop: %.1f" % (trajID, minTSNew, maxTSNew, opval[0], opval[-1]))

            minTSByPhase[i] = minTS
            maxTSByPhase[i] = maxTS

            print("%s, phase: %d -> min opval: %d, max opval: %d" % (stagetup, i, minTS, maxTS))

if __name__=='__main__':
    test_trajectory_id_coordination()
