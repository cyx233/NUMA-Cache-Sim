#!/bin/bash
cachesim_path="/mnt"
pin_path="/mnt/pin"
cd $cachesim_path/programs
make

# cp ~/cachesim/util/pinatrace.cpp ~/pin/source/tools/ManualExamples
cd $pin_path/source/tools/ManualExamples
make

progs=(ts_lock tts_lock ticketlock arraylock arraylock_aligned)
protocols=(MSI MOESI)

generate_traces () {
    cd $pin_path/source/tools/ManualExamples
    mkdir -p $cachesim/traces
    for prog in ${progs[@]}; do
        echo "Generating trace for ${prog} with ${threads} threads"
        outfile=$cachesim_path/traces/${prog}${threads}.trace
        ../../../pin -t obj-intel64/pinatrace.so -o $outfile -- $cachesim_path/programs/${prog}.out ${threads}

        # check that the file ends in the eof str
        eof_str=$(tail -n 1 $outfile)
        if [[ $eof_str != "#eof" ]]
        then echo "Failed to generate trace - out of memory"
                exit 1
        fi

        # run the sim on it
        if [ $sim = true ]
        then
            run_one_sim $prog
            echo "Removing ${outfile}"
            rm $outfile
        fi
    done
}

for t in 2 4 8 16 32
do 
    threads=$t
    generate_traces
done