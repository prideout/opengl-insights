from math import *
from euclid import *

def perp(u):
    """Randomly picks a reasonable perpendicular vector"""
    v = Vector3(1, 0, 0);
    u_prime = u.cross(v)
    if u_prime.magnitude_squared() < 0.01:
        v = Vector3(0, 1, 0);
        u_prime = u.cross(v)
    return u_prime.normalized()
    
def tube(u, v, func, radius):
    
    # Compute three basis vectors
    p1 = Vector3(*func(u))
    p2 = Vector3(*func(u + 0.01))
    A = (p2 - p1).normalized()
    B = perp(A)
    C = A.cross(B).normalized()

    # Rotate the Z-plane circle appropriately
    m = Matrix4.new_rotate_triple_axis(B, C, A)
    spoke_vector = m * Vector3(cos(2*v), sin(2*v), 0)

    # Add the spoke vector to the center to obtain the rim position.
    center = p1 + radius * spoke_vector
    return center[:]

def granny_path(t):
    t = 2 * t
    x = -0.22 * cos(t) - 1.28 * sin(t) - 0.44 * cos(3 * t) - 0.78 * sin(3 * t)
    y = -0.1 * cos(2 * t) - 0.27 * sin(2 * t) + 0.38 * cos(4 * t) + 0.46 * sin(4 * t)
    z = 0.7 * cos(3 * t) - 0.4 * sin(3 * t)
    return x, y, z
    
def granny(u, v):
    return tube(u, v, granny_path, radius = 0.1)
