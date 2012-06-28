import random
import sys
from euclid import *
from elementtree.ElementTree import ElementTree as etree
from elementtree.ElementTree import fromstring

def Evaluate(rules, seed = 0):
    """
    Takes an XML string (see the Library) and return a list of shapes.
    Each shape is a 2-tuple: (shape name, transform matrix).
    """

    def radians(d):
        return float(d * 3.141 / 180.0)
    
    def pick_rule(tree, name):
    
        rules = tree.findall("rule")
        elements = []
        for r in rules:
            if r.get("name") == name:
                elements.append(r)

        if len(elements) == 0:
            print "Error, no rules found with name '%s'" % name
            quit()
    
        sum, tuples = 0, []
        for e in elements:
            weight = int(e.get("weight", 1))
            sum = sum + weight
            tuples.append((e, weight))
        n = random.randint(0, sum - 1)
        for (item, weight) in tuples:
            if n < weight:
                break
            n = n - weight
        return item

    def parse_xform(xform_string):
        matrix = Matrix4.new_identity()
        tokens = xform_string.split(' ')
        t = 0
        while t < len(tokens) - 1:
            command, t = tokens[t], t + 1

            # Translation
            if command == 'tx':
                x, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_translate(x, 0, 0)
            elif command == 'ty':
                y, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_translate(0, y, 0)
            elif command == 'tz':
                z, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_translate(0, 0, z)
            elif command == 't':
                x, t = float(tokens[t]), t + 1
                y, t = float(tokens[t]), t + 1
                z, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_translate(x, y, z)

            # Rotation
            elif command == 'rx':
                theta, t = radians(float(tokens[t])), t + 1
                matrix *= Matrix4.new_rotatex(theta)
            elif command == 'ry':
                theta, t = radians(float(tokens[t])), t + 1
                matrix *= Matrix4.new_rotatey(theta)
            elif command == 'rz':
                theta, t = radians(float(tokens[t])), t + 1
                matrix *= Matrix4.new_rotatez(theta)

            # Scale
            elif command == 'sx':
                x, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_scale(x, 1, 1)
            elif command == 'sy':
                y, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_scale(1, y, 1)
            elif command == 'sz':
                z, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_scale(1, 1, z)
            elif command == 'sa':
                v, t = float(tokens[t]), t + 1
                x, y, z = v, v, v
                matrix *= Matrix4.new_scale(x, y, z)
            elif command == 's':
                x, t = float(tokens[t]), t + 1
                y, t = float(tokens[t]), t + 1
                z, t = float(tokens[t]), t + 1
                matrix *= Matrix4.new_scale(x, y, z)

            else:
                print "unrecognized transformation: '%s' at position %d in '%s'" % (command, t, xform_string)
                quit()

        return matrix

    random.seed(seed)
    tree = fromstring(rules)
    entry = pick_rule(tree, "entry")
    shapes = []
    stack = []
    stack.append((entry, 0, Matrix4.new_identity()))
    max_depth = int(tree.get("max_depth"))

    progressCount = 0
    print "Evaluating Lindenmayer system",
    while len(stack) > 0:

        if len(shapes) > progressCount + 1000:
            print ".",
            progressCount = len(shapes)
    
        rule, depth, matrix = stack.pop()

        local_max_depth = max_depth
        if "max_depth" in rule.attrib:
            local_max_depth = int(rule.get("max_depth"))

        if len(stack) >= max_depth:
            shapes.append(None)
            continue

        if depth >= local_max_depth:
            if "successor" in rule.attrib:
                successor = rule.get("successor")
                rule = pick_rule(tree, successor)
                stack.append((rule, 0, matrix))
            shapes.append(None)
            continue
    
        for statement in rule:
            xform = parse_xform(statement.get("transforms", ""))
            count = int(statement.get("count", 1))
            for n in xrange(count):
                matrix *= xform
                if statement.tag == "call":
                    rule = pick_rule(tree, statement.get("rule"))
                    cloned_matrix = matrix.copy()
                    stack.append((rule, depth + 1, cloned_matrix))
                elif statement.tag == "instance":
                    name = statement.get("shape")
                    if name == "curve":
                        P = Point3(0, 0, 0)
                        N = Vector3(0, 0, 1)
                        P = matrix * P
                        N = matrix.upperLeft() * N
                        shapes.append((P, N))
                    elif name == "box":
                        P = matrix * Point3(0, 0, 0)
                        X = 0.2
                        U = matrix.upperLeft() * Vector3(X, 0, 0)
                        V = matrix.upperLeft() * Vector3(0, X, 0)
                        W = matrix.upperLeft() * Vector3(0, 0, X)
                        shapes.append((P, U, V, W))
                    else:
                        shape = (name, matrix)
                        shapes.append(shape)
                else:
                    print "malformed xml"
                    quit()

    print "\nGenerated %d shapes." % len(shapes)
    return shapes
