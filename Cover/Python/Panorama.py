#!/usr/bin/python

ThreadCount = 4
ShadingRate = 4 # Bigger is faster
ImageFormat = (350*3/2,500*3/2,1)
PixelSamples = (1,1)
OccSamples = 4
Crop = None  # left, top, width, height

import sys
import os
import time
import LSystem
import euclid

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

    bakeArgs = dict(filename="AO.ptc",samples=OccSamples)
    bakeArgs['displaychannels'] = '_occlusion'

    if Crop:
        crop = (Crop[0] / ImageFormat[0], 
                (Crop[0] + Crop[2]) / ImageFormat[0],
                Crop[1] / ImageFormat[1],
                (Crop[1] + Crop[3]) / ImageFormat[1])
        ri.CropWindow(*crop)

    ri.Option("limits", {"int threads" : ThreadCount})
    ri.Display("panorama", "framebuffer", "rgba")
    ri.Format(*ImageFormat)
    ri.ShadingRate(ShadingRate)
    ri.PixelSamples(*PixelSamples)
    ri.Projection(ri.PERSPECTIVE, {ri.FOV: 30})
    #ri.Imager("PanoramicDistortion", {"background": (0.0/255.0,165.0/255.0,211.0/255.0)})
    ri.WorldBegin()
    ri.Declare("samples", "float")
    ri.Declare("displaychannels", "string")
    ri.Attribute("cull", dict(hidden=0,backfacing=0))
    ri.Attribute("dice", dict(rasterorient=0))
    ri.Attribute("visibility", dict(diffuse=1,specular=1))

    ri.SetLabel('Floor')
    bakeArgs['color em'] = (0.0/255.0,165.0/255.0,211.0/255.0)
    ri.Surface("AmbientOcclusion", bakeArgs)
    ri.TransformBegin()
    ri.Rotate(90, 1, 0, 0)
    ri.Disk(0.7, 30, 360)
    ri.TransformEnd()

    width = 0.15
    height = 0.1
    sharpness = 2.0

    ri.SetLabel('Sculpture')
    bakeArgs['color em'] = (1.5,1.5,1.5)
    ri.Surface("AmbientOcclusion", bakeArgs)
    tree = open('City.xml').read()
    shapes = []
    seed = 76
    for x in xrange(5):
        shapes += LSystem.Evaluate(tree, seed)
        seed = seed + 1
    ri.TransformBegin()
    ri.Translate(-1.25,0,20.0) # 2.5 for panoromic
    ri.Rotate(45, 0, 1, 0)
    for shape in shapes:
        if shape == None:
            continue
        P, U, V, W = shape
        if P.y > 0:
            continue
        corners = []
        corners += (P - U - V - W)[:]
        corners += (P + U - V - W)[:]
        corners += (P - U + V - W)[:]
        corners += (P + U + V - W)[:]
        corners += (P - U - V + W)[:]
        corners += (P + U - V + W)[:]
        corners += (P - U + V + W)[:]
        corners += (P + U + V + W)[:]
        ri.PointsPolygons([4, 4, 4, 4, 4, 4],
                          [2,3,1,0, 1,5,4,0, 7,6,4,5, 3,2,6,7, 0,4,6,2, 7,5,1,3],
                          {ri.P:corners})
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
