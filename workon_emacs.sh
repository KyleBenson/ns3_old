#!/bin/bash
if [ "$1" == '-nw' ];
then em geocron_simulator_runner.py scratch/ron/*.cc scratch/ron/*.h
else emacs geocron_simulator_runner.py scratch/ron/*.cc scratch/ron/*.h &
fi
