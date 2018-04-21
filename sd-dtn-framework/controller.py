# import time
import zmq
import time
import json 
import pprint

# from SDDTN import SDDTN
from fire_detection import FireDetection


context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")
x = FireDetection(2)
count = 1
while True:
	json_str = socket.recv()
	data_print = json.loads(json_str.decode("utf-8"))
	print "Received event type: ",data_print['event_type']
	print "Received json details %s" % json_str.decode("utf-8")

	if (data_print['event_type']==0):
		print data_print['datapoints']	

	# DITO MO IPROCESS ANG NARECEIVE NA JSON


	jsonreply = {"install": [{"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"1"}, {"priority": "200","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"0"}],"delete": [{"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"1"}, {"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*", "action":"2"}]}
	# print jsonreply	

	jsonreply2 = x.sim_call(data_print)
	print "Json replied: ", jsonreply2
	print "\n\n"
	socket.send_json(jsonreply2)
	count+=1;