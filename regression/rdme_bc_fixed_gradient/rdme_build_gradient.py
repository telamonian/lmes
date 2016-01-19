import sys
import os
import h5py

if len(sys.argv) != 8:
    quit("Usage: filename xsize ysize zsize axis c1 c2")

filename=sys.argv[1]
xsize=int(sys.argv[2])+2
ysize=int(sys.argv[3])+2
zsize=int(sys.argv[4])+2
axis=sys.argv[5]
c1=float(sys.argv[6])
c2=float(sys.argv[7])

f = h5py.File(filename, "a")
g = f.create_dataset("/Model/Diffusion/Gradient", (xsize,ysize,zsize), dtype='d')
g[:,:,:]=0.0

if axis == "x":
    for x in range(0,xsize):
        c=c1+(c2-c1)*float(x)/float(xsize-1)
        for y in range(0,ysize):
            for z in range(0,zsize):
                g[x,y,z]=c

elif axis == "y":
    for y in range(0,ysize):
        c=c1+(c2-c1)*float(y)/float(ysize-1)
        for x in range(0,xsize):
            for z in range(0,zsize):
                g[x,y,z]=c
    

elif axis == "z":
    for z in range(0,zsize):
        c=c1+(c2-c1)*float(z)/float(zsize-1)
        for x in range(0,xsize):
            for y in range(0,ysize):
                g[x,y,z]=c    

f.close()

