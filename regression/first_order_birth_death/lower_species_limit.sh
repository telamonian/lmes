dirname=data/2
filename=${dirname}/first_order_birth_death.lm
mkdir -p ${dirname}
rm -f ${filename} && lm_sbml_import ${filename} first_order_birth_death.sbml
lm_setp ${filename} writeInterval=1e-2 maxTime=1e4 "speciesLowerLimitList=0:900"
#~/git/lm_main/build/lmes -r 1-1000 -f ${filename} -gr 0 -c 2 #> ${filename}.log
