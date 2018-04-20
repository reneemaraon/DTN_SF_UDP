from flask import Flask
from flask import request
app = Flask(__name__)

@app.route('/sync')
def sync():
    return 'Calling sync function'

@app.route('/packet_in', methods=['POST'])
def packet_in():
    if request.method == 'POST':
        return request.data