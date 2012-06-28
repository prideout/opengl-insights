from sys import path as module_paths
module_paths.append('../pyctm')
module_paths.append('./pyctm')

from ctypes import *
from blob import *
from openctm import *

def export_ctm(verts, faces, outfile):
    pVerts = make_blob(verts, c_float)
    pFaces = make_blob(faces, c_uint)
    pNormals = POINTER(c_float)()
    ctm = ctmNewContext(CTM_EXPORT)

    ctmCompressionLevel(ctm, 9) # 0 to 9; default is 1
    #ctmVertexPrecision # only matters for MG2; defaults to 0.00098
    #ctmCompressionMethod(ctm, CTM_METHOD_MG2) # MG1 or MG2

    ctmDefineMesh(ctm, pVerts, len(verts), pFaces, len(faces), pNormals)
    ctmSave(ctm, outfile)
    ctmFreeContext(ctm)

def export_raw(verts, faces, outfile):
    pVerts = make_blob(verts, c_float)
    pFaces = make_blob(faces, c_uint)
    pNormals = POINTER(c_float)()
    ctm = ctmNewContext(CTM_EXPORT)
    ctmCompressionMethod(ctm, CTM_METHOD_RAW)
    ctmDefineMesh(ctm, pVerts, len(verts), pFaces, len(faces), pNormals)
    ctmSave(ctm, outfile)
    ctmFreeContext(ctm)
