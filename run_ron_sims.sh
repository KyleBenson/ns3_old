#!/bin/bash

runs=20
fail_probs='"0.1 0.2 0.3 0.5 0.7 0.9"'
AS_choices='3356 2914 1239' #Level 3, Verio, Sprintlink
#'6461 1755 3967'
disasters[1755]='"Amsterdam,_Netherlands London,_UnitedKingdom Paris,_France"'
disasters[3967]='"Herndon,_VA Irvine,_CA Santa_Clara,_CA"'
disasters[6461]='"San_Jose,_CA Los_Angeles,_CA New_York,_NY"'
disasters[3356]='"Miami,_FL New_York,_NY Los_Angeles,_CA"'
disasters[2914]='"New_York,_NY New_Orleans,_LA Irvine,_CA"'
disasters[1239]='"New_York,_NY Washington,_DC Dallas,_TX"'
heuristics='"0 1 2"' # ideal, random, orthogonal network path

for AS in $AS_choices;
do
    rseed=`date +%s`
    out_dir=ron_output/$pfail/$AS/$disaster/`if [ "$local_overlays" == '0' ]; then echo external; else echo internal; fi;`
    mkdir --parent $out_dir
               	    #echo $out_dir/run$i.out
    command="./waf --run "

    command=$command"'ron "
   # if [ "$1" == "debug" ]
   # then
    #command_temp='gdb --args %s <args>'
    
    #command=$command'--command-template="%s" --verbose=1'
    #(./waf --run <program> --command-template="gdb --args %s <args>" )
   # fi

    command=$command"--fail_prob=$fail_probs "
    command=$command"--file=rocketfuel/maps/$AS.cch "
    command=$command"--disaster=${disasters[$AS]} "
    command=$command"--RngSeed=$rseed --runs=$runs "
    command=$command"--latencies=rocketfuel/weights/all_latencies.intra --contact_attempts=20 --timeout=0.5 --heuristic=$heuristics"
    
    if [ "$1" == "test" ];
    then
	command=$command' --verbose=1'
    fi
    
    command=$command"'"

    if [ "$1" == "test" ];
    then
	echo $command
    fi
    
    exec $command

#quit after first run if argument is 'test'
    if [ "$1" == "test" ];
    then
	exit
    fi
done


