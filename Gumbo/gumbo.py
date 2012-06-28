#!/usr/bin/python

from sys import path as module_paths
module_paths.append('./pymesh')

from platform_open import *
from weld import *
import parametric, patch

outfile = "gumbo.ctm"
infile = "gumbo.rib"

print "Loading RIB file..."

GenerateCtm = False

if GenerateCtm:

    from export_ctm import *

    funcs = patch.gen_funcs_from_rib(infile)
    slices, stacks = 8, 8

    print "Evaluating parametric functions..."
    verts, faces = parametric.multisurface(slices, stacks, funcs)

    # Note that creases are not supported by weld().
    print "Welding verts..."
    verts, faces = weld(verts, faces)

    export_raw(verts, faces, outfile)
    platform_open(outfile)

else:

    def dump_tuples(headerFile, mylist):
        for v in mylist:
            s = []
            for value in v[:]:
                if math.floor(value) == value:
                    s.append(str(int(value)))
                else:
                    s.append('%gf' % value)
            headerFile.write("\t{ %s, %s, %s },\n" % tuple(s))

    verts, norms, knots = patch.gen_indexed_knots_from_rib(infile)
    headerFile = open('GumboData.h', 'w')
    headerFile.write('float GumboPositions[%d][3] = {\n' % len(verts));
    dump_tuples(headerFile, verts)
    headerFile.write('};\n\n');
    headerFile.write('float GumboNormals[%d][3] = {\n' % len(verts));
    dump_tuples(headerFile, norms)
    headerFile.write('};\n\n');
    patchCount = len(knots) / 16
    headerFile.write('unsigned short GumboKnots[%d][16] = {\n' % patchCount);
    grouper = [iter(knots)] * 16
    for v in izip(*grouper):
        headerFile.write( "\t{%s},\n" % ", ".join(map(str,v)) )
    headerFile.write('};\n\n');
