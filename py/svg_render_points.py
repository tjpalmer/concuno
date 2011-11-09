#!/usr/bin/env python


"""
From lists of points, renders an svg file.
"""


def find_bounds(points):
    from numpy import hstack, zeros
    # Get copies to modify for max and min.
    locations = points[:, :-2]
    if locations.shape[1] == 1:
        locations = hstack((locations, zeros(locations.shape)))
    maxs = locations + points[:, -2:-1]
    mins = locations - points[:, -2:-1]
    maxs = maxs.max(0)
    mins = mins.min(0)
    bounds = hstack((mins, maxs - mins))
    return bounds


def main():
    from numpy import array, loadtxt
    from sys import argv
    file = argv[1]
    points = loadtxt(file)
    maxs = points.max(0)
    mins = points.min(0)
    bounds = find_bounds(points)
    tick = 0.01 * min(bounds[2:])
    ranges = maxs - mins
    def draw_cross(color):
        print (
            "    "
            "<path stroke-width='%g' stroke='%s'"
            " d='M 0 -%g V %g M -%g 0 H %g'/>" % (
                (tick, color) + (3 * tick,) * 4))
    print (
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='%g %g %g %g'>" %
            tuple(bounds))
    print "  <title>%s</title>" % file # TODO Escape < and &!
    for i in xrange(points.shape[0]):
        point = points[i, :]
        location = point[0:-2]
        radius = point[-2]
        value = (point[-1] - mins[-1]) / ranges[-1]
        if location.size == 1:
            location = [location[0], 0]
        color = "rgb(%g%%, %g%%, %g%%)" % tuple(
            100 * array([value, 0, 1 - value]))
        print "  <g transform='translate(%g %g)'>" % tuple(location)
        draw_cross(color)
        print (
            "    "
            "<circle r='%g' fill='%s' fill-opacity='0'"
            " stroke-width='%g' stroke='%s'/>" % (
                (radius, color, tick, color)))
        print "  </g>"
    # Origin.
    draw_cross("black")
    print "</svg>"
    #print maxs
    #print mins
    #print ranges


if __name__ == '__main__':
    main()
