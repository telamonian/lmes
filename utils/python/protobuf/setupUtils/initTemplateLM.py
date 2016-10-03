import importlib, pkgutil, re
__path__ = pkgutil.extend_path(__path__, __name__)

def _ShallowImport(path, name, nameFilter=None, nameFilterClusivity='include', skipModules=False, skipPkgs=False, shortNames=False, _deep=False, onerror=None):
    '''
    imports every module and package in path into the name namespace
    use in a __init__.py file verbatim like this:
        modDict = ShallowImport(path=__path__, name=__name__, shortNames=True)
        locals().update(modDict)
    '''
    if nameFilter is not None: nameFilter = re.compile(nameFilter)
    iterFunc = pkgutil.walk_packages if _deep else pkgutil.iter_modules

    moduleDict = {}
    for importer, modName, isPkg in iterFunc(path=path, prefix=name+'.', onerror=onerror):
        if modName.split('.')[-1][:4]=='old_':
            # this is disabled code, skip it
            continue
        if skipPkgs and isPkg:
            # we've been told to skip packages and this is a package
            continue
        if skipModules and not isPkg:
            # we've been told to skip modules and this is a module
            continue
        if nameFilter is not None:
            if nameFilterClusivity=='exclude':
                # if modName matches a specified filter, skip this mod
                if nameFilter.search(modName):
                    continue
            elif nameFilterClusivity=='include':
                # if modName does not match a specified filter, skip this mod
                if not nameFilter.search(modName):
                    continue
        mod = importlib.import_module(modName)

        if shortNames:
            # strip any parent packages off of the mod's dot-name
            modName = modName.split(name+'.')[-1]
        moduleDict[modName] = mod

    return moduleDict

def _DeepImport(path, name, nameFilter=None, nameFilterClusivity='include', skipModules=False, skipPkgs=False, shortNames=False, onerror=None):
    return _ShallowImport(path=path, name=name, nameFilter=nameFilter, nameFilterClusivity=nameFilterClusivity, skipModules=skipModules, skipPkgs=skipPkgs, shortNames=shortNames, _deep=True, onerror=onerror)

def _GenMsgTypeDict(path=__path__, name=__name__, onerror=None):
    msgDict = {}
    for _protoModule in _DeepImport(path=path, name=name, nameFilter='_pb[0-9]', skipPkgs=True, onerror=onerror).values():
        try:
            for _msgDescriptor in _protoModule.DESCRIPTOR.message_types_by_name.values():
                # from IPython.core.debugger import Tracer; Tracer()()
                msgDict[_msgDescriptor.full_name] = _msgDescriptor._concrete_class
        except AttributeError:
            pass
    return msgDict

_msgTypeDict = _GenMsgTypeDict()

# get the protobufs from robertslab.pbuf, if we can
try:
    import robertslab

    # function that we pass down (ultimately to pkgutil.walk_packages) so that any possible errors are skipped
    def _skipOnerror(name):
        pass

    _msgTypeDict.update(_GenMsgTypeDict(name=robertslab.__name__, path=robertslab.__path__, onerror=_skipOnerror))
except ImportError:
    pass

def GetMsgType(fullName):
    '''
    get the protobuf python type from the full name of a protobuf msg type (parent packages and msg name joined by '.', eg lm.message.WorkUnitOutput)
    '''
    return _msgTypeDict[fullName]