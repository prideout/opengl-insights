from sys import path as module_paths
module_paths.append('../pyctm')
module_paths.append('./pyctm')

from ctypes import *
from blob import *
from openctm import *

def import_ctm(infile):
    ctm = ctmNewContext(CTM_IMPORT)
    ctmLoad(ctm, infile)
    vertexCount = ctmGetInteger(ctm, CTM_VERTEX_COUNT);
    faceCount = ctmGetInteger(ctm, CTM_TRIANGLE_COUNT);
    
    facePointer = ctmGetIntegerArray(ctm, CTM_INDICES);
    vertPointer = ctmGetFloatArray(ctm, CTM_VERTICES)
    
    verts = [(vertPointer[3*i], vertPointer[3*i+1], vertPointer[3*i+2]) for i in xrange(vertexCount)]
    faces = [(facePointer[3*i], facePointer[3*i+1], facePointer[3*i+2]) for i in xrange(faceCount)]
    
    ctmFreeContext(ctm)
    return (verts, faces)