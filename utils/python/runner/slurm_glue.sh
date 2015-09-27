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
if [ -n "$SLURM_JOBID" ] ; then
	JOB_UUID=$SLURM_JOBID
else
	JOB_UUID=`mktemp -u XXXXXXXXXXX`
fi

SCRATCHDIR=$SCRATCHDIR/$JOB_UUID
SCRATCHFILE=$SCRATCHDIR/$SCRATCHFILE
mkdir -p $SCRATCHDIR
echo "Copying $sim_file to $SCRATCHFILE"
cp -v $sim_file $SCRATCHFILE

if [ -n "$SLURM_JOBID" ] ; then
    # Print out the machines allocated to the job.
	echo "Running in $SLURM_SUBMIT_DIR"
	echo "Job: $SLURM_JOBID"
	echo "Host: $SLURM_SUBMIT_HOST"
	echo "Nodes: $SLURM_JOB_NODELIST"
	echo "CPUs: $SLURM_CPUS_ON_NODE"
	
    # Create the MPICH node list.
	scontrol show hostname $SLURM_JOB_NODELIST > $SCRATCHDIR/mpich.hosts
	NUMNODES=`cat $SCRATCHDIR/mpich.hosts|wc -l`
	echo "MPICH Hosts:"
	cat $SCRATCHDIR/mpich.hosts
	
	node_file_option="-f $SCRATCHDIR/mpich.hosts"

#    if [[ $SAGA_HOSTNAME == "kirin" ]]; then
#        lm_nodelist_option="--nodelist=$TMPDIR/machines"
#        mpi_bin="mpiexec -launcher ssh"
#    else
#        lm_resource_map_option="--resource-map=$TMPDIR/machine-resources"
#        mpi_bin="mpirun"
#    fi    
    
else # this job is running in a normal shell
    NUMNODES=1
    lm_args="$lm_args -c $number_cpus"
fi

# Add the cuda lib directory, if it exists.
if [ "${QUEUE}" == "gpu" ]; then
    echo \$LD_LIBRARY_PATH
    echo \$PATH
    module list
    module load cuda
    echo \$LD_LIBRARY_PATH
    echo \$PATH
    module list
fi

mpi_bin="mpirun"

# Run the job.
echo "Running $mpi_bin -n $NUMNODES $node_file_option $lm_bin $lm_resource_map_option $lm_nodelist_option $lm_args -f $sim_file"
$mpi_bin -n $NUMNODES $node_file_option $lm_bin $lm_resource_map_option $lm_nodelist_option $lm_args -f $sim_file

# Copy the results back to the simulation directory.
rm \$SCRATCHDIR/mpich.hosts
rm -r $SCRATCHDIR
echo "Done."
