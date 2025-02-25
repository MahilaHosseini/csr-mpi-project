#!/bin/bash

# List of VM internal IPs
VMS=("10.198.0.3" "10.198.0.4" "10.198.0.5" "10.198.0.6" "10.164.0.2" "10.164.0.3" "10.128.0.2")

# Loop through each VM and run the commands
for vm in "${VMS[@]}"; do
    echo "Updating and installing OpenMPI on $vm..."
    ssh pegah_moradpour01@$vm << EOF
        sudo apt update
        sudo apt install -y openmpi-bin openmpi-common libopenmpi-dev
        mpirun --version
        sudo apt install -y build-essential

EOF
done

echo "Installation and version check complete on all VMs!"

