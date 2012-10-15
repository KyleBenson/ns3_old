#!/bin/bash

runs=`seq 1 20`
fail_probs='0.1' # 0.2 0.3
AS_choices='6461' #1755 3967 
disasters[1755]='Amsterdam,_Netherlands London,_UnitedKingdom Paris,_France'
disasters[3967]='Herndon,_VA Irvine,_CA Santa_Clara,_CA'
disasters[6461]='San_Jose,_CA Los_Angeles,_CA New_York,_NY'

for pfail in $fail_probs;
do
    for AS in $AS_choices;
    do
	for disaster in ${disasters[$AS]}
	do
	    rseed=`date +%s`
	    for i in $runs
	    do
		out_dir=ron_output/$pfail/$AS/$disaster
		mkdir --parent $out_dir
	    #echo $out_dir/run$i.out
		./waf --run 'ron --file=rocketfuel/maps/'$AS'.cch --disaster='$disaster' --RngSeed='$rseed' --RngRun='$i' --fail_prob='$pfail' --latencies=rocketfuel/weights/'$AS'/latencies.intra --trace_file='$out_dir/run$i.out' --contact_attempts=20 --timeout=0.5 --verbose=1'
		
		#quit after first run if argument is 'test'
		if [ "$1" == "test" ]
		    then exit
		fi
	    done
	done
    done
done


