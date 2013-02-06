#!/bin/sh

LC_ALL=C
IFS=' '

ipcs -m | sort | sed -e '1,4d' | awk '{ print $2 }' > tmp

i=0
while read id; do
  ipcrm -m "${id}"
  i="$((${i}+1))"
done < tmp

printf "shm segments: %d\n" "${i}"

ipcs -q | sort | sed -e '1,4d' | awk '{ print $2 }' > tmp

i=0
while read id; do
  ipcrm -q "${id}"
  i="$((${i}+1))"
done < tmp

printf "message queues: %d\n" "${i}"

ipcs -s | sort | sed -e '1,4d' | awk '{ print $2 }' > tmp

i=0
while read id; do
  ipcrm -s "${id}"
  i="$((${i}+1))"
done < tmp

printf "semaphore arrays: %d\n" "${i}"

rm -f tmp
