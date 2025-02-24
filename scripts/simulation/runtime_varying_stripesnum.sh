#!/bin/bash
# algorithm run time

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 standbysize"
    exit 1
fi

# home dir
home_dir=$HOME
proj_dir="$home_dir/parafullnode"
script_dir="$proj_dir/scripts"
simulation_dir="$script_dir/simulation"

tmplog="$proj_dir/log.txt"
logdir="$proj_dir/log/runtime"
retdir="$proj_dir/ret/runtime"
mkdir -p $logdir
mkdir -p $retdir
rm -rf $logdir/*
rm -rf $retdir/*

# executable_file_path
executable_file_parallel="$proj_dir/build/Sim_balance"
gen_simulation_data="$script_dir/data/gen_simulation_data.py"

# param
agents_num=20
# stripes_num=20
code="Clay"
ecn=14
eck=10
ecw=256
# scenario="standby"
batchsize=20
standbysize=$1
echo "standbysize = $standbysize"


for i in {1..10}; do
	for stripes_num in {100,200,300,400,500,600,700,800,900,1000}; do 
		key="${code}_${ecn}_${eck}_${ecw}_${stripes_num}"
	    scatter_log="$logdir/${key}_parallel_$i"
		standby_log="$logdir/${key}_standby_$i"
        scatter_ret="$retdir/$key_parallel_$i"
		standby_ret="$retdir/$key_standby_$i"  

		# gen placement
		echo "for stripesnum = $stripes_num  $i times"
		python $gen_simulation_data $agents_num $stripes_num $ecn $eck 0  
		simulation_placement="$proj_dir/stripeStore/simulation_${agents_num}_${stripes_num}_${ecn}_${eck}"
		
		# scatter
		# echo "$executable_file_parallel $simulation_placement $agents_num $stripes_num $code $ecn $eck $ecw scatter $batchsize 0  0 &> demo"
		rm $tmplog
		$executable_file_parallel $simulation_placement $agents_num $stripes_num $code $ecn $eck $ecw scatter $batchsize 0  0 &> demo
		cp $tmplog $scatter_log
		grep -oP 'duration = \K\d+' $tmplog
		grep -oP 'duration = \K\d+' $tmplog >> $scatter_ret
		echo
		

		# standby
		rm $tmplog
		$executable_file_parallel $simulation_placement $agents_num $stripes_num $code $ecn $eck $ecw standby $batchsize 0  $standbysize &> demo
		cp $tmplog $standby_log
		grep -oP 'duration = \K\d+' $tmplog
		grep -oP 'duration = \K\d+' $tmplog >> $standby_ret
		echo	
	done
	exit 1 
done
