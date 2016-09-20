import importlib
import pkgutil
__path__ = pkgutil.extend_path(__path__, __name__)

def ShallowImport(path, name, skipModules=False, skipPkgs=False, shortNames=False, _deep=False):
    '''
    imports every module and package in path into the name namespace
    use in a __init__.py file verbatim like this:
        modDict = ShallowImport(path=__path__, name=__name__, shortNames=True)
        locals().update(modDict)
    '''
    iterFunc = pkgutil.walk_packages if _deep else pkgutil.iter_modules

    moduleDict = {}
    for importer, modName, isPkg in iterFunc(path=path, prefix=name+'.'):
        if modName.split('.')[-1][:4]=='old_':
            # this is disabled code, skip it
            continue
        if (skipPkgs and isPkg):
            continue
        if (skipModules and not isPkg):
            continue
        mod = importlib.import_module(modName)

        if shortNames:
            modName = modName.split(name+'.')[-1]
        moduleDict[modName] = mod

    return moduleDict

def DeepImport(path, name, skipModules=False, skipPkgs=False, shortNames=False):
    return ShallowImport(path=path, name=name, skipModules=skipModules, skipPkgs=skipPkgs, shortNames=shortNames, _deep=True)

_msgDict = {}
for _protoModule in DeepImport(path=__path__, name=__name__, skipPkgs=True).values():
    try:
        for _msgDescriptor in _protoModule.DESCRIPTOR.message_types_by_name.values():
            # from IPython.core.debugger import Tracer; Tracer()()
            _msgDict[_msgDescriptor.full_name] = _msgDescriptor._concrete_class
    except AttributeError:
        pass

def GetMsgByFullName(fullName):
    return _msgDict[fullName]