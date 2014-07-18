#!/bin/bash


exec > /var/log/addGrants.log
exec 2> /var/log/addGrants.log

/usr/local/bin/icmGrants/icmGrants.py > /tmp/asd

/usr/local/slurm/current/bin/sacctmgr -i load  file=/tmp/asd 

sacctmgr -i  add account slurmmon
sacctmgr -i add user slurmmon account=slurmmon defaultaccount=slurmmon



