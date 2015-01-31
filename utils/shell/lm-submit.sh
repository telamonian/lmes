#!/bin/bash
# This script runs lm using the SGE queue submission engine.

# Requirements:
#
# Bash environment variables set (or equivalent):
#  export PATH=/share/apps/bin:"${PATH}"
#  export LD_LIBRARY_PATH=/share/apps/lib:"${LD_LIBRARY_PATH}"
#  export LMLIBDIR=/share/apps/lib/lm

if [ -z "$4" ]; then
  echo "Usage: $0 simulation_file arguments* log_file queue number_cpus"
  exit
fi

# Parse the arguments.
SIMFILE=$1
SCRATCHDIR=/tmp
SCRATCHFILE=`basename $SIMFILE`
ARGS=
for ((i=2; i<=`expr $# - 3`; i++))
do
  eval ARG=\${$i}
  ARGS="$ARGS $ARG"
done
eval LOGFILE=\${`expr $# - 2`}
eval QUEUE=\${`expr $# - 1`}
eval CPUS=\${`expr $#`}

# Make sure we are submitting to a supported queue.
if [ $QUEUE == "gpu-1" ]; then
    
    # Set the pe.
    PE=mpi-cuda
    
    # Figure out which executable to run.
    BINDIR=/home/cklein13/git/lm/build
    LMBIN="$BINDIR/lm -gr 1"
else

    # Set the parallel environemnt.
    PE=mpi
    
    # Figure out which executable to run.
    BINDIR=/share/apps/bin
    LMBIN="$BINDIR/lm -gr 0"
fi

# Create the job file.
JOBFILE=`mktemp /tmp/lmes.XXXXXXXXXXXX`
cat >$JOBFILE <<EOF
#$ -S /bin/bash
#$ -V
#$ -o $LOGFILE
#$ -j y
#$ -m n
#$ -pe $PE $CPUS
#$ -q $QUEUE
#$ -cwd

# Copy the simulation file to the scratch directory.
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
echo "Running mpirun -n \$NUMNODES -f \$TMPDIR/mpich.hosts $LMBIN --nodelist=\$TMPDIR/machines $ARGS -f \$SCRATCHFILE"
mpirun -n \$NUMNODES -f \$TMPDIR/mpich.hosts $LMBIN --resource-map=\$TMPDIR/machine-resources $ARGS -f \$SCRATCHFILE

# Copy the results back to the simulation directory.
echo "Copying results back to $SIMFILE"
cp -v \$SCRATCHFILE $SIMFILE && rm \$SCRATCHFILE
if [ -f \$SCRATCHFILE.chk ]; then
    cp -v \$SCRATCHFILE.chk $SIMFILE.chk && rm \$SCRATCHFILE.chk
fi
rmdir --ignore-fail-on-non-empty \$SCRATCHDIR

EOF

echo "Submitting job to $QUEUE (cpus=$CPUS)"
qsub $JOBFILE && rm -f $JOBFILE




