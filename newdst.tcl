#latest
#sensor
$node_(0) set X_ 0
$node_(0) set Y_ 400
#mobile
$node_(1) set X_ 60 
$node_(1) set Y_ 300
#base
$node_(2) set X_ 400
$node_(2) set Y_ 400

#14 s R
$ns_ at 0 "$node_(1) setdest 340 300 20"
#3 s U
$ns_ at 14 "$node_(1) setdest 340 240 20"
#14 s L
$ns_ at 17 "$node_(1) setdest 60 240 20"
#3 s D
$ns_ at 31 "$node_(1) setdest 60 300 20"

#14 s R
$ns_ at 34 "$node_(1) setdest 340 300 20"
#3 s U
$ns_ at 48 "$node_(1) setdest 340 240 20"
#14 s L
$ns_ at 51 "$node_(1) setdest 60 240 20"
# #3 s D GUMAGANA NAMAN PAG COMMENTED OUT KASO NAGSSISEGV ERROR SA NOHUP.OUT/ OKS SA NETANIM
# $ns_ at 65 "$node_(1) setdest 60 300 20"


#2nd
# #sensor
# $node_(0) set X_ 30
# $node_(0) set Y_ 250
# #mobile
# $node_(1) set X_ 30
# $node_(1) set Y_ 100
# #mobile
# $node_(2) set X_ 400
# $node_(2) set Y_ 150
# #base
# $node_(3) set X_ 600
# $node_(3) set Y_ 250

# $ns_ at 9.21324 "$node_(1) setdest 30 150 20"
# $ns_ at 15.2123 "$node_(1) setdest 350 150 50"
# $ns_ at 30.212244 "$node_(2) setdest 600 150 20"
# $ns_ at 37 "$node_(2) setdest 600 50 35"
# $ns_ at 39 "$node_(2) setdest 600 0 35"
# $ns_ at 40 "$node_(2) setdest 300 0 20"
# $ns_ at 40 "$node_(1) setdest 350 300 50"
# $ns_ at 44 "$node_(1) setdest 500 300 50"

#1st
# #sensor
# $node_(0) set X_ 30
# $node_(0) set Y_ 200
# #mobile
# $node_(1) set X_ 30
# $node_(1) set Y_ 50
# #mobile
# $node_(2) set X_ 400
# $node_(2) set Y_ 100
# #base
# $node_(3) set X_ 600
# $node_(3) set Y_ 200

# $ns_ at 9.21324 "$node_(1) setdest 30 100 20"
# $ns_ at 15.2123 "$node_(1) setdest 350 100 50"
# $ns_ at 30.212244 "$node_(2) setdest 600 100 20"
