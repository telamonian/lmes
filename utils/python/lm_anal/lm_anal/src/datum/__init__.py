from lm_anal.src.helper import ShallowImportAllModules

localDict, allList = ShallowImportAllModules(path=__path__, name=__name__)
locals().update(localDict)
__all__=allList

# import lm_anal.src.datum.datum as datum
# from lm_anal.src.datum.datum import Datum, DatumMetaclass
# from lm_anal.src.datum.datumPropertySpec import DatumPropertySpec
# 
#  
# # from lm_anal.src.datum.data import Data
# import lm_anal.src.datum
# @property
# def Data(self):
# #     import lm_anal.src.datum.data
#     return lm_anal.src.datum.data.Data
# 
# from lm_anal.src.datum.datumPropertySpecs import DatumPropertySpecs

# Data = data.Data
# Datum = datum.Datum
# DatumMetaclass = datum.DatumMetaclass