from SDDTN import SDDTN

class test(SDDTN):
    def boot(self):
        pass
    
    def packet_in(self):
        pass
    
    def alive(self):
        pass

x = test()

x.add_flow('1.1.1.1', 'test')
print(x.get_flow_table('1.1.1.1'))