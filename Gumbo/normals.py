from sympy import *

def Mat4(s):
    return Matrix(4,4,lambda i,j:Symbol(s + str(i) + str(j)))

def Syms(s):
    return map(Symbol,s.split())

def Mats(s):
    return map(Mat4,s.split())

def Vec3(*v):
    return Matrix(3,1,v)

u,v=Syms('u v')
x,y,z,w=Syms('x y z w')
P = Matrix(4,1,[x,y,z,w])
U = Matrix(1,4, [u*u*u, u*u, u, 1])
V = Matrix(4,1, [v*v*v, v*v, v, 1])
cx,cy,cz = Mats('cx cy cz')
correct = diff((U*cx*V)[0], u)
dU = Matrix(1,4, [3*u*u, 2*u, 1, 0])
guess = (dU*cx*V)[0]
print guess - correct
print "Hurray they're equal!"

f = Vec3(
        (U*cx*V)[0],
        (U*cy*V)[0],
        (U*cz*V)[0])
dfdu = Vec3(*[diff(g, u) for g in f])
dfdv = Vec3(*[diff(g, v) for g in f])
print dfdu.cross(dfdv)
