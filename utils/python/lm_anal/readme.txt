Design statement:
-The hierarchy of objects in lm_anal that represents Lattice Microbes data has three levels
	-Replicate
		-Mostly relevant for replicate sampling
		-Won't be used for say, fflux simulations
	-Sim
		-The data from one complete set of lm simulations. Should be equivalent to what's found in a .lm file
	-Sweep
		-The data from a parameter sweep of lm simulations. Will be equivalent to a set of .lm files, organized as a grid according to parameter value