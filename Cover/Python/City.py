#!/usr/bin/python

ThreadCount = 4
ShadingRate = 8 # Bigger is faster
ImageFormat = (350*4/2,500*4/2,1)
PixelSamples = (1,1)
OccSamples = 256
Crop = True

import sys
import os
import time
import LSystem
import euclid
from struct import pack

if 'RMANTREE' in os.environ:
    from sys import path as module_paths
    path = os.environ['RMANTREE'] + '/bin'
    module_paths.append(path)

import prman

def SetLabel( self, label, groups = '' ):
    """Sets the id and ray group(s) for subsequent gprims"""
    self.Attribute(self.IDENTIFIER,{self.NAME:label})
    if groups != '':
        self.Attribute("grouping",{"membership":groups})

def Compile(shader):
    """Compiles the given RSL file"""
    print 'Compiling %s...' % shader
    retval = os.system("shader %s.sl" % shader)
    if retval:
        quit()

def CreateBrickmap(base):
    """Creates a brick map from a point cloud"""
    if os.path.exists('%s.bkm' % base):
        print "Found brickmap for %s" % base
    else:
        print "Creating brickmap for %s..." % base
        if not os.path.exists('%s.ptc' % base):
            print "Error: %s.ptc has not been generated." % base
        else:
            os.system("brickmake %s.ptc %s.bkm" % (base, base))

def Clean():
    """Removes build artifacts from the current directory"""
    from glob import glob
    filespec = "*.slo *.bkm *.ptc *.xml *.tif *.mov *.pyc"
    patterns = map(glob, filespec.split(" "))
    for files in patterns:
        map(os.remove, files)
    
def ReportProgress():
    """Communicates with the prman process, printing progress"""
    previous = progress = 0
    while progress < 100:
        prman.RicProcessCallbacks()
        progress = prman.RicGetProgress()
        if progress == 100 or progress < previous:
            break
        if progress != previous:
            print "\r%04d - %s%%" % (ReportProgress.counter, progress),
            previous = progress
            time.sleep(0.1)
    print "\r%04d - 100%%" % ReportProgress.counter
    ReportProgress.counter += 1
ReportProgress.counter = 0

Cages = []
Curves = []

def DrawScene(ri, time):
    """Everything between RiBegin and RiEnd"""

    frameString = "%04d" % DrawScene.counter
    filename = "Art%s.tif" % frameString
    DrawScene.counter += 1

    bakeArgs = dict(filename="City.ptc",samples=OccSamples)
    bakeArgs['displaychannels'] = '_occlusion'

    if Crop:
        left = 0
        right = 1
        top = 0.2
        bottom = 0.85
        ri.CropWindow(left, right, top, bottom)

    ri.Option("limits", {"int threads" : ThreadCount})
    ri.Display("panorama", "framebuffer", "rgba")
    ri.Format(*ImageFormat)
    ri.DisplayChannel("float _occlusion")
    ri.ShadingRate(ShadingRate)
    ri.PixelSamples(*PixelSamples)
    ri.Projection(ri.PERSPECTIVE, {ri.FOV: 30})
    #ri.Imager("PanoramicDistortion", {"background": (0.0/255.0,165.0/255.0,211.0/255.0)})

    ri.WorldBegin()

    ri.Translate(0, 0, 20)
    ri.Rotate(-20, 1, 0, 0)
    ri.Translate(0, 0, -20)

    ri.Declare("samples", "float")
    ri.Declare("displaychannels", "string")
    ri.Declare("coordsys", "string")
    ri.Attribute("cull", dict(hidden=0,backfacing=0))
    ri.Attribute("dice", dict(rasterorient=0))
    ri.Attribute("visibility", dict(diffuse=1,specular=1))

    width = 0.15
    height = 0.1
    sharpness = 2.0

    ri.SetLabel('Sculpture')
    bakeArgs['color em'] = (1.5,1.5,1.5)
    bakeArgs['coordsys'] = 'object'
    ri.Surface("AmbientOcclusion", bakeArgs)
    tree = open('City.xml').read()
    shapes = []
    seed = 76
    for x in xrange(10):
        shapes += LSystem.Evaluate(tree, seed)
        seed = seed + 1
    ri.TransformBegin()
    ri.Translate(0,0,15.0) # 2.5 for panoromic
    ri.Rotate(45, 0, 1, 0)
    ri.CoordinateSystem("Sculpture")
    for shape in shapes:
        if shape == None:
            continue
        P, U, V, W = shape
        if P.y < 0.0:
            continue
        if P.y > 0.25 and P.x * P.x + P.z * P.z > 1.0:
            continue

        if False:
            u = U.normalized()
            v = V.normalized()
            w = W.normalized()
            if abs(u.y) > 0.9: pass
            elif abs(v.y) > 0.9: (U, V, W) = (V, U, W)
            elif abs(w.y) > 0.9: (U, V, W) = (W, U, V)

        corners = []
        corners += (P - U - V - W)[:] # 0
        corners += (P + U - V - W)[:] # 1
        corners += (P - U + V - W)[:] # 2
        corners += (P + U + V - W)[:] # 3
        corners += (P - U - V + W)[:] # 4
        corners += (P + U - V + W)[:] # 5
        corners += (P - U + V + W)[:] # 6
        corners += (P + U + V + W)[:] # 7

        ri.PointsPolygons([4, 4, 4, 4 , 4, 4],
                          [2,3,1,0, 1,5,4,0, 7,6,4,5, 3,2,6,7,  0,4,6,2, 7,5,1,3],
                          {ri.P:corners})

    if False:
        ri.SetLabel('RightWall')
        bakeArgs['color em'] = (0.0/255.0,165.0/255.0,211.0/255.0)
        ri.Surface("AmbientOcclusion", bakeArgs)
        corners = []
        P = euclid.Point3(0, 0, 0)
        U = euclid.Vector3(10, 0, 0)
        V = euclid.Vector3(0, 10, 0)
        W = euclid.Vector3(0, 0, 0)
        corners += (P - U - V - W)[:]
        corners += (P + U - V - W)[:]
        corners += (P - U + V - W)[:]
        corners += (P + U + V - W)[:]
        ri.PointsPolygons([4,], [2,3,1,0], {ri.P:corners})
    
        ri.SetLabel('LeftWall')
        ri.Surface("AmbientOcclusion", bakeArgs)
        corners = []
        P = euclid.Point3(0, 0, 0)
        U = euclid.Vector3(0, 0, 10)
        V = euclid.Vector3(0, 10, 0)
        W = euclid.Vector3(0, 0, 0)
        corners += (P - U - V - W)[:]
        corners += (P + U - V - W)[:]
        corners += (P - U + V - W)[:]
        corners += (P + U + V - W)[:]
        ri.PointsPolygons([4,], [2,3,1,0], {ri.P:corners})

    ri.SetLabel('Floor')
    bakeArgs['color em'] = (0.0/255.0,165.0/255.0,211.0/255.0)
    ri.Surface("AmbientOcclusion", bakeArgs)
    corners = []
    P = euclid.Point3(0, 0, 0)
    U = euclid.Vector3(0, 0, 10)
    V = euclid.Vector3(10, 0, 0)
    W = euclid.Vector3(0, 0, 0)
    corners += (P - U - V - W)[:]
    corners += (P + U - V - W)[:]
    corners += (P - U + V - W)[:]
    corners += (P + U + V - W)[:]
    ri.PointsPolygons([4], [2,3,1,0], {ri.P:corners})

    # I started on try to export to GLBO format
    # but "easy_install python-lzf" does not work
    # and I decided to abondon this effort in the
    # interest of time
    if False:
        vertices = [euclid.Point3(1,1,1)]
        indices = [0, 0, 0]
        GL_UNSIGNED_SHORT = 0x1403
        GL_FLOAT = 0x1406
        vertices = [pack('fff', *v[:]) for v in vertices]
        indices = [pack('H', i) for i in indices]
        frames = [vertices]
        attrib = \
            pack('PiiIiP',
                 0,
                 3,
                 GL_FLOAT,
                 3 * 4,
                 len(frames),
                0)
        attribs = [attrib]
        header = ''
        header += \
             pack('iiiiQ',
                  len(attribs),
                  len(indices),
                  len(vertices),
                  GL_UNSIGNED_SHORT,
                  len(indices) * 2)

    # Write contents to a simple binary file
    if True:
        i = 0
        vertices = []
        normals = []
        for shape in shapes:
            if shape == None:
                continue
            P, U, V, W = shape
            if P.y < 0.0:
                continue
            if P.y > 0.25 and P.x * P.x + P.z * P.z > 1.0:
                continue

            u = U.normalized()
            v = V.normalized()
            w = W.normalized()
            if abs(u.y) > 0.9: pass
            elif abs(v.y) > 0.9: (U, V, W) = (V, U, W)
            elif abs(w.y) > 0.9: (U, V, W) = (W, U, V)

            corners = []
            corners.append(P - U - V - W)
            corners.append(P + U - V - W)
            corners.append(P - U + V - W)
            corners.append(P + U + V - W)
            corners.append(P - U - V + W)
            corners.append(P + U - V + W)
            corners.append(P - U + V + W)
            corners.append(P + U + V + W)
            faces = [2,3,1,0, 1,5,4,0, 7,6,4,5,     3,2,6,7,     0,4,6,2, 7,5,1,3];

            U.normalize()
            V.normalize()
            W.normalize()

            facets =[W,W,W,W, V,V,V,V, -W,-W,-W,-W, -V,-V,-V,-V, U,U,U,U, -U,-U,-U,-U]

            faces = [corners[x] for x in faces]
            faces = [c for v in faces for c in v] # flatten
            facets = [c for v in facets for c in v] # flatten

            #if len(faces) != 72: raise Exception("Internal issue")
            #if len(facets) != 72: raise Exception("Internal issue")

            vertices.append(pack('72f', *faces))
            normals.append(pack('72f', *facets))

        vertexCount = len(vertices) * 72 / 3

        outfile = open('City.dat', 'wb')
        outfile.write(pack('i', vertexCount))
        outfile.write(''.join(vertices))
        outfile.write(''.join(normals))
        outfile.close()

    ri.TransformEnd()
    ri.WorldEnd()
DrawScene.counter = 0

if __name__ == "__main__":

    if sys.argv[-1] == "clean":
        Clean()
        quit()

    Compile('AmbientOcclusion')
    Compile('PanoramicDistortion')
    prman.Ri.SetLabel = SetLabel
    ri = prman.Ri()

    ri.Begin("launch:prman? -ctrl $ctrlin $ctrlout")
    DrawScene(ri, 0)
    ReportProgress()
    ri.End()
