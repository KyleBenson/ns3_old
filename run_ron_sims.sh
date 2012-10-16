#!/bin/bash

runs=`seq 1 20`
fail_probs='0.1 0.2 0.3 0.5 0.7 0.9'
AS_choices='3356 2914 1239' #Level 3, Verio, Sprintlink
#'6461 1755 3967'
disasters[1755]='Amsterdam,_Netherlands London,_UnitedKingdom Paris,_France'
disasters[3967]='Herndon,_VA Irvine,_CA Santa_Clara,_CA'
disasters[6461]='San_Jose,_CA Los_Angeles,_CA New_York,_NY'
disasters[3356]='Miami,_FL New_York,_NY Los_Angeles,_CA'
disasters[2914]='New_York,_NY New_Orleans,_LA Irvine,_CA'
disasters[1239]='Washington,_DC Dallas,_TX New_York,_NY'
overlay_regions='0 1'

for pfail in $fail_probs;
do
    for AS in $AS_choices;
    do
	for disaster in ${disasters[$AS]}
	do
	    for local_overlays in $overlay_regions
	    do
		rseed=`date +%s`
		for i in $runs
		do
		    out_dir=ron_output/$pfail/$AS/$disaster/`if [ "$local_overlays" == '0' ]; then echo external; else echo internal; fi;`
		    mkdir --parent $out_dir
               	    #echo $out_dir/run$i.out
		    ./waf --run "ron --file=rocketfuel/maps/$AS.cch --disaster=$disaster --RngSeed=$rseed --RngRun=$i --fail_prob=$pfail --latencies=rocketfuel/weights/all_latencies.intra --trace_file=$out_dir/run$i.out --contact_attempts=20 --timeout=0.5 --use_local_overlays=$local_overlays`if [ "$1" == "test" ]; then echo ' --verbose=1'; fi`"
		    
         		#quit after first run if argument is 'test'
		    if [ "$1" == "test" ];
		    then
			exit
		    fi
		done
	    done
	done
    done
done


