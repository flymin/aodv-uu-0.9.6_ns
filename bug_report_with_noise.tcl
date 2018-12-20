#===============================================================================
# ENSC 835: High-Performance Networks
# Implementation of a Multi-Channel Multi-Interface Ad-Hoc Wireless Networks
#
# Student: Chih-Hao Howard Chang
# 20007-2192
# howardc@sfu.ca
#
# Description: Simulation script for multi-Channel multi-Interface Ad-Hoc
# wireless network in chain topologies
#
# File: wireless_chain_mcmi.tcl
#===============================================================================
#======================================
# MAC Layer Setup
#======================================
Mac/802_11 set dataRate_ 5.4e6 ;# rate for data frames in Mbps
#======================================
# Simulation Parameters Setup
#======================================
set val(chan) Channel/WirelessChannel ;# channel type
set val(prop) Propagation/TwoRayGround ;# radio-propagation model
set val(netif) Phy/WirelessPhy ;# network interface type
set val(mac) Mac/802_11 ;# MAC type
set val(ifq) Queue/DropTail/PriQueue ;# interface queue type
set val(ll) LL ;# link layer type
set val(ant) Antenna/OmniAntenna ;# antenna model
set val(ifqlen) 50 ;# max packet in ifq
set val(rp) AODVUU ;# routing protocol
set val(x) 1000 ;# X dimension of topography
set val(y) 1000 ;# Y dimension of topography
set val(stop) 70 ;# nam stop time
set val(nn) 6 ;# number of mobilenodes
set val(nc) 3 ;# number of channels
set val(ni) 3 ;# number of interfaces, <= number of channels
set val(pu) 2 ;# number of PUs
set pktsize 1000 ;# packet size in bytes
set pktrate 0.0014 ;# packet rate in seconds
set filename chain_replace
puts "Ad-Hoc Wireless Network in Chain Topologies - $val(nn) Nodes, $val(nc) Channels, $val(ni) Interfaces"
#======================================
# Initialization
#======================================
# Create a ns simulator
set ns_ [new Simulator]
# Setup topography object
set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)
set god_ [create-god [expr $val(nn)*$val(nc)]]
# Open the NS trace file
set tracefd [open $filename.tr w]
$ns_ trace-all $tracefd
$ns_ use-newtrace
# Open the NAM trace file
set namfile [open $filename.nam w]
$ns_ namtrace-all $namfile
$ns_ namtrace-all-wireless $namfile $val(x) $val(y)
# Create wireless channels
for {set i 0} {$i < $val(nc)} {incr i} {
    set chan($i) [new $val(chan)]
}
#======================================
# Mobile Node Parameter Setup
#======================================
$ns_ node-config -adhocRouting $val(rp) \
                -llType $val(ll) \
                -macType $val(mac) \
                -ifqType $val(ifq) \
                -ifqLen $val(ifqlen) \
                -antType $val(ant) \
                -propType $val(prop) \
                -phyType $val(netif) \
                -channel $chan(0) \
                -topoInstance $topo \
                -agentTrace ON \
                -routerTrace ON \
                -macTrace OFF \
                -movementTrace OFF \
                -ifNum $val(ni) \
                -workMode 0 \
                -noiseChannel 0
#======================================
# Nodes Definition
#======================================
$ns_ change-numifs $val(nc)
for {set i 0} {$i < $val(nc)} {incr i} {
    $ns_ add-channel $i $chan($i)
}
# Create nodes
for {set i 0} {$i < $val(nn)} {incr i} {
    set n($i) [$ns_ node]
    $god_ new_node $n($i)
}

# Set node positions in horizontal chain topology

set nodedist 250

#for {set i 0} {$i < $val(nn)} {incr i} {
#    $n($i) set X_ [expr $i * $nodedist + 20]
#    $n($i) set Y_ 50
#    $n($i) set Z_ 0.0
#    $ns_ initial_node_pos $n($i) 40
#    $n($i) random-motion 0
#}

#modified for chain_replace procedure++++++++++++++++++++++++++++++++++++++++++

$n(0) set X_ 0
$n(0) set Y_ 300
$n(0) set Z_ 0.0
$ns_ initial_node_pos $n(0) 10
$n(0) random-motion 0

$n(1) set X_ 100
$n(1) set Y_ 300
$n(1) set Z_ 0.0
$ns_ initial_node_pos $n(1) 10
$n(1) random-motion 0

$n(2) set X_ 300
$n(2) set Y_ 300
$n(2) set Z_ 0.0
$ns_ initial_node_pos $n(2) 10
$n(2) random-motion 0

$n(3) set X_ 300
$n(3) set Y_ 0
$n(3) set Z_ 0.0
$ns_ initial_node_pos $n(3) 10
$n(3) random-motion 0

$n(4) set X_ 500
$n(4) set Y_ 300
$n(4) set Z_ 0.0
$ns_ initial_node_pos $n(4) 10
$n(4) random-motion 0

$n(5) set X_ 600
$n(5) set Y_ 300
$n(5) set Z_ 0.0
$ns_ initial_node_pos $n(5) 10
$n(5) random-motion 0

#motion part added here
$ns_ at 1.0 "$n(2) setdest 300.0 600.0 100.0"
$ns_ at 1.3 "$n(3) setdest 300.0 300.0 100.0"
$ns_ at 12.0 "$n(2) setdest 300.0 300.0 100.0"
$ns_ at 10.0 "$n(3) setdest 300.0 100.0 100.0"
$ns_ at 21.0 "$n(2) setdest 300.0 600.0 100.0"
$ns_ at 24.3 "$n(3) setdest 300.0 300.0 100.0"
#motion modified ends here

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#======================================
# PU Node Parameter Setup
#======================================
$ns_ node-config -workMode -1 \
                 -noiseChannel 0

set pu_node(0) [$ns_ node]
$god_ new_node $pu_node(0)

$pu_node(0) set X_ 500
$pu_node(0) set Y_ 200
$pu_node(0) set Z_ 0.0

$ns_ initial_node_pos $pu_node(0) 40
$pu_node(0) random-motion 0

$ns_ node-config -workMode -1 \
                -noiseChannel 1

set pu_node(1) [$ns_ node]
$god_ new_node $pu_node(1)

$pu_node(1) set X_ 500
$pu_node(1) set Y_ 400
$pu_node(1) set Z_ 0.0
$ns_ initial_node_pos $pu_node(1) 40
$pu_node(1) random-motion 0

#======================================
# Agents Definition
#======================================
set udp0 [new Agent/UDP]
$ns_ attach-agent $n(0) $udp0
set sink0 [new Agent/Null]
set last_node_id [expr $val(nn)-1]
$ns_ attach-agent $n(5) $sink0
$ns_ connect $udp0 $sink0
#======================================
# Applications Definition
#======================================
# Setup a CBR Application over UDP connection
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
$cbr0 set packetSize_ $pktsize
$cbr0 set interval_ $pktrate
$ns_ at 1.0 "$cbr0 start"
$ns_ at 61.0 "$cbr0 stop"


#======================================
# Simulation Termination
#======================================
# Define a finish procedure
proc finish {} {
global ns_ tracefd filename pktsize last_node_id
    global namfile
    $ns_ flush-trace
    close $tracefd
    close $namfile
    # exec nam $filename.nam &
    # Call throughput analyzer (AWK scripts written by Marco Fiore, marco.fiore@polito.it)
    # exec awk -f avgStats.awk src=0 dst=$last_node_id flow=0 pkt=$pktsize $filename.tr &
    exit 0
}
for {set i 0} {$i < $val(nn) } { incr i } {
    $ns_ at $val(stop) "\$n($i) reset"
} 
$ns_ at $val(stop) "$ns_ nam-end-wireless $val(stop)"
$ns_ at $val(stop) "finish"
$ns_ at $val(stop) "puts \"done\" ; $ns_ halt"
$ns_ run
