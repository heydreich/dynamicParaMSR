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
# bdwt=1000
standbysize=$1
echo "standbysize = $standbysize"

tmplog="$proj_dir/log.txt"
logdir="$proj_dir/log/standby_bdwt"
retdir="$proj_dir/ret/standby_bdwt"
mkdir -p $logdir
mkdir -p $retdir
rm -rf $logdir/*
rm -rf $retdir/*


# varying standby nodes num
for times in {1..5}; do
    for bdwt in {10000,1000,500,100}; do
        bdwt=100
        for method in {balance,centralize,offline}; do
            key="${code}_${ecn}_${eck}_${ecw}_${bdwt}_${method}_${standbysize}"
            logfile="$logdir/${key}_${times}"
            retfile="$retdir/$key"  
            echo "python $script $cluster $code $ecn $eck $ecw $method $scenario $blkMB $pktKB $batchsize $numstripes false $bdwt $standbysize"
            date 
            timeout 20m python $script $cluster $code $ecn $eck $ecw $method $scenario $blkMB $pktKB $batchsize $numstripes false $bdwt $standbysize &> demo
            cp $tmplog $logfile
            grep -oP 'Coordinator::repair. repairbatch = \K\d+' $tmplog
            grep -oP 'Coordinator::repair. repairbatch = \K\d+' $tmplog >> $retfile
            echo
        done  
        exit 1     
    done
    
done
        




