import re

from lm_anal.src.helper import Setify
from lm_anal.src.spec.datum.datumSpec import DatumSpec
from lm_anal.src.spec.specs import Specs

__all__ = ['DatumSpecs']

class DatumSpecs(Specs):
    aliasRe = re.compile('alias', re.IGNORECASE)

    def addAlias(self, name, targetName):
        self.addSpecList((DatumSpec(name=name, targetName=targetName, type='alias'),))

    def addFieldAlias(self, name, targetField, targetName):
        self.addSpecList((DatumSpec(name=name, targetField=targetField, targetName=targetName, type='alias'),))

    def getAliases(self):
        '''
        an iterator over the entries in self.map that contain alias-type DatumSpecs
        '''
        for name,spec in self.items():
            # yield the spec only if it is an alias type
            if spec.type is not None:
                # if the word 'alias' can be found in .type, it is an alias type
                if self.aliasRe.search(spec.type):
                    yield name,spec
    
    def getReals(self):
        '''
        an iterator over the entries in self.map that *do not* contain alias-type DatumSpecs
        '''
        for name,spec in self.items():
            # yield the spec only if it is not an alias type
            if spec.type is not None:
                # if the word 'alias' can be found in .type, it is an alias type
                if self.aliasRe.search(spec.type):
                    continue
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