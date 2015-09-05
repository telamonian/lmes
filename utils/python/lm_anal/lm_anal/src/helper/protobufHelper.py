from pathlib import Path
import os,sys

__all__ = ['AppendProtobufPath', 'DirectionEnum', 'LifecycleEnum']

# stuff for setting path to protobuf python stuff (we'll keep trying to find some way to make direct manipulation of sys.path unnecessary...)
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
pythonProtobufPath = Path(os.path.join(thisScriptDir, '../../python_protobuf')).resolve()

def AppendProtobufPath():
    if str(pythonProtobufPath) not in sys.path:
        sys.path.append(str(pythonProtobufPath))
        
AppendProtobufPath()

from lm_anal.python_protobuf.lm.io.FFluxOutput_pb2 import FFluxOutput
DirectionEnum = FFluxOutput.Direction 
LifecycleEnum = FFluxOutput.Lifecycle