# aspects of this build system are based on https://github.com/PySide/pyside-setup
from __future__ import print_function

from distutils.errors import DistutilsSetupError
from distutils.spawn import find_executable
import os
from setuptools import find_packages, setup
from setuptools.command.develop import develop
from setuptools.command.egg_info import egg_info
from setuptools.command.install import install
import sys

sys.path.insert(0, '.')
from setupUtils import CopyCode, CopyDirStructure, GetSetupSrcDir, InitInit, OptionValue, RGlob, RunProcess, WrappedMakedirs

# Globals! Hooray!
setup_src_dir = GetSetupSrcDir()

package_dir = {'': 'build',
               'lm': 'build/lm',
               'robertslab': 'build/robertslab'}
thisScriptDir = os.path.dirname(os.path.realpath(__file__))

protoSrcDir = os.path.realpath(os.path.join(setup_src_dir, '../../../src/protobuf'))
protoBuildDir = os.path.join(thisScriptDir, 'build')

lmSrcDir = os.path.join(protoSrcDir, 'lm')
lmSupDir = os.path.join(thisScriptDir, 'lm')
lmBuildDir = os.path.join(protoBuildDir, 'lm')

robertslabSrcDir = os.path.join(protoSrcDir, 'robertslab')
robertslabBuildDir = os.path.join(protoBuildDir, 'robertslab')

initInitPaths = [lmBuildDir, robertslabBuildDir]
initInitTemplatedPaths = [(lmBuildDir, 'initTemplateLM.py'),
                          (robertslabBuildDir, 'initTemplateRobertslab.py')]

if 'PROTOC' in os.environ:
    OPTION_PROTOC = os.environ['PROTOC']
else:
    OPTION_PROTOC = OptionValue("protoc")
if OPTION_PROTOC is None:
    OPTION_PROTOC = find_executable("protoc")
if OPTION_PROTOC is None or not os.path.exists(OPTION_PROTOC):
    raise DistutilsSetupError(
        "Failed to find protoc."
        " Please specify the path to protoc with --protoc parameter.")

class CustomEggInfoCommand(egg_info):
    '''
    customized egg_info command class
    deals with the fact that the build dir containing the compiled protobuf files might not exist initially
    '''
    initInitPaths = initInitPaths
    initInitTemplatedPaths = initInitTemplatedPaths
    package_dir = package_dir

    def __init__(self, dist, **kw):
        [WrappedMakedirs(dirPath) for dirPath in self.package_dir.values()]
        CopyDirStructure(lmSrcDir, lmBuildDir)
        CopyDirStructure(robertslabSrcDir, robertslabBuildDir)
        [InitInit(dirPath, recursive=True) for dirPath in self.initInitPaths]
        [InitInit(dirPath, template=template) for dirPath,template in self.initInitTemplatedPaths]

        egg_info.__init__(self, dist, **kw)

class CustomSetupCommand:
    '''
    customized setup command base class
    meant to be used in a subclass that also inherits either setuptools.command.install.install or .develop
    '''

    def run(self, srcDir, buildDir):
        self._build_extension(srcDir, buildDir)

    def _build_extension(self, srcDir, buildDir):
        print('using protoc at %s' % OPTION_PROTOC)

        WrappedMakedirs(buildDir)
        protoSrcPaths = RGlob(srcDir, '*.proto')

        if not protoSrcPaths:
            # protoSrcPaths is empty, something went wrong with locating the .proto files
            raise DistutilsSetupError('Error locating protobuf source (.proto) files')

        # Compile protobuf files to python
        protoc_python_cmd = [
            OPTION_PROTOC,
            '--proto_path=%s' % srcDir,
            '--python_out=%s' % buildDir
        ]
        protoc_python_cmd.extend(protoSrcPaths)

        if RunProcess(protoc_python_cmd)!=0:
            raise DistutilsSetupError('Error compiling protobuf files to Python')

class CustomDevelopCommand(CustomSetupCommand, develop):
    def run(self):
        CustomSetupCommand.run(self, protoSrcDir, protoBuildDir)
        CopyCode(lmSupDir, lmBuildDir, symlink=True)
        develop.run(self)

class CustomInstallCommand(CustomSetupCommand, install):
    def run(self):
        CustomSetupCommand.run(self, protoSrcDir, protoBuildDir)
        CopyCode(lmSupDir, lmBuildDir)
        install.run(self)

setup(
    author = 'Elijah Roberts, Max Klein',
    cmdclass = {'develop': CustomDevelopCommand,
                'egg_info': CustomEggInfoCommand,
                'install': CustomInstallCommand},
    description = 'python modules compiled from protobuf sources for projects in the Roberts Lab',
    license = 'UIOSL',
    name = "robertslab-protobuf",
    package_dir = package_dir,
    packages = find_packages(where='./build'),
    zip_safe = False,
)