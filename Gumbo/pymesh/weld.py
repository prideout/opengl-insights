from math import *
from euclid import *
from itertools import *

def weld(verts, faces, epsilon = 0.00001):
    """Find duplicated verts and merge them"""
    
    # Create the remapping table: (this is the slow part)
    count = len(verts)
    remap_table = range(count)
    for i1 in xrange(0, count):

        # Crude progress indicator:
        if i1 % (count / 10) == 0:
            print (count - i1) / (count / 10)
    
        p1 = verts[i1]
        if not p1: continue

        for i2 in xrange(i1 + 1, count):
            p2 = verts[i2]
            if not p2: continue

            if abs(p1[0] - p2[0]) < epsilon and \
                abs(p1[1] - p2[1]) < epsilon and \
                abs(p1[2] - p2[2]) < epsilon:
                remap_table[i2] = i1
                verts[i2] = None

    # Remove holes from the vert list:
    newverts = []
    shift = 0
    shifts = []
    for v in xrange(count):
        if verts[v]:
            newverts.append(verts[v])
        else:
            shift = shift + 1
        shifts.append(shift)
    verts = newverts
    print "Reduced from %d verts to %d verts" % (count, len(verts))

    # Apply the remapping table:
    if type(faces[0]) is tuple:
        faces = [[x - shifts[x] for x in (remap_table[y] for y in f)] for f in faces]
    else:
        faces = [x - shifts[x] for x in [remap_table[i] for i in faces]]

    return verts, faces
