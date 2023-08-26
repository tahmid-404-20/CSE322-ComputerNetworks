#!/bin/bash

file="1905002.cc"

data="scratch/offline3/data"
plot="scratch/offline3/plot"

rm -rf "$data"
rm -rf "$plot"
mkdir "$data"
mkdir "$plot"

output_str=""

adaptiveRenoSpeed="$data/adaptiveRenoSpeed.dat"
adaptiveRenoNewRenoSpeed="$data/adaptiveRenoNewRenoSpeed.dat"
adaptiveRenoExp="$data/adaptiveRenoExp.dat"
adaptiveRenoNewRenoExp="$data/adaptiveRenoNewRenoExp.dat"

touch $adaptiveRenoSpeed
touch $adaptiveRenoNewRenoSpeed
touch $adaptiveRenoExp
touch $adaptiveRenoNewRenoExp

# do same for westwoodPlus
westwoodPlusSpeed="$data/westwoodPlusSpeed.dat"
westwoodPlusNewRenoSpeed="$data/westwoodPlusNewRenoSpeed.dat"
westwoodPlusExp="$data/westwoodPlusExp.dat"
westwoodPlusNewRenoExp="$data/westwoodPlusNewRenoExp.dat"

touch $westwoodPlusSpeed
touch $westwoodPlusNewRenoSpeed
touch $westwoodPlusExp
touch $westwoodPlusNewRenoExp

# do same for highspeed
highspeedSpeed="$data/highspeedSpeed.dat"
highspeedNewRenoSpeed="$data/highspeedNewRenoSpeed.dat"
highspeedExp="$data/highspeedExp.dat"
highspeedNewRenoExp="$data/highspeedNewRenoExp.dat"

touch $highspeedSpeed
touch $highspeedNewRenoSpeed
touch $highspeedExp
touch $highspeedNewRenoExp


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

generate_gnuplot_script_multiple_file() {
    local data_file1=$1
    local data_file2=$2
    local output_png=$3
    local algo1=$4
    local algo2=$5

    touch "$output_png"

    cat > gnuplot_script.gp <<EOF
set terminal png size 640,480
set output "${output_png}"
plot "${data_file1}" using 1:2 title '${algo1}' with linespoints, "${data_file2}" using 1:2 title '${algo2}' with linespoints
EOF

    gnuplot gnuplot_script.gp
    rm gnuplot_script.gp
}


# --bRate:     BottleNeckDataRate [50]
#     --errorExp:  10^[-errorExp] [6]
#     --congAlg:   Congestion Control Algorithm [ns3::TcpAdaptiveReno]

#  congestion control algorithm as argument 1
generate_data_varying_parameters()
{
    local congestion_control=$1  
    local cong_speed=$2
    local cong_speed_newReno=$3
    local cong_exp=$4
    local cong_exp_newReno=$5

    # varying speed as 1,25,50, 75, ... ,300 Mbps and write to file cong_speed and cong_newReno_speed
    echo "Congestion control algorithm: $congestion_control"
    echo ""
    echo "Varying bottleneck capacity"
    speed=1
    for i in {0..12}
    do
        if [ $i -eq 0 ]
        then
            speed=1
        else
            speed=$((i*25))
        fi

        echo "Running $file with bottleneck capacity $speed"
        switch="--bRate=$speed --congAlg=$congestion_control"
        output_str="$(./ns3 run "$file $switch")"

        IFS=',' read -ra ADDR <<< "$output_str"
        cong_throughput="${ADDR[0]}"
        newReno_througput="${ADDR[1]}"

        echo "$speed $cong_throughput" >> $cong_speed
        echo "$speed $newReno_througput" >> $cong_speed_newReno
    done

    # varying exp as 1,2,3,4,5,6 and write to file cong_exp and cong_exp_newReno
    echo ""
    echo "Varying error exponent"
    for i in {1..6}
    do        
        exp=$((i))
        echo "Running $file with error exponent $exp"
        switch="--errorExp=$exp --congAlg=$congestion_control"
        output_str="$(./ns3 run "$file $switch")"

        IFS=',' read -ra ADDR <<< "$output_str"
        throughput="${ADDR[0]}"
        ratio="${ADDR[1]}"

        echo "$exp $throughput" >> $cong_exp
        echo "$exp $ratio" >> $cong_exp_newReno
    done

    echo ""
    echo ""

}

generate_congestion_window_data_and_plot() {
    local congestion_control=$1
    local output_png=$2
    local flow0_file="scratch/offline3/cwnd0.dat"
    local flow1_file="scratch/offline3/cwnd1.dat"

    echo "Running $file with congestion control $congestion_control"
    switch="--congAlg=$congestion_control"
    ./ns3 run "$file $switch"

    generate_gnuplot_script_multiple_file $flow0_file $flow1_file "$output_png" "$congestion_control" "ns3::TcpNewReno"   
}

# generate data for adaptiveReno
generate_data_varying_parameters "ns3::TcpAdaptiveReno" $adaptiveRenoSpeed $adaptiveRenoNewRenoSpeed $adaptiveRenoExp $adaptiveRenoNewRenoExp

# generate data for westwoodPlus
generate_data_varying_parameters "ns3::TcpWestwoodPlus" $westwoodPlusSpeed $westwoodPlusNewRenoSpeed $westwoodPlusExp $westwoodPlusNewRenoExp

# generate data for highspeed
generate_data_varying_parameters "ns3::TcpHighSpeed" $highspeedSpeed $highspeedNewRenoSpeed $highspeedExp $highspeedNewRenoExp


echo "Generating plots for calculated data"
# generate plots for varying speed
generate_gnuplot_script_multiple_file $adaptiveRenoSpeed $adaptiveRenoNewRenoSpeed "$plot/adaptiveRenoCapacity.png" "Adaptive Reno" "New Reno"
generate_gnuplot_script_multiple_file $westwoodPlusSpeed $westwoodPlusNewRenoSpeed "$plot/westwoodPlusCapacity.png" "Westwood Plus" "New Reno"
generate_gnuplot_script_multiple_file $highspeedSpeed $highspeedNewRenoSpeed "$plot/highspeedCapacity.png" "Highspeed" "New Reno"

# generate plots for varying exp
generate_gnuplot_script_multiple_file $adaptiveRenoExp $adaptiveRenoNewRenoExp "$plot/adaptiveRenoExp.png" "Adaptive Reno" "New Reno"
generate_gnuplot_script_multiple_file $westwoodPlusExp $westwoodPlusNewRenoExp "$plot/westwoodPlusExp.png" "Westwood Plus" "New Reno"
generate_gnuplot_script_multiple_file $highspeedExp $highspeedNewRenoExp "$plot/highspeedExp.png" "Highspeed" "New Reno"

echo ""
echo "Collecting data and generating plot for congestion window using default parameters"
generate_congestion_window_data_and_plot "ns3::TcpAdaptiveReno" "$plot/adaptiveRenoCwnd.png"
generate_congestion_window_data_and_plot "ns3::TcpWestwoodPlus" "$plot/westwoodPlusCwnd.png"
generate_congestion_window_data_and_plot "ns3::TcpHighSpeed" "$plot/highspeedCwnd.png"
