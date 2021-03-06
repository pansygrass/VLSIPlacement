#!/bin/bash

export BENCHMARK_ROOT=$HOME/results
export SCRIPT_ROOT=$HOME/thesis/VLSIPlacement/scripts
export UTILS_ROOT=$HOME/Downloads/Benchmarks
export EXEC_FILE=$HOME/thesis/VLSIPlacement/code/the_exec
export OUTPUT_PATH=$(pwd)
export DESIGNS="cordic_cpu12 cordic_cpu8"
export SYNTHESIS_PATH=/opt/CAD/Synopsys/Current/syn/bin
export ICC_PATH=/opt/CAD/Synopsys/Current/icc/bin
export PT_PATH=/opt/CAD/Synopsys/Current/pts/bin
export INPUT_TAB="$1"
export EXEC_TYPE="$2"

echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
echo "Beginning analysis for inputs $INPUT_TAB"
echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
$SCRIPT_ROOT/topAnalyze.pl $INPUT_TAB
