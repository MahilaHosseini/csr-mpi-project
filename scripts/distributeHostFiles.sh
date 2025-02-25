#!/bin/bash

# List of VM internal IPs
VMS=("10.198.0.3" "10.198.0.4" "10.198.0.5" "10.198.0.6" "10.164.0.2" "10.164.0.3" "10.128.0.2")

# Path to the compiled binary
BINARY_i="./hostfile"
BINARY_m="./hostfile2"

# Loop through each VM and copy the binary
for vm in "${VMS[@]}"; do
    echo "Copying $BINARY_i $BINARY_m to $vm..."
    scp $BINARY_i $BINARY_m pegah_moradpour01@$vm:~/
done

echo "Distribution complete!"

