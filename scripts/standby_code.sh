#!/bin/bash


if [ "$#" -ne 1 ]; then
    echo "Usage: $0 standbysize"
    exit 1
fi

# home dir
system="parafullnode"
home_dir=$HOME
proj_dir="$home_dir/$system"
script_dir="$proj_dir/scripts"
script="${script_dir}/pretest/bw.py"

cd $proj_dir

cluster="aliyun"
# code="Clay"
# ecn=14
# eck=10
# ecw=256
scenario="standby"
stripes_num=20
blkMB=256
pktKB=64
batchsize=20
numstripes=20
# gendata="false"
bdwt=1000
standbysize=$1
echo "standbysize = $standbysize"

tmplog="$proj_dir/log.txt"
logdir="$proj_dir/log/standby_code"
retdir="$proj_dir/ret/standby_code"
mkdir -p $logdir
mkdir -p $retdir
rm -rf $logdir/*
rm -rf $retdir/*

codevec=("Clay" "Clay" "BUTTERFLY")
n=(12 16 12)
k=(8 12 10)
w=(64 256 512)

# varying standby nodes num
for times in {1..5}; do
    gendata="false"
    for codeid in {0..2}; do
        ecn=${n[codeid]}
        eck=${k[codeid]}
        ecw=${w[codeid]}
        code=${codevec[codeid]}
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
            grep -a -oP 'Coordinator::repair. repairbatch = \K\d+' $tmplog >> $retfile
        done
    done
    exit 1
done
        




