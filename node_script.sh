#!/bin/bash

# Define the command to run (replace with your command)
command_to_run="node node_scheduler.js"

# Number of times to run the command
runs=1

# Array to store process IDs
pids=()

# Function to run the command in the background
run_command() {
    for ((i=1; i<=$runs; i++)); do
        $command_to_run
    done
}

# Record start time
start=$(date +%s.%N)

# Run the command in parallel
for ((i=1; i<=runs; i++)); do
    run_command &
    pids+=($!)
done

# Wait for all background processes to finish
for pid in ${pids[@]}; do
    wait $pid
done

# Record end time
end=$(date +%s.%N)

# Calculate total time
total_time=$(echo "$end - $start" | bc)

# Print total time
echo "Total time taken: $total_time seconds"
