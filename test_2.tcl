set val(chan)           Channel/WirelessChannel    ;#Channel Type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation model
set val(netif)          Phy/WirelessPhy            ;# network interface type
set val(mac)            Mac/802_11                 ;# MAC type
set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(rp)             AODVUU                     ;# routing protocol
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                         ;# max packet in ifq
set val(nn)             4                          ;# number of mobilenodes
set val(ni)		3
set val(x)		1000
set val(y)		1000

# Initialize Global Variables
set ns_		[new Simulator]

set tracefd     [open wireless_2.tr w]
$ns_ trace-all $tracefd

set namtrace [open wireless_2.nam w]
$ns_ namtrace-all-wireless $namtrace $val(x) $val(y)

# set up topography object
set topo 	[new Topography]
$topo load_flatgrid $val(x) $val(y)

# Create God
create-god [expr $val(nn)*$val(ni)]

$ns_ use-newtrace

# Create channel 
for {set i 0} {$i < $val(ni)} {incr i} {
	set chan($i) [new $val(chan)]
}

# configure node, please note the change below.
$ns_ node-config -adhocRouting $val(rp) \
		-llType $val(ll) \
		-macType $val(mac) \
		-ifqType $val(ifq) \
		-ifqLen $val(ifqlen) \
		-antType $val(ant) \
		-propType $val(prop) \
		-phyType $val(netif) \
		-topoInstance $topo \
		-agentTrace ON \
		-routerTrace ON \
		-macTrace ON \
		-movementTrace ON \
		-channel $chan(0) \
		-ifNum $val(ni)

$ns_ change-numifs $val(ni)
for {set i 0} {$i < $val(ni)} {incr i} {
	$ns_ add-channel $i $chan($i)
}

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	#$ns_ initial_node_pos $node_($i) 20
	$node_($i) random-motion 0
}

#
# Provide initial (X,Y, for now Z=0) co-ordinates for mobilenodes
#
$node_(0) set X_ 5.0
$node_(0) set Y_ 20.0
$node_(0) set Z_ 0.0
$ns_ initial_node_pos $node_(0) 10

$node_(1) set X_ 55.0
$node_(1) set Y_ 20.0
$node_(1) set Z_ 0.0
$ns_ initial_node_pos $node_(1) 10

$node_(2) set X_ 105.0
$node_(2) set Y_ 20.0
$node_(2) set Z_ 0.0
$ns_ initial_node_pos $node_(2) 10

$node_(3) set X_ 155.0
$node_(3) set Y_ 20.0
$node_(3) set Z_ 0.0
$ns_ initial_node_pos $node_(3) 10
# set node movement
set god_ [God instance]
$ns_ at 5.0 "$node_(1) setdest 55.0 400.0 60.0"
$ns_ at 5.0 "$node_(3) setdest 400.0 20.0 60.0"
$ns_ at 15.0 "$node_(0) setdest 5.0 900.0 60.0"
$ns_ at 20.0 "$node_(1) setdest 900.0 100.0 60.0"
# Setup traffic flow between nodes
# TCP connections between node_(0) and node_(1)

set tcp [new Agent/TCP]
set sink [new Agent/TCPSink]
$ns_ attach-agent $node_(0) $tcp
$ns_ attach-agent $node_(1) $sink
$ns_ connect $tcp $sink
set ftp [new Application/FTP]
$ftp attach-agent $tcp
$ns_ at 3.0 "$ftp start" 
$ns_ at 99.0 "$ftp stop"

#
# Tell nodes when the simulation ends
#
for {set i 0} {$i < $val(nn) } {incr i} {
    $ns_ at 100.0 "$node_($i) reset";
}
$ns_ at 99.0 "stop"
$ns_ at 99.01 "puts \"NS EXITING...\" ; $ns_ halt"
proc stop {} {
    global ns_ tracefd namtrace
    $ns_ flush-trace
    close $tracefd
    close $namtrace
    exit 0
}

puts "Starting Simulation..."
$ns_ run

