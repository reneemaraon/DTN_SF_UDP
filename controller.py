# import time
import zmq
import time
import json 

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")

count = 1
while True:
	json_str = socket.recv()
	print "Received packet in %s" % json_str.decode("utf-8")
	data_print = json.loads(json_str.decode("utf-8"))
	print "Received event type: "
	print data_print['eventType']

	

	# DITO MO IPROCESS ANG NARECEIVE NA JSON


	jsonreply = {"install": [{"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"1"}, {"priority": "200","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"0"}],"delete": [{"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"1"}, {"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"2"}]}
	print jsonreply	


	socket.send_json(jsonreply)
	count+=1;