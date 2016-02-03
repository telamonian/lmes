import os
import re

__all__ = ['CamelCaseLower', 'CamelCaseUpper', 'FixedWidth', 'ListInStr', 'PathJoin', 'ShortenName', 'Singular', 'SnakeCaseLower']

def CamelCaseLower(s):
    '''
    convert CamelCase (or snake_case) to camelCase
    '''
    if '_' in s:
        output = ''
        tokens = s.split('_')
        output+=CamelCaseLower(tokens[0])
        for token in tokens[1:]:
            output+=CamelCaseUpper(token)
        return output
    else:
        output = s
        for i,l in enumerate(output):
            if l.isupper():
                output = output[:i] + l.lower() + output[i+1:]
            else:
                return output
        return output

def CamelCaseUpper(s):
    '''
    convert camelCase (or snake_case) to CamelCase
    '''
    if '_' in s:
        output = ''
        for token in s.split('_'):
            output+=CamelCaseUpper(token)
        return output
    else:
        # using s[0:1] instead of s[0] handles the empty string appropriately
        return s[0:1].upper() + s[1:]

def FixedWidth(s, width='07'):
    '''
    helper function for formatting keys in the Lattice Microbes hdf5 standard '%07d' format
    '''
    if isinstance(s, int):
        return ('%%sd' % width) % s
    else:
        return s

def ListInStr(l,s):
    '''
    test if any of a list of substrings is in a string
    '''
    for subS in l:
        if subS in s:
            return True
    return False

def PathJoin(path, *paths):
    '''
    exactly like os.path.join, except that it *will* join absolute paths without throwing out prior path elements
    '''
    return os.path.join(path, *[path.lstrip(os.sep) for path in paths])

def ShortenName(s):
    '''
    input: CamelCase style name 
    output: abbreviated form (in lowercase) generated from the first letter of each "word"
    '''
    return ''.join(re.findall('[A-Z]', s)).lower()

def Singular(s):
    '''
    returns the singular form of a plural name
    '''
    if s[-3:]=='ies':
        return s[:-3] + 'y'
    elif s[-1]=='s':
        return s[:-1]

capitalizedWordRe = re.compile(r'([^_])([A-Z][a-z]+)')
blockOfCapitalsRe = re.compile(r'([a-z0-9])([A-Z])')
def SnakeCaseLower(s):
    '''
    convert camelCase, CamelCase, or Snake_Case to snake_case
    modified from http://stackoverflow.com/a/1176023/425458
    '''
    s1 = capitalizedWordRe.sub(r'\1_\2', s)
    return blockOfCapitalsRe.sub(r'\1_\2', s1).lower()