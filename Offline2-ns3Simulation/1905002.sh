#!/bin/bash

file1="1905002_1.cc"
file2="1905002_2.cc"

rm -rf "scratch/static"
rm -rf "scratch/mobile"
mkdir "scratch/static"
mkdir "scratch/mobile"

output_str=""

static="scratch/static"
mobile="scratch/mobile"

staticNodeThroughput="$static/nodeThroughput.dat"
staticNodeRatio="$static/nodeRatio.dat"

staticFlowThroughput="$static/flowThroughput.dat"
staticFlowRatio="$static/flowRatio.dat"

staticPacketThroughput="$static/packetThroughput.dat"
staticPacketRatio="$static/packetRatio.dat"

staticAreaThroughput="$static/areaThroughput.dat"
staticAreaRatio="$static/areaRatio.dat"

touch $staticNodeThroughput
touch $staticNodeRatio
touch $staticFlowThroughput
touch $staticFlowRatio
touch $staticPacketThroughput
touch $staticPacketRatio
touch $staticAreaThroughput
touch $staticAreaRatio


# do the same for mobile
mobileNodeThroughput="$mobile/nodeThroughput.dat"
mobileNodeRatio="$mobile/nodeRatio.dat"

mobileFlowThroughput="$mobile/flowThroughput.dat"
mobileFlowRatio="$mobile/flowRatio.dat"

mobilePacketThroughput="$mobile/packetThroughput.dat"
mobilePacketRatio="$mobile/packetRatio.dat"

mobileSpeedThroughput="$mobile/speedThroughput.dat"
mobileSpeedRatio="$mobile/speedRatio.dat"


touch $mobileNodeThroughput
touch $mobileNodeRatio
touch $mobileFlowThroughput
touch $mobileFlowRatio
touch $mobilePacketThroughput
touch $mobilePacketRatio
touch $mobileSpeedThroughput
touch $mobileSpeedRatio


#!/bin/bash

# Function to generate the Gnuplot script and PNG file for a given data file
generate_gnuplot_script() {
    local data_file=$1
    local output_png=$2
    local param=$3
    local y_label=$4

    touch "$output_png"

    cat > gnuplot_script.gp <<EOF
set terminal png size 640,480
set output "${output_png}"
plot "${data_file}" using 1:2 title '${param} VS ${y_label}' with linespoints
EOF

    gnuplot gnuplot_script.gp
    rm gnuplot_script.gp
}


for i in {1..5}
do
    # execute file1 and redirect output to output_str
    nNodes=$((i*20))
    nFlows=$((i*10))
    nPacketsPerSecond=$((i*100))
    speed=$((i*5))
    coverageMutliplier=$i

    # varying nodes
    echo "Running $file1 with $nNodes nodes"
    switch="--nNodes=$nNodes --nFlows=$nFlows"
    output_str="$(./ns3 run "$file1 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$nNodes $throughput" >> $staticNodeThroughput
    echo "$nNodes $ratio" >> $staticNodeRatio

    # varying flows
    echo "Running $file1 with $nFlows flows"
    switch="--nFlows=$nFlows"
    output_str="$(./ns3 run "$file1 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$nFlows $throughput" >> $staticFlowThroughput
    echo "$nFlows $ratio" >> $staticFlowRatio

    # varying packets
    echo "Running $file1 with $nPacketsPerSecond packets per second"
    switch="--nPacketsPerSecond=$nPacketsPerSecond"
    output_str="$(./ns3 run "$file1 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$nPacketsPerSecond $throughput" >> $staticPacketThroughput
    echo "$nPacketsPerSecond $ratio" >> $staticPacketRatio

    # varying area
    echo "Running $file1 with $coverageMutliplier coverage multiplier"
    switch="--coverageMultiplier=$coverageMutliplier"
    output_str="$(./ns3 run "$file1 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$coverageMutliplier $throughput" >> $staticAreaThroughput
    echo "$coverageMutliplier $ratio" >> $staticAreaRatio


    # for mobile topology
    # varying nodes
    echo "Running $file2 with $nNodes nodes"
    switch="--nNodes=$nNodes --nFlows=$nFlows"
    output_str="$(./ns3 run "$file2 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$nNodes $throughput" >> $mobileNodeThroughput
    echo "$nNodes $ratio" >> $mobileNodeRatio

    # varying flows
    echo "Running $file2 with $nFlows flows"
    switch="--nFlows=$nFlows"
    output_str="$(./ns3 run "$file2 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$nFlows $throughput" >> $mobileFlowThroughput
    echo "$nFlows $ratio" >> $mobileFlowRatio

    # varying packets
    echo "Running $file2 with $nPacketsPerSecond packets per second"
    switch="--nPacketsPerSecond=$nPacketsPerSecond"
    output_str="$(./ns3 run "$file2 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$nPacketsPerSecond $throughput" >> $mobilePacketThroughput
    echo "$nPacketsPerSecond $ratio" >> $mobilePacketRatio

    # varying speed
    echo "Running $file2 with $speed speed"

    switch="--speed=$speed"
    output_str="$(./ns3 run "$file2 $switch")"

    IFS=',' read -ra ADDR <<< "$output_str"
    throughput="${ADDR[0]}"
    ratio="${ADDR[1]}"

    echo "$speed $throughput" >> $mobileSpeedThroughput
    echo "$speed $ratio" >> $mobileSpeedRatio

done


# plot for static
echo "Generating plots for static topology"
generate_gnuplot_script $staticNodeThroughput "scratch/static/nodeThroughput.png" "Nodes" "Throughput"
generate_gnuplot_script $staticNodeRatio "scratch/static/nodeRatio.png" "Nodes" "Ratio"

generate_gnuplot_script $staticFlowThroughput "scratch/static/flowThroughput.png" "Flow" "Throughput"
generate_gnuplot_script $staticFlowRatio "scratch/static/flowRatio.png" "Flow" "Ratio"

generate_gnuplot_script $staticPacketThroughput "scratch/static/packetThroughput.png" "Pkt/sec" "Throughput"
generate_gnuplot_script $staticPacketRatio "scratch/static/packetRatio.png" "Pkt/sec" "Ratio"

generate_gnuplot_script $staticAreaThroughput "scratch/static/areaThroughput.png" "Area" "Throughput"
generate_gnuplot_script $staticAreaRatio "scratch/static/areaRatio.png" "Area" "Ratio"



# plot for mobile
echo "Generating plots for mobile topology"
generate_gnuplot_script $mobileNodeThroughput "scratch/mobile/nodeThroughput.png" "Nodes" "Throughput"
generate_gnuplot_script $mobileNodeRatio "scratch/mobile/nodeRatio.png" "Nodes" "Ratio"

generate_gnuplot_script $mobileFlowThroughput "scratch/mobile/flowThroughput.png" "Flow" "Throughput"
generate_gnuplot_script $mobileFlowRatio "scratch/mobile/flowRatio.png" "Flow" "Ratio"

generate_gnuplot_script $mobilePacketThroughput "scratch/mobile/packetThroughput.png" "Pkt/sec" "Throughput"
generate_gnuplot_script $mobilePacketRatio "scratch/mobile/packetRatio.png" "Pkt/sec" "Ratio"

generate_gnuplot_script $mobileSpeedThroughput "scratch/mobile/speedThroughput.png" "Speed" "Throughput"
generate_gnuplot_script $mobileSpeedRatio "scratch/mobile/speedRatio.png" "Speed" "Ratio"
