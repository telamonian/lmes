from importlib import import_module
from pkgutil import iter_modules

__all__ = ['ShallowImportAll', 'ShallowImportAllModules', 'ShallowImportAllPackages']

def AllAttrCheck(mod):
    return hasattr(mod, '__all__')

def DefImportedProp(mod, clsName, subModName=None):
    if subModName==None or subModName=='':
        @property
        def prop(self):
            return mod.__getattribute__(clsName)
    else:
        @property
        def prop(self):
            
            return mod.__getattribute__(clsName)
    return prop
#     @prop.setter
#     def prop(self, val):
#         self.__setattr__(spec['targetName'], val)
#     dct[name] = prop

def ShallowImportAll(path, name, modCheck=AllAttrCheck, moduleOnly=False, pkgOnly=False):
    '''
    imports everything in every submodule's/subpkg's __all__ into the local namespace of the calling package
    skips things without an explicit __all__ defined
    the recursion is set up to be shallow, ie only over the first level of submodules/subpkgs (for deep, we could use walk_packages instead of iter_modules)
    if every subpkg also implements this function in its __init__.py, the recursion effectively becomes deep
    use in a __init__.py file verbatim like this:
        localDict, allList = ShallowImportAll(path=__path__, name=__name__)
        locals().update(localDict)
        __all__+=allList
    '''
    localDict = {}
    allList = []
    for importer, modname, ispkg in iter_modules(path=path, prefix=name+'.'):
        if modname.split('.')[-1][:4]=='old_':
            # this is disabled code, skip it
            continue
        if (moduleOnly and ispkg):
            continue
        if (pkgOnly and not ispkg):
            continue
        mod = import_module(modname)
        if not modCheck(mod):
            # POI: we should maybe add a way to completely unload modules we're not interested in here, but a quick check of SO shows that's hard to do
            del mod
            continue
        else:
            for subModName,allSeq in [(var.split('__all__')[1], mod.__getattribute__(var)) for var in vars(mod) if len(var.split('__all__')) > 1]:
                for clsName in allSeq:
                    localDict[clsName] = DefImportedProp(mod, clsName, subModName) #mod.__getattribute__(clsName)
                allList+=mod.allSeq
    return localDict, allList

def ShallowImportAllModules(path, name, modCheck=AllAttrCheck):
    ShallowImportAll(path, name, modCheck, moduleOnly=True)

def ShallowImportAllPackages(path, name, modCheck=AllAttrCheck):
    ShallowImportAll(path, name, modCheck, pkgOnly=True)