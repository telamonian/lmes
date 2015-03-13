import bisect as bi
import matplotlib.cm as cm
import matplotlib.pyplot as plt
import numpy as np
import os

cm.hot.set_bad(cm.hot(0))
# stuff to make the perceptual color maps
pCMap = {}
rPCMap = {}
walk = os.walk(os.path.join(os.path.dirname(os.path.realpath(__file__)),'colormaps')).next()
for fname in (fname for fname in walk[2] if fname[0]!='.'):
    with open(os.path.join(walk[0],fname)) as f:
        cVals = np.array([map(float,line.strip().split(',')) for line in f])
        # Setting up columns for tuples
        b3 = cVals[:,2]
        b2 = cVals[:,2]
        b1 = np.linspace(0, 1, len(b2))
        g3 = cVals[:,1]
        g2 = cVals[:,1]
        g1 = np.linspace(0,1,len(g2))
        r3 = cVals[:,0]
        r2 = cVals[:,0]
        r1 = np.linspace(0,1,len(r2))
        # Creating tuples
        R = zip(r1,r2,r3)
        G = zip(g1,g2,g3)
        B = zip(b1,b2,b3)
        # Transposing
        RGB = zip(R,G,B)
        rgb = zip(*RGB)
        # Creating color maps
        k = ['red', 'green', 'blue']
        pCMap[fname.split('.')[0]] = matplotlib.colors.LinearSegmentedColormap(fname.split('.')[0], dict(zip(k,rgb)))
        pCMap[fname.split('.')[0]].set_bad(cVals[0,:]) # set RGB value for bad pixels (like the zeros in log-norm hist2d plots)
        # Creating reverse color maps
        rgb.reverse()
        rPCMap[fname.split('.')[0]] = matplotlib.colors.LinearSegmentedColormap(fname.split('.')[0], dict(zip(k,rgb)))
        rPCMap[fname.split('.')[0]].set_bad(cVals[0,:]) # set RGB value for bad pixels (like the zeros in log-norm hist2d plots)

# determine whether font color of an annotation should be black or white depending on background
def AnnotationColor(r, g, b, a=1.0):
    # Counting the perceptive luminance - human eye favors green color... 
    darknessCoefficient = 1 - ( 0.299 * r + 0.587 * g + 0.114 * b);

    if (darknessCoefficient < 0.5):
       d = 0 # bright background color, so use black font
    else:
       d = 1 # dark background color, so use white font

    return (d, d, d, a)

def chunks(l, n):
    if n < 1:
        n = 1
    return [l[i:i + n] for i in range(0, len(l), n)]

def clear_frame(ax=None): 
    # Taken from a post by Tony S Yu
    if ax is None: 
        ax = plt.gca() 
    ax.xaxis.set_visible(False) 
    ax.yaxis.set_visible(False) 
    for spine in ax.spines.itervalues(): 
        spine.set_visible(False) 

# Interface to LineCollection:
def colorline(x, y, z=None, cmap=pCMap['cube1'], norm=plt.Normalize(0.0, 1.0), linewidth=3, alpha=1.0, ax=None, zdir=None, zs=0):
    '''
    Plot a colored line with coordinates x and y
    Optionally specify colors in the array z
    Optionally specify a colormap, a norm function and a line width
    '''
    # Default colors equally spaced on [0,1]:
    if z is None:
        z = np.linspace(0.0, 1.0, len(x))
           
    # Special case if a single number:
    if not hasattr(z, "__iter__"):  # to check for numerical input -- this is a hack
        z = np.array([z])
        
    z = np.asarray(z)
    
    segments = make_segments(x, y)
    lc = LineCollection(segments, array=z, cmap=cmap, norm=norm, linewidth=linewidth, alpha=alpha)
    
    if ax is None:
        ax = plt.gca()
    
    if zdir is None:
        ax.add_collection(lc)
    else:
        ax.add_collection3d(lc, zdir=zdir, zs=zs)
    
    return lc        
    
# Data manipulation:
def make_segments(x, y):
    '''
    Create list of line segments from x and y coordinates, in the correct format for LineCollection:
    an array of the form   numlines x (points per line) x 2 (x and y) array
    '''
    points = np.array([x, y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)
    
    return segments