#!/bin/bash
count=100000;
for (( i = 0; i < count; i++ )); do
	curl localhost:3000 &
done
