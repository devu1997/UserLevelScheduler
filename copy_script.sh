#!/bin/bash

# Loop to copy log.txt to log0.txt, log1.txt, ..., log31.txt
for i in {1..516}; do
    cp logs/log0.txt "logs/log$i.txt"
done
