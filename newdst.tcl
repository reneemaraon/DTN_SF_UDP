#sensor
$node_(0) set X_ 30
$node_(0) set Y_ 200
#mobile
$node_(1) set X_ 30
$node_(1) set Y_ 50
#mobile
$node_(2) set X_ 400
$node_(2) set Y_ 100
#base
$node_(3) set X_ 600
$node_(3) set Y_ 200

$ns_ at 9.21324 "$node_(1) setdest 30 100 20"
$ns_ at 15.2123 "$node_(1) setdest 350 100 50"
$ns_ at 30.212244 "$node_(2) setdest 600 100 20"


# $node_(0) set X_ 30
# $node_(0) set Y_ 200
# $node_(1) set X_ 230
# $node_(1) set Y_ 200
# $node_(2) set X_ 30
# $node_(2) set Y_ 50
# $node_(3) set X_ 430
# $node_(3) set Y_ 200
# $ns_ at 0.00 "$node_(2) setdest 230 100 50"
# $ns_ at 30.00 "$node_(2) setdest 430 100 50"