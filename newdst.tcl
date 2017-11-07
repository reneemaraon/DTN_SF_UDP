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
$ns_ at 65 "$node_(1) setdest 60 300 20"


#14 s R
$ns_ at 68 "$node_(1) setdest 340 300 20"
#3 s U
$ns_ at 72 "$node_(1) setdest 340 240 20"
#14 s L
$ns_ at 75 "$node_(1) setdest 60 240 20"
# #3 s D GUMAGANA NAMAN PAG COMMENTED OUT KASO NAGSSISEGV ERROR SA NOHUP.OUT/ OKS SA NETANIM
$ns_ at 89 "$node_(1) setdest 60 300 20"


#14 s R
$ns_ at 92 "$node_(1) setdest 340 300 20"
#3 s U
$ns_ at 106 "$node_(1) setdest 340 240 20"
#14 s L
$ns_ at 109 "$node_(1) setdest 60 240 20"
# #3 s D GUMAGANA NAMAN PAG COMMENTED OUT KASO NAGSSISEGV ERROR SA NOHUP.OUT/ OKS SA NETANIM
$ns_ at 123 "$node_(1) setdest 60 300 20"


#14 s R
$ns_ at 126 "$node_(1) setdest 340 300 20"
#3 s U
$ns_ at 140 "$node_(1) setdest 340 240 20"
#14 s L
$ns_ at 143 "$node_(1) setdest 60 240 20"
# #3 s D GUMAGANA NAMAN PAG COMMENTED OUT KASO NAGSSISEGV ERROR SA NOHUP.OUT/ OKS SA NETANIM
$ns_ at 157 "$node_(1) setdest 60 300 20"


#14 s R
$ns_ at 160 "$node_(1) setdest 340 300 20"
#3 s U
$ns_ at 174 "$node_(1) setdest 340 240 20"
#14 s L
$ns_ at 177 "$node_(1) setdest 60 240 20"
# #3 s D GUMAGANA NAMAN PAG COMMENTED OUT KASO NAGSSISEGV ERROR SA NOHUP.OUT/ OKS SA NETANIM
$ns_ at 191 "$node_(1) setdest 60 300 20"
