#!/bin/bash
$LMBIN=/home/cklein13/git/lm/build/lm

# getopts variables
OPTIND=1         # Reset in case getopts has been used previously in the shell.

show_help() {
cat << EOF
Usage: ${0##*/} [-a lm_args] [-h] [-l log_file] [-n queue] [-q number_cpus] [-s sim_file]
    -a lm_args          arguments for lm, except for -f which is taken care of by sim_file
    -h			        display this help and exit
	-l log_file 	    path to log file
    -n number_cpus      number of cpus to use for the job  
	-q queue		    name of queue
    -s sim_file  lm input/output(?) file
EOF
}                


# Initialize our own variables:
lm_args=""
log_file=""
number_cpus=0
queue=""
sim_file=""

while getopts "hl:q:n:" opt; do
    case "$opt" in
    a)  lm_args=$OPTARG
        ;;
    h)
        show_help
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
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

#echo "lm_args=$lm_args, log_file=$log_file, number_cpus=$number_cpus, queue=$queue, sim_file=$sim_file, Leftovers: $@"

# Copy the simulation file to the scratch directory.
SCRATCHDIR=/tmp
SCRATCHFILE=`basename $`

SCRATCHDIR=$SCRATCHDIR/\$JOB_ID
SCRATCHFILE=\$SCRATCHDIR/$SCRATCHFILE
mkdir -p \$SCRATCHDIR
echo "Copying $SIMFILE to \$SCRATCHFILE"
cp -v $SIMFILE \$SCRATCHFILE

# Print out the machines allocated to the job.
echo "Running in \$SGE_O_WORKDIR"
echo "Job: \$JOB_ID"
echo "Host: \$HOSTNAME"
echo "Num Hosts: \$NHOSTS"
echo "Num Slots: \$NSLOTS"
echo "Nodes:"
cat \$TMPDIR/machines
echo "Resources:"
cat \$TMPDIR/machine-resources

# Create the MPICH node list.
uniq < \$TMPDIR/machines > \$TMPDIR/mpich.hosts
NUMNODES=\`cat \$TMPDIR/mpich.hosts|wc -l\`
echo "MPICH Hosts:"
cat \$TMPDIR/mpich.hosts

# Add the cuda lib directory, if it exists.
if [ -d /usr/local/cuda/lib64 ]; then
    LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/usr/local/cuda/lib64
fi

# Run the job.
echo "Running mpirun -n \$NUMNODES -f \$TMPDIR/mpich.hosts $LMBIN --nodelist=\$TMPDIR/machines $lm_args -f \$SCRATCHFILE"
mpirun -n \$NUMNODES -f \$TMPDIR/mpich.hosts $LMBIN --resource-map=\$TMPDIR/machine-resources $lm_args -f \$SCRATCHFILE

# Copy the results back to the simulation directory.
echo "Copying results back to $SIMFILE"
cp -v \$SCRATCHFILE $SIMFILE && rm \$SCRATCHFILE
if [ -f \$SCRATCHFILE.chk ]; then
    cp -v \$SCRATCHFILE.chk $SIMFILE.chk && rm \$SCRATCHFILE.chk
fi
rmdir --ignore-fail-on-non-empty \$SCRATCHDIR
