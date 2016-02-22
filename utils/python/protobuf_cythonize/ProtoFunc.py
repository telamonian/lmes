from helper import ConvertType
import re
import sys
        
class ProtoFunc(object):
    def __init__(self, funcLine):
        self.funcLine = funcLine
        self.pxd = [' '*8 + '%s %s(%s)\n']
        self.pyx = [' '*4 + 'def %s(self%s):\n',
                    ' '*8 + '%sself.thisptr.%s(%s)\n']
        self.tokens = self.funcLine.split(' ')
        self.FixTemplatedTokens()
        if self.Tag()=='simple':
            self.SortTokens()
            self.ConvertType()
            
    def __str__(self):
        return 'returnType: %s, funcName: %s, argTypes: %s, argNames: %s' % (self.returnType, self.funcName, self.argTypes, self.argNames)
    
    def ConvertType(self):
        self.returnType = ConvertType(self.returnType)
        self.argTypes = ConvertType(self.argTypes)
    
    def FixTemplatedTokens(self):
        '''
        fix templated tokens with whitespace in them (so they show up as a single token)
        '''
        i = 0
        while i < len(self.tokens):
            if '<' in self.tokens[i]:
                j=i
                while j < len(self.tokens):
                    if '>' in self.tokens[j]:
                        self.tokens[i:j+1] = [' '.join(self.tokens[i:j+1])]
                        i=j
                        break
                    else:
                        j+=1
            i+=1
    
    def GenPxd(self):
        # the argSubString stuff allows for variable numbers of function arguments in the final substituted .pxd line
        if len(self.argTypes) > 0:
            argSubString = '%s' + ',%s'*(len(self.argTypes) - 1)
        else:
            argSubString = ''
        self.pxd[0] = (self.pxd[0] % ('%s', '%s', argSubString)) % tuple([self.returnType] + [self.funcName] + self.argTypes)
        
    def GenPyx(self):
        # the argSubString stuff allows for variable numbers of function arguments in the final substituted .pyx lines
        if len(self.argTypes) > 0:
            argSubStringSig = ', %s %s'*len(self.argTypes)
            argSubStringCall = '%s' + ',%s'*(len(self.argTypes) - 1)
        else:
            argSubStringSig = ''
            argSubStringCall = ''
        funcSignatureList = [item for sublist in zip(self.argTypes,self.argNames) for item in sublist]
        self.pyx[0] = (self.pyx[0] % ('%s', argSubStringSig)) % tuple([self.funcName] + funcSignatureList)
        # if the function has a return type, then the function definition in the .pyx should contain the 'return ' keyword
        returnSub = '' if self.returnType=='void' else 'return '
        self.pyx[1] = (self.pyx[1] % ('%s', '%s', argSubStringCall)) % tuple([returnSub] + [self.funcName] + self.argNames)
    
    def SortTokens(self):
        '''
        sort the tokens into return type, function name, argument types, and argument names
        '''
        self.returnType = None
        self.funcName = None
        self.argNames = []
        self.argTypes = []
        i = 0
        while i < len(self.tokens):
            if '(' in self.tokens[i]:
                self.returnType = self.tokens[i-1]
                self.funcName = self.tokens[i][:self.tokens[i].index('(')]
                if ')' in self.tokens[i]:
                    pass
                else:
                    # TODO: this part may need to be made more robust in the future
                    j = i
                    while j < len(self.tokens):
                        self.argTypes.append(self.tokens[j].split('(')[-1].strip().strip(';,').strip())
                        self.argNames.append(self.tokens[j+1].split(')')[0].strip().strip(';,').strip())
                        if ')' in self.tokens[j+1]:
                            break
                        j+=2
                break
            i+=1
                    
    def Tag(self):
        '''
        tag the different functions (sets of tokens, really) by how they should be implemented in cython
        '''
        for token in self.tokens:
            if '//' in token:
                return 'off'
        for token in self.tokens:
            if token=='static':
                return 'static'
        for token in self.tokens:
            if 'RepeatedField' in token:
                return 'RepeatedField'
        for token in self.tokens:
            if 'RepeatedPtrField' in token:
                return 'RepeatedPtrField'
        return 'simple'