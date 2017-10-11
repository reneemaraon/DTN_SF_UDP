## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    obj = bld.create_ns3_program('dtn_sf_udp',
                                 ['wifi', 'internet', 'netanim'])
    obj.source = ['dtn.cc','mypacket.cc']
