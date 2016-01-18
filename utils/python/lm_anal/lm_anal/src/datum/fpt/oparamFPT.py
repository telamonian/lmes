from lm_anal.src.datum.fpt.fpt import FPT
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['OParamFPT']

class OParamFPT(FPT):
    # change name of base FPT 'id' field to 'oparam_id', and type of 'count' field to float
    pointsDtype = [('oparam_id', 'int'), ('count', 'float'), ('initial_count', 'float'), ('time', 'float')]
    propertySpecs = DatSpcs(
        DatSpc(name='order_parameter_id', paths=('order_parameter_id'), type='scalar'),
        DatSpc(name='points', dtype=pointsDtype, paths=('points'), storageType='numpy', type='array')
    )

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='oparamFPT', targetName='points')

    # add some field alias specs
    propertySpecs.addFieldAlias(name='id', targetField='oparam_id', targetName='points')
    propertySpecs.addFieldAlias(name='oparam_id', targetField='oparam_id', targetName='points')