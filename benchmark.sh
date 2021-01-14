#!/bin/bash
set -u

SOLVER=$1
TIMELIMIT=$2

total_ms=0
echo "Tim Limit... ${TIMELIMIT} s"
for file in `find cnf/benchmark/*/*.cnf -type f`; do

    cnf=`basename $file`
    result="benchmark/"${cnf}_result.txt
    echo "UNKNOWN" > $result
    
    echo "Solving.... ${file}"
    start_ms=`date +%s%3N`
    timeout ${TIMELIMIT}s ${SOLVER} $file $result
    end_ms=`date +%s%3N`
    status=`head -n 1 $result`
    elasped_ms=`expr $end_ms - $start_ms`
    total_ms=`expr $total_ms + $elasped_ms`
    echo "Status: ${status} Time: ${elasped_ms} ms"
done

echo "Total Time: ${total_ms} ms"
