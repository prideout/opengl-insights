from ctypes import *

def make_blob(verts, T):
    """Convert a list of tuples of numbers into a ctypes pointer-to-array"""
    size = len(verts) * len(verts[0])
    Blob = T * size
    floats = [c for v in verts for c in v]
    blob = Blob(*floats)
    return cast(blob, POINTER(T))
