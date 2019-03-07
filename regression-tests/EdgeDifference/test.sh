#!/bin/bash
die() { echo "$@" >&2; exit 1
}

echo 'Testing measurement Edge Difference'

TEST_DIR=`pwd`/regression-tests/EdgeDifference
[ -d "$TEST_DIR" ] || die "should be run from top-level directory of the SANA repo"

exitCode=0

for network in 150 WMean_con WMean_ocd; do
    file="$TEST_DIR/$network"

    # Run SANA to align the graph to itself
    echo "Aligning network $network"
    ./sana -t 1 -fg1 "$file.elw" -fg2 "$file.elw" -s3 0 -ed 1 -o "$file" &> "$file.progress"

    # Test if SANA has aligned the graph to itself
    match_nodes=`awk '$1==$2' $file.align | wc -l`
    nodes_count=`awk '{ print $1"\n"$2 }' $file.elw | sort -u | wc -l`
    echo "  EXPECTED matching nodes: $nodes_count GOT: $match_nodes"
    if (( $match_nodes != $nodes_count )); then
        echo "Failed to align $network to itself" 
        exitCode=1
    fi
done

echo 'Done testing measurement Edge Difference.'
exit $exitCode
