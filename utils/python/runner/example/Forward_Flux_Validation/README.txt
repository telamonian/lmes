INFO:

Batch run scripts for the Forward Flux Validation project. They're all based on variants of the genetic toggle switch. All of the code, including the models, needed to rerun the simulations for this project are supplied. Meant to be used in conjunction with lm_runner (which can be found in the Lattice Microbes source code at utils/python/runner).

INSTRUCTIONS:

To use one of the scripts, do the following

1. Change the variables in the "user defined" section at the top of the script to appropriate values:
	host: the ssh hostname or url of the computer you're trying to run the Lattice Microbes jobs on (you'll have to have previously set up public key authentication (ie passwordless log on) on the computer you're trying to use). Can be set to 'localhost' to use the local computer.

	lm_bin: the path to the Lattice Microbes executable on the host.

	local_home_directory: you shouldn't need to change this.

	remote_home_directory: your home directory on the host. Used to help determine where simulation output data goes (actual destination fully specified in the 'rootPath' entry in the sweep_dict towards the end of the script)

	type: specify 'shell' to run the jobs sequentially from a bash shell on the host, or 'sge' to submit the jobs to a Sun Grid Engine scheduler instance running on the host (for cluster hosts, you'll probably want 'sge'. 'shell' is included mostly for testing purposes).

	user_id: your user ID (ie login name) on the host.

	runnerPath: path to the lm_runner files. As mentioned above, can be found in the Lattice Microbes source code at utils/python/runner

2. Run as you would any python script. If python complains about a missing dependency (eg "ImportError: No module named 'saga') try installing it with pip (eg "pip install saga-python")
