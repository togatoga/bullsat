#!/bin/bash
set -u

SOLVER=$1
TIMELIMIT=$2

sat=0
unsat=0
unknown=0
total_ms=0
echo "Time Limit... ${TIMELIMIT} s"
for file in `find cnf/benchmark/*/*.cnf -type f`; do

    cnf=`basename $file`
    result="benchmark/"${cnf}_result.txt
    echo "UNKNOWN" > $result
    
    echo "Solving.... ${file}"
    start_ms=`date +%s%3N`
    timeout ${TIMELIMIT}s ${SOLVER} $file $result
    end_ms=`date +%s%3N`
    status=`head -n 1 $result`
    if [ ${status} = "SAT" ]; then
        sat=`expr $sat + 1`
    elif [ ${status} = "UNSAT" ]; then
        unsat=`expr $unsat + 1`
    else
        unknown=`expr $unsat + 1`
    fi
    elasped_ms=`expr $end_ms - $start_ms`
    total_ms=`expr $total_ms + $elasped_ms`
    echo "Status: ${status} Time: ${elasped_ms} ms"
done

echo "SAT: ${sat} UNSAT: ${unsat} UNKNOWN: ${unknown}"
echo "Total Time: ${total_ms} ms"
