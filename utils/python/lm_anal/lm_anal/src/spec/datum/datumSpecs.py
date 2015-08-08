from lm_anal.src.helper import Setify
from lm_anal.src.spec.datum.datumSpec import DatumSpec
from lm_anal.src.spec.specs import Specs

__all__ = ['DatumSpecs']

class DatumSpecs(Specs):
    def addAlias(self, name, targetName):
        self.addSpecList((DatumSpec(name=name, targetName=targetName, type='alias'),))
    
    def getAliases(self):
        '''
        an iterator over the entries in self.map that contain alias-type DatumSpecs
        '''
        for name,spec in self.items():
            # yield the spec only if both hasattr(spec, 'type') and type=='alias'
            try:
                if spec.type=='alias':
                    yield name,spec
            except AttributeError:
                pass
    
    def getReals(self):
        '''
        an iterator over the entries in self.map that *do not* contain alias-type DatumSpecs
        '''
        for name,spec in self.items():
            # skip the spec if both hasattr(spec, 'type') and type=='alias'
            try:
                if spec.type=='alias':
                    continue
            except AttributeError:
                pass
            yield name,spec
    
    def getUnsatisfied(self, satisfiedNames):
        satisfiedNames = Setify(satisfiedNames)
        unsatisfiedNames = set()
        for name,spec in self.getReals():
            if name not in satisfiedNames:
                unsatisfiedNames.add(name)
        for name,spec in self.getAliases():
            if name in satisfiedNames:
                unsatisfiedNames.discard(spec.targetName)
        return unsatisfiedNames