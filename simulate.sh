#!/bin/bash

for i in 1; do
    new_seed=`expr 12345670 + 2 \* $i - 1`

    tracedir='/home/dtn14/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP'
    thisdir='/home/dtn14/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP'
    # tracedir='/home/dtn14/Documents/workspace/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP'
    # thisdir='/home/dtn14/Documents/workspace/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP'
    ./waf --run "dtn_sf_udp --seed=$new_seed --traceFile=$tracedir/newdst.tcl --logFile=$thisdir/ns-2.log --nodeNum=3 --duration=595.0" > dtn.txt
    grep 'final' dtn.txt > temp.txt
    awk '{print $1" "$1" "$" "$" "$}' temp.txt > bundle_delays.tr
    rm temp.txt
    grep -v 'bundle' dtn.txt|grep -v 'antipacket'|grep -v 'ing'|grep -v 'too big' > qtrace.tr
    mv *.tr dtn.txt Run$i
done
