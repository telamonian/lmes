#!/bin/bash

# getopts variables
OPTIND=1         # Reset in case getopts has been used previously in the shell.

show_help() {
cat << EOF
Usage: ${0##*/} [-a lm_args] [-h] [-l log_file] [-n queue] [-q number_cpus] [-s sim_file] [-x lm_bin]
    -a lm_args          arguments for lm, except for -f which is taken care of by sim_file
    -h			        display this help and exit
	-l log_file 	    path to log file
    -n number_cpus      number of cpus to use for the job  
	-q queue		    name of queue
    -s sim_file         lm input/output(?) file
    -x lm_bin           path to lattice microbes executable
EOF
}

# Initialize our own variables:
log_file=""
queue=""

job_manager=false
lm_args=""
lm_bin=""
number_cpus=0
sim_file=""

while getopts "a:hl:n:q:s:x:" opt; do
    case "$opt" in
    a)  lm_args=$OPTARG
        ;;
    h)  show_help
        exit 0
        ;;
	l)  log_file=$OPTARG
        ;;
    n)  number_cpus=$OPTARG
        ;;
    q)  queue=$OPTARG
        ;;
    s)  sim_file=$OPTARG
        ;;
    x)  lm_bin=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
echo "lm_args=$lm_args, lm_bin=$lm_bin, log_file=$log_file, number_cpus=$number_cpus, queue=$queue, sim_file=$sim_file, Leftovers: $@"

# Copy the simulation file to the scratch directory.
SCRATCHDIR=/tmp
SCRATCHFILE=`basename $sim_file`
JOB_UUID=`mktemp -u XXXXXXXXXXX`

SCRATCHDIR=$SCRATCHDIR/$JOB_UUID
SCRATCHFILE=$SCRATCHDIR/$SCRATCHFILE
mkdir -p $SCRATCHDIR
echo "Copying $sim_file to $SCRATCHFILE"
cp -v $sim_file $SCRATCHFILE

if [ -n "$SGE_TASK_ID" ] ; then
    # Print out the machines allocated to the job.
	echo "Running in $SGE_O_WORKDIR"
	echo "Job: $JOB_ID"
	echo "Host: $HOSTNAME"
	echo "Num Hosts: $NHOSTS"
	echo "Num Slots: $NSLOTS"
	# if the Nodes and/or Resources files exist (depends on how the PE is set up), print them
	if [ -f $TMPDIR/machines ]; then
		echo "Nodes:"
		cat $TMPDIR/machines
	fi
	if [ -f $TMPDIR/machine-resources ]; then
		echo "Resources:"
    	cat $TMPDIR/machine-resources
	fi
	
    # Create the MPICH node list.
	uniq < $TMPDIR/machines > $TMPDIR/mpich.hosts
	NUMNODES=`cat $TMPDIR/mpich.hosts|wc -l`
	echo "MPICH Hosts:"
	cat $TMPDIR/mpich.hosts
	
    node_file_option="-f $TMPDIR/mpich.hosts"
    
    if [[ $SAGA_HOSTNAME == "kirin" ]]; then
        lm_nodelist_option="--nodelist=$TMPDIR/machines"
        mpi_bin="mpiexec -launcher ssh"
    else
        lm_resource_map_option="--resource-map=$TMPDIR/machine-resources"
        mpi_bin="mpirun"
    fi    
    
else # this job is running in a normal shell
    NUMNODES=1
    lm_args="$lm_args -c $number_cpus"
fi

# Add the cuda lib directory, if it exists.
if [ -d /usr/local/cuda/lib64 ]; then
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64
fi

# Run the job.
echo "Running $mpi_bin -n $NUMNODES $node_file_option $lm_bin $lm_resource_map_option $lm_nodelist_option $lm_args -f $SCRATCHFILE"
$mpi_bin -n $NUMNODES $node_file_option $lm_bin $lm_resource_map_option $lm_nodelist_option $lm_args -f $SCRATCHFILE

# Copy the results back to the simulation directory.
echo "Copying results back to $sim_file"
cp -v $SCRATCHFILE $sim_file && rm $SCRATCHFILE
if [ -f $SCRATCHFILE.chk ]; then
    cp -v $SCRATCHFILE.chk $sim_file.chk && rm $SCRATCHFILE.chk
fi
rm -r $SCRATCHDIR
