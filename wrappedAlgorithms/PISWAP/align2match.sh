#!/bin/sh
echo "M0 = {"
cat "$@" | awk '
    NR==1{printf "\"%s\":\"%s\"",$1,$2}
    NR>1{printf ",\n\"%s\":\"%s\"",$1,$2}'
echo "}"
