from importlib import import_module
from pkgutil import iter_modules

__all__ = ['ShallowImport', 'ShallowImportModules', 'ShallowImportPackages', 'ShallowImportAll', 'ShallowImportAllModules', 'ShallowImportAllPackages']

def AllAttrCheck(mod):
    return hasattr(mod, '__all__')

def DefaultCheck(mod):
    '''
    default function to be used in modCheck, always returns True
    '''
    return True

def DefImportedProp(name, mod):
    @property
    def prop(self):
        return mod.__getattribute__(name)
    return prop
#     @prop.setter
#     def prop(self, val):
#         self.__setattr__(spec['targetName'], val)
#     dct[name] = prop

def ShallowImport(path, name, modCheck=DefaultCheck, moduleOnly=False, pkgOnly=False, outputAll=True):
    '''
    imports every module and package in path into the name namespace
    '''
    localDict = {}
    allList = []
    for importer, modName, isPkg in iter_modules(path=path, prefix=name+'.'):
        if modName.split('.')[-1][:4]=='old_':
            # this is disabled code, skip it
            continue
        if (moduleOnly and isPkg):
            continue
        if (pkgOnly and not isPkg):
            continue
        mod = import_module(modName)
        if not modCheck(mod):
            # POI: we should maybe add a way to completely unload modules we're not interested in here, but a quick check of SO shows that's hard to do
            del mod
            continue
        modShortName = modName.split(name+'.')[-1]
        localDict[modShortName] = mod
        allList+=modShortName
    if outputAll:
        return localDict, allList
    else:
        return localDict

def ShallowImportModules(path, name, modCheck=DefaultCheck, outputAll=True):
    return ShallowImport(path, name, modCheck, moduleOnly=True, outputAll=outputAll)

def ShallowImportPackages(path, name, modCheck=DefaultCheck, outputAll=True):
    return ShallowImport(path, name, modCheck, pkgOnly=True, outputAll=outputAll)

def ShallowImportAll(path, name, modCheck=AllAttrCheck, moduleOnly=False, pkgOnly=False):
    '''
    imports everything in every submodule's/subpkg's __all__ into the local namespace of the calling package
    skips things without an explicit __all__ defined (this behavior can be changed by passing a testing function in the modCheck arg)
    the recursion is set up to be shallow, ie only over the first level of submodules/subpkgs (for deep, we could use walk_packages instead of iter_modules)
    if every subpkg also implements this function in its __init__.py, the recursion effectively becomes deep
    use in a __init__.py file verbatim like this:
        localDict, allList = ShallowImportAll(path=__path__, name=__name__)
        locals().update(localDict)
        __all__+=allList
    '''
    localDict = {}
    allList = []
    for importer, modName, isPkg in iter_modules(path=path, prefix=name+'.'):
        if modName.split('.')[-1][:4]=='old_':
            # this is disabled code, skip it
            continue
        if (moduleOnly and isPkg):
            continue
        if (pkgOnly and not isPkg):
            continue
        mod = import_module(modName)
        if not modCheck(mod):
            # POI: we should maybe add a way to completely unload modules we're not interested in here, but a quick check of SO shows that's hard to do
            del mod
            continue
        for clsName in mod.__all__:
            localDict[clsName] = mod.__getattribute__(clsName) #DefImportedProp(clsName, mod) #
        allList+=mod.__all__
    return localDict, allList

def ShallowImportAllModules(path, name, modCheck=AllAttrCheck):
    return ShallowImportAll(path, name, modCheck, moduleOnly=True)

def ShallowImportAllPackages(path, name, modCheck=AllAttrCheck):
    return ShallowImportAll(path, name, modCheck, pkgOnly=True)