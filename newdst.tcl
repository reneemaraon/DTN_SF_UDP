$node_(0) set X_ 30
$node_(0) set Y_ 200
$node_(1) set X_ 230
$node_(1) set Y_ 200
$node_(2) set X_ 30
$node_(2) set Y_ 100
$node_(3) set X_ 430
$node_(3) set Y_ 200
$ns_ at 0.00 "$node_(2) setdest 230 100 50"
$ns_ at 30.00 "$node_(2) setdest 430 100 50"
