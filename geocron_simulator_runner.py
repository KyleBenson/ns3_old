#! /usr/bin/python
from __future__ import print_function
GEOCRON_SIMULATOR_RUNNER_DESCRIPTION = '''Handles building command line arguments to be passed to the Geocron Simulator.'''

# @author: Kyle Benson
# (c) Kyle Benson 2012

import argparse
from os import system
#from os.path import isdir
#from os import listdir

default_runs=100
default_start=0 # num to start run IDs on
default_nprocs=8
default_as_choices=['3356',
                    '1239'
                    ]
default_heuristics=['1',
                    '2'
                    ]
default_fprobs=["0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6"
                ]
disasters = {}
disasters['1755']='"Amsterdam,_Netherlands-London,_UnitedKingdom-Paris,_France"'
disasters['3967']='"Herndon,_VA-Irvine,_CA-Santa_Clara,_CA"'
disasters['6461']='"San_Jose,_CA-Los_Angeles,_CA-New_York,_NY"'
disasters['3356']='"New_York,_NY-Los_Angeles,_CA"' #Miami,_FL-
disasters['2914']='"New_York,_NY-New_Orleans,_LA-Irvine,_CA"'
disasters['1239']='"New_York,_NY-Dallas,_TX"' #Washington,_DC
default_verbosity_level=1
def parse_args(args):
## DEFAULTS
##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
# action is one of: store[_const,_true,_false], append[_const], count
# nargs is one of: N, ?(defaults to const when no args), *, +, argparse.REMAINDER
# help supports %(var)s: help='default value is %(default)s'
##################################################################################

    parser = argparse.ArgumentParser(description=GEOCRON_SIMULATOR_RUNNER_DESCRIPTION,
                                     add_help=True,
                                     #formatter_class=argparse.RawTextHelpFormatter,
                                     #epilog='Text to display at the end of the help print',
                                     )

    # Simulation parameters
    parser.add_argument('--as', nargs='+', default=default_as_choices,
                        help='''choose the AS topologies for the simulations''',dest='topologies')
    parser.add_argument('--disasters', type=str, nargs='*',
                        help='''disaster locations to apply to ALL AS choices (cities currently) (default depends on AS)''')
    parser.add_argument('--fprobs', '-f', type=str, nargs='*',
                        default=default_fprobs,
                        help='''failure probabilities to use (default=%(default)s)''')
    parser.add_argument('--heuristics', nargs='*', default=default_heuristics,
                        help='''which heuristics to run (1,2,...MAX) (default=%(default)s)''')

    # Control simulation repetition
    parser.add_argument('--runs', '-r', nargs='?',default=default_runs, type=int,
                        help='''number of times to run simulation for each set of parameters (default=%(default)s)''')
    parser.add_argument('--nprocs', '-n', nargs='?', action='store', type=int,
                        const=default_nprocs, default=1,
                        help='''number of parallel ns-3 instances to run  (default=%(default)s if flag unspecified, %(const)s if flag specified)''')
    parser.add_argument('--start', '-s', type=int, action='store',
                        default=default_start,
                        help='''unique ID number to start runs on  (default=%(default)s). Useful for running parallel instances.''')

    # Control UI
    parser.add_argument('--debug', '-d', action="store_true",
                        help='''run simulator through GDB''')
    parser.add_argument('--visualize', action="store_true",
                        help='''run with PyViz visualizer''')
    parser.add_argument('--verbose', '-v', nargs='?',
                        default=default_verbosity_level, type=int,
                        help='''run with verbose printing (default=%(default)s when no arg given)''')
    parser.add_argument('--show-cmd', '-c', action="store_true", dest="show_cmd", default=False,
                        help='''print the system command to be executed then exit''')
    parser.add_argument('--test', '-t', action="store_true",
                        help='''simulator will only run once in a single process, then exit''')

    args = parser.parse_args(args)
    return args

def makecmds(args):
    if args.test:
        args.runs = 1
        args.nprocs = 1
        args.heuristics = args.heuristics[0]
        args.topologies = args.topologies[0]
        args.fprobs = args.fprobs[0]

    # convert commands to pass to geocron-simulator
    fprobs = '"%s"' % '-'.join(args.fprobs)
    heuristics = '"%s"' % '-'.join(args.heuristics)

    # determine # procs for each topology
    procs_per_topology = 1
    remainder_procs_per_topology = 0
    if args.nprocs < len(args.topologies):
        print("WARNING: more topology files specified than number of processes to be run. "
              "Exit and re-run with correct parameters to avoid creating too many processes")
    else:
        procs_per_topology = args.nprocs/len(args.topologies)
        remainder_procs_per_topology = args.nprocs % len(args.topologies)

    # build a command for each proc, or each topology if its more
    for topology in args.topologies:
        startnum = args.start # reset for each topology

        # determine how many procs for this topology
        numprocs = procs_per_topology
        if remainder_procs_per_topology:
            numprocs += 1
            remainder_procs_per_topology -= 1

        # set nruns for each proc in this topology
        runs_per_proc = args.runs/numprocs
        remainder_runs_per_proc = args.runs % numprocs

        for i in range(numprocs):
            nruns = runs_per_proc
            if remainder_runs_per_proc:
                nruns += 1
                remainder_runs_per_proc -= 1

            cmd = "./waf --run ron --command-template='"
            if args.debug:
                cmd += "gdb --args "
            cmd += r'%s '

            cmd += '--fail_prob=%s ' % fprobs
            cmd += '--file=rocketfuel/maps/%s.cch ' % topology
            cmd += '--disaster=%s ' % (args.disasters if args.disasters else disasters[topology])
            cmd += '--runs=%i ' % nruns
            cmd += '--start_run=%i ' % startnum
            cmd += '--heuristic=%s ' % heuristics

            # static args
            cmd += "--latencies=rocketfuel/weights/all_latencies.intra "
            cmd += "--locations=rocketfuel/city_locations.txt "
            cmd += "--contact_attempts=20 --timeout=0.5"

            ## $$$$ NO LONGER ASSUME SPACES " " AFTER COMMANDS!

            if args.verbose:
                cmd += ' --verbose=%i' % args.verbose
            cmd += "'" #terminate command template (non-waf args)
            if args.visualize:
                cmd += ' --visualize'

            startnum += nruns

            yield cmd

# Main
if __name__ == "__main__":
    
    import sys, subprocess

    args = parse_args(sys.argv[1:])

    children = []
    for cmd in makecmds(args):
    
        if args.show_cmd or args.verbose or args.test:
            print(cmd)
        if args.show_cmd:
            exit(0)

        children.append(subprocess.Popen(cmd))
            
    # wait for children
    for c in children:
        c.wait()

    #TODO: trap ctrl-c
    #TODO: email when finished
