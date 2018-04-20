from abc import ABCMeta, abstractmethod 
import time

class SDDTN:
    __metaclass__ = ABCMeta

    @abstractmethod
    def __init__(self, mode):
        if mode == 2:
            self.flow_tables = {}
            self.to_be_deleted = {}
        elif mode == 1:
            self.flow_tables = {}
            self.to_be_deleted = {}
        
    @abstractmethod
    def boot(self, request):
        pass
    
    @abstractmethod
    def packet_in(self, request):
        pass

    @abstractmethod
    def alive(self, request):
        pass

    def sim_call(self, request):
        if request['event_type'] == 0:
            return self.packet_in(request)
        elif request['event_type'] == 1:
            return self.alive(request)
        elif request['event_type'] == 2:
            return self.boot(request)

    def install_flow(self, ip_address, flow):
        self.__validate_ip_address(ip_address)
        
        # Get existing flow tables to check list of existing node ids
        
        if not ip_address in self.flow_tables.keys():
            self.flow_tables[ip_address] = {}
        # Append to specific flow table in DB
        flow['synced' ] = False
        flow['expiration'] += time.time()
        slot = str(flow['priority'])
        self.flow_tables[ip_address][slot] = flow.copy()
    
    def install_flow_to_all(self, flow):
        for key, value in self.flow_tables.iteritems():
            self.install_flow(key, flow.copy())
    
    def delete_flow_from_all(self, priority):
        for key, value in self.flow_tables.iteritems():
            self.delete_flow(key, priority)
    
    def delete_flow(self, ip_address, priority):
        slot = str(priority)
        del self.flow_tables[ip_address][slot]
        if not ip_address in self.to_be_deleted.keys():
            self.to_be_deleted[ip_address] = []
        if not slot in self.to_be_deleted[ip_address]:
            self.to_be_deleted[ip_address].append(slot)

    def clean_expired_flows(self, ip_address):
        for key, value in self.flow_tables[ip_address].iteritems():
            if value['expiration'] < time.time():
                self.delete_flow(ip_address, key)

    def get_flow_table(self, ip_address):
        try:
            return self.flow_tables[ip_address]
        except KeyError:
            print('IP Address ' + ip_address + ' does not exist in flow tables')

    def sync_flows(self, ip_address):
        # Query specific flow table and delete expired flows
        # Return as a JSON response
        self.clean_expired_flows(ip_address)

        flow_table = self.flow_tables[ip_address]

        response = {}
        response['install'] = []
        if not ip_address in self.to_be_deleted.keys():
            response['delete'] = []
        else:
            response['delete'] = self.to_be_deleted[ip_address]
        self.to_be_deleted[ip_address] = []

        for key, value in flow_table.iteritems():
            if not value['synced']:
                flow_to_sync = flow_table[key].copy()
                flow_to_sync.pop('synced', None)
                response['install'].append(flow_to_sync)
                self.flow_tables[ip_address][key]['synced'] = True
        return response
    
    def __validate_ip_address(self, ip_address):
        if not type(ip_address) == str:
            raise TypeError('IP Address should be a string')

        ip_address = ip_address.split('.')

        if len(ip_address) != 4:
            raise ValueError('IP Address format is invalid, IP Address given: ' + '.'.join(ip_address))

        for section in ip_address:
            if len(section) < 1 or len(section) > 3:
                raise ValueError('IP Address format is invalid, IP Address given: ' + '.'.join(ip_address))

# Management of Dead Nodes
            