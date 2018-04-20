from flask import Flask
from flask import request
import json
app = Flask(__name__)
from flask_pymongo import PyMongo

app.config['MONGO3_URI'] = 'mongodb://sdtn:password@ds247678.mlab.com:47678/sd-dtn-controller'
app.config['MONGO3_PORT'] = 47678
app.config['MONGO3_DBNAME'] = 'dbname_three'
mongo = PyMongo(app, config_prefix='MONGO3')

@app.route('/sync')
def sync():
    ipAddr = request.args.get('ipAddr')
    flowIds = request.args.get('flowIds')
    flowIds = flowIds.split(',')
    flowTables = mongo.db.flow_tables.find_one_or_404({'ipAddr': ipAddr})
    filtered = []
    for flow in flowTables["flow_table"]:
        if flow["flowId"] not in flowIds:
            filtered.append(flow)

    return json.dumps(filtered)

@app.route('/packet_in', methods=['POST'])
def packet_in():
    if request.method == 'POST':
        save = json.loads(request.data)
        mongo.db.saved_bundles.insert(save)
        return str(request.data)