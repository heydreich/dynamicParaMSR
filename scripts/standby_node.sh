#!/bin/bash

# home dir
system="parafullnode"
home_dir=$HOME
proj_dir="$home_dir/$system"
script_dir="$proj_dir/scripts"
script="${script_dir}/pretest/bw.py"

cd $proj_dir

cluster="aliyun"
code="Clay"
ecn=14
eck=10
ecw=256
scenario="standby"
stripes_num=20
blkMB=256
pktKB=64
batchsize=20
numstripes=20
# gendata="false"
bdwt=1000

tmplog="$proj_dir/log.txt"
logdir="$proj_dir/log/standby_node"
retdir="$proj_dir/ret/standby_node"
mkdir -p $logdir
mkdir -p $retdir
rm -rf $logdir/*
rm -rf $retdir/*

gendata="false"
# varying standby nodes num
for times in {1..5}; do
    for standbysize in {1..4}; do
        for method in {balance,centralize,offline}; do
            key="${code}_${ecn}_${eck}_${ecw}_${standbysize}_${method}"
            logfile="$logdir/${key}_${times}"
            retfile="$retdir/$key"
            echo "python $script $cluster $code $ecn $eck $ecw $method $scenario $blkMB $pktKB $batchsize $numstripes $gendata $bdwt $standbysize"
            date
            timeout 10m python $script $cluster $code $ecn $eck $ecw $method $scenario $blkMB $pktKB $batchsize $numstripes $gendata $bdwt $standbysize &> demo
            cp $tmplog $logfile
            echo $logfile
            grep -a -oP 'Coordinator::repair. repairbatch = \K\d+' $tmplog
            grep -oP 'Coordinator::repair. repairbatch = \K\d+' $tmplog >> $retfile
        done
    done
    exit 1
done
        




