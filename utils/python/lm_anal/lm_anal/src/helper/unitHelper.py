__all__funcs = ['InchesToPoints', 'PointsToInches']
__all__vals = ['POINTS_PER_INCH']

__all__ = __all__funcs + __all__vals

POINTS_PER_INCH = 72.27

# points <=> inches
def InchesToPoints(inches):
    return inches*POINTS_PER_INCH

def PointsToInches(points):
    return points*(1/POINTS_PER_INCH)