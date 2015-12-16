from lm_anal.src.datum.fpt.fpt import FPT
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['OParamFPT']

class OParamFPT(FPT):
    # change name of base FPT 'id' field to 'oparam_id', and type of 'count' field to float
    propertySpecs = DatSpcs(DatSpc(name='points', dtype=[('oparam_id', 'int'), ('count', 'float'), ('time', 'float')], paths=('points'), storageType='numpy', type='array'))

    # add some field alias specs
    propertySpecs.addFieldAlias(name='id', targetField='oparam_id', targetName='points')