from math import *
from euclid import *
from itertools import *
import re, os
import math
import pickle
from weld import *

def catmull_rom():
    m = Matrix4()
    m[0:16] = (
        -0.5, 1.5, -1.5, 0.5,
        1, 2.5, 2, -0.5,
        -0.5,  0, 0.5, 0,
        0, 1, 0, 0)
    return m

def hermite():
    m = Matrix4()
    m[0:16] = (
        2,-2, 1, 1,
        -3, 3, -2, -1,
        0, 0, 1, 0,
        1, 0, 0, 0)
    return m

def bspline():
    m = Matrix4()
    m[0:16] = (
        -1/6.,  1/2., -1/2.,  1/6.,
         1/2., -1,     1/2.,  0,
        -1/2.,  0,     1/2.,  0,
         1/6.,  2/3.,  1/6.,  0 )
    return m

def bezier():
    m = Matrix4()
    m[0:16] = (
        -1, 3, -3, 1,
        3, -6, 3, 0,
        -3, 3, 0, 0,
        1, 0, 0, 0 )
    return m

def compute_patch_matrices(knots, B):
    assert(len(knots) == 3)
    return [B * P * B.transposed() for P in knots]

def get_patch_knots(tokens, transform, file):
    """ Parse a RIB line that looks like this:
        Patch "bicubic" "P" [6 0 1 ... 3 0 1]
    """
    basis = tokens[1].strip('"')
    if basis != 'bicubic':
        print "Sorry, I only understand bicubic patches."
        quit()
    tokens = tokens[3:]
    
    # Parse all knots on this line:
    coords = map(float, ifilter(lambda t:len(t)>0, tokens))
            
    # If necessary, continue to the next line, looping until done:
    while len(coords) < 16*3:
        line = file.next()
        tokens = re.split('[\s\[\]]', line)
        c = map(float, ifilter(lambda t:len(t)>0, tokens))
        coords.extend(c)

    # Transform each knot position by the given transform:
    args = [iter(coords)] * 3
    return [transform * Point3(*v) for v in izip(*args)]

def create_patch_coefficients(knots):

    # Split the coordinates into separate lists of X, Y, and Z:
    knots = [c for v in knots for c in v]
    knots = [islice(knots, n, None, 3) for n in (0,1,2)]

    # Arrange the coordinates into 4x4 matrices:
    mats = [Matrix4() for n in (0,1,2)]
    for knot, mat in izip(knots, mats):
        mat[0:16] = list(knot)
    
    return compute_patch_matrices(mats, bezier())

def parse_rib(filename, patchCallback):
    file = open(filename,'r') 
    coefficient_matrices = []
    transform = Matrix4()
    transform_stack = []
    for line in file:
        line = line.strip(' \t\n\r')
        tokens = re.split('[\s\[\]]', line)
        if len(tokens) < 1: continue
            
        action = tokens[0]
        if action == 'TransformBegin':
            transform_stack.append(transform)
        elif action == 'TransformEnd':
            transform = transform_stack[-1]
            transform_stack = transform_stack[:-1]
        elif action == 'Translate':
            xlate = map(float, tokens[1:])
            transform = transform * Matrix4.new_translate(*xlate)
        elif action == 'Scale':
            scale = map(float, tokens[1:])
            transform = transform * Matrix4.new_scale(*scale)
        elif action == 'Rotate':
            angle = float(tokens[1])
            angle = math.radians(angle)
            axis = Vector3(*map(float, tokens[2:]))
            transform = transform * Matrix4.new_rotate_axis(angle, axis)
        elif action == 'Patch':
            knots = get_patch_knots(tokens, transform, file)
            patchCallback(knots)

def make_col_vector(a, b, c, d):
    V = Matrix4()
    V[0:16] = [a, b, c, d] + [0] * 12
    return V

def make_row_vector(a, b, c, d):
    return make_col_vector(a, b, c, d).transposed()

def make_patch_func(Cx, Cy, Cz):
    def patch(u, v):
        U = make_row_vector(u*u*u, u*u, u, 1)
        V = make_col_vector(v*v*v, v*v, v, 1)
        x = U * Cx * V
        y = U * Cy * V
        z = U * Cz * V
        return x[0], y[0], z[0]
    return patch

def gen_funcs_from_rib(filename):
    coefficient_matrices = []
    def patch(knots):
        Cx, Cy, Cz = create_patch_coefficients(knots)
        coefficient_matrices.append((Cx, Cy, Cz))
    parse_rib(filename, patch)
    
    # Create a list of function objects that can be passed to the parametric evaluator:
    return [make_patch_func(Cx, Cy, Cz) for Cx, Cy, Cz in coefficient_matrices]

def gen_indexed_knots_from_rib(filename):

    # Build a list of 16-wide lists:
    verts = []
    def patch(knots):
        verts.append(knots)
    parse_rib(filename, patch)

    # Flatten the lists:
    verts = [c for v in verts for c in v]

    # The initial index buffer is just 0,1,2...
    knots = range(len(verts))

    print "Vert count before welding: ", len(verts)

    if os.path.exists("weldcache"):
        print "Loading weldcache..."
        flatverts, knots = pickle.load( open("weldcache") )
        args = [iter(flatverts)] * 3
        verts = [Point3(*v) for v in izip(*args)]
    else:
        verts, knots = weld(verts, knots)
        print "Saving weldcache..."
        flatverts = [c for v in verts for c in v]
        cache = (flatverts, knots)
        pickle.dump( cache, open("weldcache", "wb") )

    print "Vert count after welding: ", len(verts)
    print "Index count: ", len(knots)

    patchCount = len(knots) / 16
    quadCount = 9 * patchCount
    print "Quad count: ", quadCount

    # Consider Renderman's 16-point (left-handed) patch.
    # This shows the 16 vertex indices and the 9 quad indices:
    #
    #    0----1----2------3 
    #    |    |    |      |
    #    | 0  | 1  |   2  |
    #    |    |    |      | 
    #    4----5----6------7 
    #    |    |    |      |
    #    | 3  | 4  |  5   | 
    #    |    |    |      |
    #    8----9----10----11 
    #    |    |    |      |
    #    | 6  | 7  |  8   | 
    #    |    |    |      |
    #    12---13---14----15 
    #

    # Build a 9-entry lookup table.
    # Maps from a quad index to the vertex indices at its 4 corners.
    cornerMap = []
    for n in xrange(9):
        i = 4 * (n / 3) + (n % 3)
        cornerMap.append((i, 1+i, 4+i, 5+i))

    # Generate the vertex-to-quads map and compute facet normals.
    print "Building topology map..."
    quadNeighbors = [[] for v in verts]
    quadNormals = []
    for q in xrange(quadCount):
        corners = []
        for i in cornerMap[q % 9]:
            v = knots[i + (q / 9) * 16]
            quadNeighbors[v] += [q]
            corners.append(verts[v])
        a, b, c, d = corners
        n = (c - a).cross(b - a).normalized()
        quadNormals.append(n)

    # Average together the facet normals.  Creases are not supported.
    print "Computing smooth normals..."
    norms = []
    for vertex, quads in izip(verts, quadNeighbors):
        normal = Vector3(0, 0, 0)
        for quad in quads:
            normal += quadNormals[quad]
        normal.normalize()
        norms.append(normal)

    return verts, norms, knots
