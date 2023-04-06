#!/bin/bash
workdir="/home/yxchen/Projects/CSE240/NUMA-Cache-Sim"
cd $workdir
make

progs=(ts_lock tts_lock ticketlock arraylock arraylock_aligned)
protocols=(MSI MOESI)

# run msi, mesi and moesi sims on given prog name
run_one_sim () {
    prog=$1
    mkdir -p "$workdir/results/$prog"
    for protocol in ${protocols[@]}; do
        echo "Running sim on $prog with protocol $protocol and $threads threads"
        $workdir/sim.out -t $workdir/traces/${prog}${threads}.trace -p ${threads} -n ${threads} -m ${protocol} -A -i > $workdir/results/${prog}/${prog}_${threads}_${protocol}.txt
    done
}


run_sim () {
    for t in 2 4 8 16 32
    do 
        threads=$t
        for prog in ${progs[@]}; do
            mkdir -p $workdir/traces
            mkdir -p $workdir/results/$prog
            run_one_sim $prog
        done
    done
}

run_sim