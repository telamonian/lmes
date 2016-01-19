import sys
import os

if len(sys.argv) != 2:
    quit("Usage: simulation_filename")

simulationFilename=sys.argv[1]

xlen=3e-6
ylen=4e-6
zlen=5e-6
spacing=1e-6

builder=LatticeBuilder(xlen,ylen,zlen,spacing,1,0)
topType=0
top=Cuboid(point(0.0,0.0,0.0), point(xlen,ylen,spacing*2), topType)
builder.addRegion(top)
midType=1
mid=Cuboid(point(0.0,0.0,spacing*2), point(xlen,ylen,spacing*3), midType)
builder.addRegion(mid)
botType=2
bot=Cuboid(point(0.0,0.0,spacing*3), point(xlen,ylen,zlen), botType)
builder.addRegion(bot)

# Species types.
A=0
B=1
C=2

# Add the particles.
builder.addParticles(A, topType, 20);
builder.addParticles(B, botType, 20);

# Make sure the file exists.
if not os.path.isfile(simulationFilename):
    quit("Simulation file must already exist.")

# Open the file.
sim=SimulationFile(simulationFilename)
spatialModel=SpatialModel()
builder.getSpatialModel(spatialModel)
sim.setSpatialModel(spatialModel)

# Discretize the lattice.
diffusionModel=DiffusionModel()
sim.getDiffusionModel(diffusionModel)
lattice = ByteLattice(diffusionModel.lattice_x_size(), diffusionModel.lattice_y_size(), diffusionModel.lattice_z_size(), diffusionModel.lattice_spacing(), diffusionModel.particles_per_site())
builder.discretizeTo(lattice, 0, 0.0)
sim.setDiffusionModel(diffusionModel)
sim.setDiffusionModelLattice(diffusionModel, lattice)

sim.close()

