#!/bin/sh

#SBATCH -J "lm"
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=24
#SBATCH --ntasks-per-node=1
#SBATCH -D /home-1/cklein13@jhu.edu/git/lm_ndarray/regression/genetic_toggle_switch
#SBATCH -o lm_fflux.log
#SBATCH -e lm_fflux.err
#SBATCH -t 168:00:00
#SBATCH -p parallel
#SBATCH --mail-user=cklein13@jhu.edu
#SBATCH --exclusive
#SBATCH --export=ALL
#SBATCH --mail-type=end
#SBATCH --exclude=compute0253,compute0480,compute0500

## ENVIRONMENT
export SAGA_HOSTNAME=marcc

python fflux.py  -f genetic_toggle_switch.lm,fixedStartPointInput.sfile -ff sfile -fo genetic_toggle_switch.sfile -gr 0 -t .1 -c 24 -cr 1
