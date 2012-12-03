#!/bin/bash

verbosity_level=1

# check args

if [ "$1" == "test" -o "$1" == "debug" ]
then
    verbose=1
    testing=1
fi

if [ "$1" == "debug" ]
then
    debug=1
fi

if [ "$1" == "visualize" -o "$1" == "vis" ]
then
    visualize=1
fi

runs=10
fail_probs='"0.5"' #'"0.1 0.2 0.3 0.5 0.7 0.9"'
#AS_choices='3356 2914 1239' #Level 3, Verio, Sprintlink
AS_choices='6461 1755 3967' #smaller ones
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
    #out_dir=ron_output/$pfail/$AS/$disaster/`if [ "$local_overlays" == '0' ]; then echo external; else echo internal; fi;`
    #mkdir --parent $out_dir
               	    #echo $out_dir/run$i.out
    #command="./waf --run "
    command="./waf --run ron --command-template='"

    #command="$command'ron "

    if [ $debug ]
    then
	command="$command""gdb --args "
    fi

    command="$command%s "
    command="$command--fail_prob=$fail_probs "
    command="$command--file=rocketfuel/maps/$AS.cch "
    command="$command--disaster=${disasters[$AS]} "
    command="$command--RngSeed=$rseed --runs=$runs "
    command="$command--latencies=rocketfuel/weights/all_latencies.intra "
    command="$command--locations=rocketfuel/city_locations.txt "
    command="$command--contact_attempts=20 --timeout=0.5 --heuristic=$heuristics"

    ##### $$$$ NO LONGER ASSUME SPACES " " AFTER COMMANDS
    
    if [ $verbose ];
    then
	command="$command --verbose=$verbosity_level"
    fi
    
    command="$command'"  ##### terminate ron program's args

    if [ $visualize ];
    then
	command="$command --visualize"
    fi

    if [ $verbose ];
    then
	echo $command
    fi
    
    # MUST use eval, or waf parses things strangely and tries to use the ron program's args...
    eval $command

#quit after first run if argument is 'test'
    if [ $testing ];
    then
	exit
    fi
done


