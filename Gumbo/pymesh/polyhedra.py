from math import *
from euclid import *

def icosahedron():
    """Construct a 20-sided polyhedron"""
    faces = [ \
        (0,1,2),
        (0,2,3),
        (0,3,4),
        (0,4,5),
        (0,5,1),
        (11,6,7),
        (11,7,8),
        (11,8,9),
        (11,9,10),
        (11,10,6),
        (1,2,6),
        (2,3,7),
        (3,4,8),
        (4,5,9),
        (5,1,10),
        (6,7,2),
        (7,8,3),
        (8,9,4),
        (9,10,5),
        (10,6,1) ]
    verts = [ \
        ( 0.000,  0.000,  1.000 ),
        ( 0.894,  0.000,  0.447 ),
        ( 0.276,  0.851,  0.447 ),
        (-0.724,  0.526,  0.447 ),
        (-0.724, -0.526,  0.447 ),
        ( 0.276, -0.851,  0.447 ),
        ( 0.724,  0.526, -0.447 ),
        (-0.276,  0.851, -0.447 ),
        (-0.894,  0.000, -0.447 ),
        (-0.276, -0.851, -0.447 ),
        ( 0.724, -0.526, -0.447 ),
        ( 0.000,  0.000, -1.000 ) ]
    return verts, faces

def octohedron():
    """Construct an eight-sided polyhedron"""
    f = sqrt(2.0) / 2.0
    verts = [ \
        ( 0, -1,  0),
        (-f,  0,  f),
        ( f,  0,  f),
        ( f,  0, -f),
        (-f,  0, -f),
        ( 0,  1,  0) ]
    faces = [ \
        (0, 2, 1),
        (0, 3, 2),
        (0, 4, 3),
        (0, 1, 4),
        (5, 1, 2),
        (5, 2, 3),
        (5, 3, 4),
        (5, 4, 1) ]
    return verts, faces
    
def subdivide(verts, faces):
    """Subdivide each triangle into four triangles, pushing verts to the unit sphere"""
    triangles = len(faces)
    for faceIndex in xrange(triangles):
    
        # Create three new verts at the midpoints of each edge:
        face = faces[faceIndex]
        a,b,c = (Vector3(*verts[vertIndex]) for vertIndex in face)
        verts.append((a + b).normalized()[:])
        verts.append((b + c).normalized()[:])
        verts.append((a + c).normalized()[:])

        # Split the current triangle into four smaller triangles:
        i = len(verts) - 3
        j, k = i+1, i+2
        faces.append((i, j, k))
        faces.append((face[0], i, k))
        faces.append((i, face[1], j))
        faces[faceIndex] = (k, j, face[2])

    return verts, faces
