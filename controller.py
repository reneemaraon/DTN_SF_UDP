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

	

	flow = {"priority": "134","ipAdd":"10.0.0.99", "rule1":"*", "rule2":"*", "rule3":"*", "rule4":"*", "rule5":"*", "rule6":"*", "rule7":"*", "rule8":"*", "rule9":"*", "rule10":"*", "action":"1"}
	flow["rule1"] = str(count)


	jsonreply = {"install": [{"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*"}, {"priority": "200","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*"}],"delete": [{"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*"}, {"priority": "100","ipAdd": "10.0.0.8","sensorId": "12","dataAveGT": "*","dataAveLT": "*","dataAveET": "*","smallestValGT": "*","smallestValLT": "4","smallestValET": "*","largestValGT": "*","largestValLT": "*","largestValET": "*"}]}
	print jsonreply	


	socket.send_json(jsonreply)
	count+=1;