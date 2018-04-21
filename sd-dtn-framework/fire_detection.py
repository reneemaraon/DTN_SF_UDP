import pprint
from SDDTN import SDDTN

class FireDetection(SDDTN):
    def __init__(self, mode):
        if mode == 2:
            self.flow_tables = {}
            self.to_be_deleted = {}
        elif mode == 1:
            self.flow_tables = {}
            self.to_be_deleted = {}
        self.emergency_sensor = -99
        self.emergency_mode = 0

    def boot(self, request):
        # DROP IF LESS THAN 7 ANG DATA AVERAGE
        flow = {
            'priority': 10,
            'ip_address': '*',
            'sensor_id': '*',
            'gt_data_ave': '*',
            'eq_data_ave': '*',
            'lt_data_ave': '7',
            'gt_smallest_val': '*',
            'eq_smallest_val': '*',
            'lt_smallest_val': '*',
            'gt_largest_val': '*',
            'eq_largest_val': '*',
            'lt_largest_val': '*',
            'action': '0',
            'expiration': 3000
        }
        # PACKET IN IF MORE THAN 10 ANG DATA AVERAGE
        flow1 = {
            'priority': 20,
            'ip_address': '*',
            'sensor_id': '*',
            'gt_data_ave': '10',
            'eq_data_ave': '*',
            'lt_data_ave': '*',
            'gt_smallest_val': '*',
            'eq_smallest_val': '*',
            'lt_smallest_val': '*',
            'gt_largest_val': '*',
            'eq_largest_val': '*',
            'lt_largest_val': '*',
            'action': '2',
            'expiration': 3000
        }

        #ALL OTHERS, SPREAD
        flow2 = {
            'priority': 1000,
            'ip_address': '*',
            'sensor_id': '*',
            'gt_data_ave': '*',
            'eq_data_ave': '*',
            'lt_data_ave': '*',
            'gt_smallest_val': '*',
            'eq_smallest_val': '*',
            'lt_smallest_val': '*',
            'gt_largest_val': '*',
            'eq_largest_val': '*',
            'lt_largest_val': '*',
            'action': '255',
            'expiration': 3000
        }
        self.install_flow(str(request['ip_address']), flow) # lacks flow
        self.install_flow(str(request['ip_address']), flow1) # lacks flow
        self.install_flow(str(request['ip_address']), flow2) # lacks flow
        response = self.sync_flows(request['ip_address'])
        return response
    
    def packet_in(self, request):
        if self.emergency_mode == 0:
            if (request['data_ave'] > 10):
                flow = {
                    'priority': 1,
                    'ip_address': '*',
                    'sensor_id': '*',
                    'gt_data_ave': '*',
                    'eq_data_ave': '*',
                    'lt_data_ave': '*',
                    'gt_smallest_val': '*',
                    'eq_smallest_val': '*',
                    'lt_smallest_val': '*',
                    'gt_largest_val': '*',
                    'eq_largest_val': '*',
                    'lt_largest_val': '*',
                    'action': '2',
                    'expiration': 3000
                }
                flow['sensor_id'] = str(request['sensor_id'])
                self.install_flow_to_all(flow)
                self.emergency_sensor = request['sensor_id']
                print "Entering Emergency Mode"
                self.emergency_mode =1

        elif self.emergency_mode ==1:
            if (request['data_ave'] < 10 and request['sensor_id'] == self.emergency_sensor):
                print "Exiting Emergency Mode"
                self.emergency_mode=0
                self.delete_flow_from_all(1)
    
        response = self.sync_flows(request['ip_address'])
        return response
    
    def alive(self, request):
        response = self.sync_flows(request['ip_address'])
        return response

    def demo(self):
        flow = {
            'priority': 1,
            'ip_address': '172.24.1.1',
            'sensor_id': 1,
            'gt_data_ave': 1,
            'eq_data_ave': 1,
            'lt_data_ave': 1,
            'gt_smallest_val': 1,
            'eq_smallest_val': 1,
            'lt_smallest_val': 1,
            'gt_largest_val': 1,
            'eq_largest_val': 1,
            'lt_largest_val': 1,
            'action': 1,
            'expiration': 3000
        }

        flow1 = {
            'priority': 2,
            'ip_address': '172.24.1.2',
            'sensor_id': 1,
            'gt_data_ave': 1,
            'eq_data_ave': 1,
            'lt_data_ave': 1,
            'gt_smallest_val': 1,
            'eq_smallest_val': 1,
            'lt_smallest_val': 1,
            'gt_largest_val': 1,
            'eq_largest_val': 1,
            'lt_largest_val': 1,
            'action': 1,
            'expiration': 3000
        }

        flow2 = {
            'priority': 3,
            'ip_address': '172.24.1.3',
            'sensor_id': 1,
            'gt_data_ave': 1,
            'eq_data_ave': 1,
            'lt_data_ave': 1,
            'gt_smallest_val': 1,
            'eq_smallest_val': 1,
            'lt_smallest_val': 1,
            'gt_largest_val': 1,
            'eq_largest_val': 1,
            'lt_largest_val': 1,
            'action': 1,
            'expiration': 3000
        }
        pp = pprint.PrettyPrinter(indent=1)

        x.install_flow('172.24.1.1', flow)
        x.install_flow('172.24.1.2', flow)
        x.install_flow('172.24.1.3', flow)
        x.delete_flow('172.24.1.3', 1)
        x.install_flow_to_all( flow1)

        # Start demo for packet in
        packet = {
            'event_type': 0,
            'mobile_id': 0,
            'ip_address': '172.24.1.1',
            'sensor_id': 1,
            'sensor_ip_address': '172.24.1.2',
            'data_ave': 15.5,
            'smallest_val' : 4.3,
            'largest_val': 15.0,
            'time_received': 123,
            'datapoints': '1'
        }

        packet1 = {
            'event_type': 0,
            'mobile_id': 0,
            'ip_address': '172.24.1.1',
            'sensor_id': 1,
            'sensor_ip_address': '172.24.1.2',
            'data_ave': 5.5,
            'smallest_val' : 4.3,
            'largest_val': 15.0,
            'time_received': 123,
            'datapoints': '1'
        }
        pp.pprint(x.sim_call(packet))
        pp.pprint(x.sim_call(packet1))

# x = FireDetection(2)
# x.demo()

