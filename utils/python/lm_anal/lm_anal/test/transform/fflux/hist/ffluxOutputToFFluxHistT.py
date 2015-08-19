import os
from pathlib import Path

from lm_anal.test.datum.hist import FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase, FFluxHistsSimTestBase

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
# testFilePath = (thisScriptDir / Path('../../../testData/biphasic_switch.lm')).resolve()
testFilePath = (thisScriptDir / Path('../../../../../../../../regression/biphasic_switch.lm')).resolve()

class FFluxOutputToFFluxHistTTestBase(FFluxHistsFieldsTestBase):
    testFilePath = testFilePath
    
class FFluxOutputToFFluxHistTSimTestBase(FFluxOutputToFFluxHistTTestBase, FFluxHistsSimTestBase):
    pass