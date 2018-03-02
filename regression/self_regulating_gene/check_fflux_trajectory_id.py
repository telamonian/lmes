from collections import namedtuple, OrderedDict
import re

from robertslab.sfile import SFileProto

stageinfotup = namedtuple('stageinfotup', ['rep', 'tiling', 'basin', 'stage'])
# phaseinfotup = namedtuple('phaseinfotup', ['rep', 'tiling', 'basin', 'stage', 'phase'])

repRe    = re.compile('Simulations/(\d*)')
tilingRe = re.compile('Tilings/(\d*)')
basinRe  = re.compile('Basins/(\d*)')
stageRe  = re.compile('Stages/(\w*)')
phaseRe  = re.compile('Phases/(\d*)')

def test_trajectory_id_coordination():
    with SFileProto.open('self_regulating_gene_-_out.sfile') as f:
        raws = OrderedDict()
        for rec in f.records(dataType='raw'):
            rep = int(repRe.search(rec.name)[1])
            tiling = int(tilingRe.search(rec.name)[1])
            basin = int(basinRe.search(rec.name)[1])
            stage = stageRe.search(rec.name)[1]

            stagetup = stageinfotup(rep=rep, tiling=tiling, basin=basin, stage=stage)

            raws[stagetup] = rec.msg()[0]

    with SFileProto.open('self_regulating_gene_-_out.sfile') as f:
        specTSByStage = OrderedDict()
        for rec in f.records(dataType='speciestimeseries'):
            rep = int(repRe.search(rec.name)[1])
            tiling = int(tilingRe.search(rec.name)[1])
            basin = int(basinRe.search(rec.name)[1])
            stage = stageRe.search(rec.name)[1]
            phase = int(phaseRe.search(rec.name)[1])

            stagetup = stageinfotup(rep=rep, tiling=tiling, basin=basin, stage=stage)

            specTSByStage[stagetup] = specTSByPhase = specTSByStage.get(stagetup, OrderedDict())
            specTSByPhase[phase] = specTSByPhase.get(phase, []) + [(rec, rec.msg())]

    for stagetup,raw in raws.items():
        print(stagetup)
        specTSByPhase = specTSByStage[stagetup]

        for i,(firstTrajID,finalTrajID) in enumerate(zip(raw.first_trajectory_ids, raw.final_trajectory_ids)):
            if i not in specTSByPhase:
                print("No species count samples from phase %d" % i)
                continue

            for specTS in specTSByPhase[i]:
                assert firstTrajID <= specTS[1][0].trajectory_id <= finalTrajID

if __name__=='__main__':
    test_trajectory_id_coordination()
