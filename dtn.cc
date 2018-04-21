#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h" 
#include "ns3/v4ping-helper.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include <iostream>
#include <cmath>
#include <vector> 
#include "ns3/ipv4-static-routing-helper.h"
#include "mypacket.h"
#include <fstream>
#include <sstream>
#include "ns3/ns2-mobility-helper.h"
#include "ns3/qos-tag.h"
#include "ns3/netanim-module.h"
#include "QueueStruct.h"
#include "flowtable.h"
#include <sstream>
#include <string.h>
#include <string>
#include <zmq.hpp>
#include <json/json.h>
#include <typeinfo>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>



#define sleep(n) Sleep(n)
#endif


using namespace ns3;

static void CourseChange(std::ostream *myos, std::string foo, Ptr<const MobilityModel> mobility){
  Ptr<Node> node = mobility->GetObject<Node>();
  Vector pos = mobility->GetPosition(); // Get position
  Vector vel = mobility->GetVelocity(); // Get velocity
  
  std::cout.precision(5);
  *myos << Simulator::Now() << "; NODE: " << node->GetId() << "; POS: x=" << pos.x << ", y=" << pos.y
    << ", z=" << pos.z << "; VEL: x=" << vel.x << ", y=" << vel.y
    << ", z=" << vel.z << std::endl;
}

typedef std::map<Ptr<Socket>,int> sockOrder;

//////////////////////////////DTN EXAMPLE CLASS DEC//////////////////////////////
class DtnExample{
public:
  DtnExample();
  bool Configure(int argc, char **argv);
  void Run();
  uint32_t GetNodeNum();
  void PacketIn(int locx, int locy, Ptr<Packet>, int nodeId);
  // void Report(std::ostream & os);

  std::string traceFile;
  std::string logFile;
  std::ofstream myos;
  std::ofstream ysaout;
  std::ifstream bufferInput;

private:
  void CreateNodes();
  void CreateDevices();
  void InstallInternetStack();
  void InstallApplications();
  void PopulateArpCache();
  void InsertToMobile(int nodeId, std::string flow[], int priority);
  uint32_t seed;
  uint32_t nodeNum;
  double duration;
  bool pcap;
  bool printRoutes;
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
};

//////////////////////////////DTN APP CLASS DEC//////////////////////////////
class DtnApp : public Application{
  public:
    DtnApp();
    virtual ~DtnApp();
    void ReceiveBundle(Ptr<Socket> socket); //CALLED NG INSTALL APPLICATION
    void ReceiveHello(Ptr<Socket> socket);
    void Retransmit(InetSocketAddress sendTo, int32_t id, int32_t retx); //CALLED NG SEND HELLO AND CHECK QUEUES AND SEND MORE
    void SendMore(InetSocketAddress sendTo, int32_t id, int32_t retx); //CALLED NG RETRANSMIT AND RECEIVE BUNDLE
    // void ScheduleTx();

  protected:
    virtual void StopApplication(void);
    void PrintBuffers(void); //CALLED NG START APPLICATION
    void CheckQueues(uint32_t bundletype); //CALLED NG SELF AND START APPLICATION
    void SendAP(Ipv4Address srcaddr, Ipv4Address dstaddr, uint32_t seqno, Time srctimestamp); //CALLED NG RECEIVE BUNDLE
    int IsDuplicate(Ptr<Packet> pkt, Ptr<Queue> queue); //CALLED NG RECEIVE BUNDLE
    int AntipacketExists(Ptr<Packet> pkt); //CALLED NG RECEIVE BUNDLE
    void RemoveBundle(Ptr<Packet> pkt); //CALLED NG RECEIVE BUNDLE

    DtnExample        *dtnExample;
    Ptr<Node>         m_node;
    Ptr<Socket>       m_socket;
    std::vector<Ptr<Packet> > newpkt;
    std::vector<Ptr<Packet> > retxpkt;
    Ptr<Queue>        m_antipacket_queue;
    Ptr<Queue>        m_queue;
    Ptr<Queue>        m_helper_queue;
    Ptr<Queue>        m_packetin_queue;
    Ptr<Queue>        m_dtb_queue;
    Ptr<Queue>        m_base_queue;
    Ptr<WifiMacQueue> mac_queue;
    Address           m_peer;
    EventId           m_sendEvent;
    bool              m_running;
    uint32_t          m_serverReadSize;
    uint32_t          neighbors;
    InetSocketAddress *neighbor_address;
    double            *neighbor_last_seen;
    uint32_t          *currentServerRxBytes;
    int32_t           **neighbor_hello_bundles;
    int32_t           **neighbor_sent_bundles;
    int32_t           **neighbor_sent_aps;
    double            **neighbor_sent_ap_when;
    uint32_t          bundles;
    InetSocketAddress *bundle_address;
    int32_t           *bundle_seqno;
    int32_t           *bundle_retx;
    uint32_t          *bundle_size;
    double            *bundle_ts;
    double            firstSendTime[10000];
    double            lastSendTime[10000];
    uint32_t          lastTxBytes[10000];
    uint32_t          currentTxBytes[10000];
    uint32_t          totalTxBytes[10000];
    InetSocketAddress *sendTos;
    int32_t           ids[10000];
    int32_t           retxs[10000];
    int               NumFlows;
    uint32_t          drops;
    double            t_c;
    uint32_t          b_s;
    uint32_t          *b_a;
    uint32_t          rp;
    uint32_t          cc;
    uint32_t          stationary;
};

//////////////////////////////SENSOR CLASS DEC//////////////////////////////
class Sensor: public DtnApp{
  public:
    void SensorSetup(Ptr<Node> node, DtnExample *dtnExample);
    void StartApplication(void);
    void ReceiveHello(Ptr<Socket> socket); //CALLED NG INSTALL APPLICATION
    
    void GenerateData(uint32_t first);
    void StoreInBuffer(std::string tempor);
    void CreateBundle();

    int bufferCount;
    int entryLength;
    int bufferLength;
    float secondsInterval;
    QueueStruct buffer;
    float dataSizeInBundle;
    int dataIDSize;
    int dataTimeSize;
    int nextID;
    int maxID;
    int bundleCount;
    float dataSum;
    int largestData;
    int smallestData;
    uint32_t destinationNode;
    std::string timeNow;
};

//////////////////////////////MOBILE CLASS DEC//////////////////////////////
class Mobile: public DtnApp{
  public:
    void MobileSetup(Ptr<Node> node, DtnExample *dtnEx);
    void StartApplication(void);
    void SendHello(Ptr<Socket> socket, double endTime, Time pktInterval, uint32_t first); //CALLED BY SELF AND INSTALL APPLICATION
    void ReceiveHello(Ptr<Socket> socket);
    void ReceiveBundle(Ptr<Socket> socket);
    void CheckQueues(uint32_t bundletype); //CALLED NG SELF AND START APPLICATION
    void Alive(int first);
    void HandleReply(json_object *jsonreply);

    int CheckMatch(std::string ichcheck[]);
    void CheckPacketInQueues(); //CALLED NG SELF AND START APPLICATION
    void TriggerInsertFlow(); 
    void ScheduleTx();

    FlowTable flowTable;
};
Ptr<Mobile> *app;

//////////////////////////////BASE CLASS DEC//////////////////////////////
class Base: public DtnApp{
  public:
    void BaseSetup(Ptr<Node> node, DtnExample *dtnEx);
    void StartApplication(void);
    void SendHello(Ptr<Socket> socket, double endTime, Time pktInterval, uint32_t first); //CALLED BY SELF AND INSTALL APPLICATION
    void ReceiveHello(Ptr<Socket> socket);
    
    // void ReceiveTeleport(Ptr<Packet> packet);
};
Ptr<Base> basenode;





/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////DTN EXAMPLE FUNCTION DEFS//////////////////////////
/////////////////////////////////////////////////////////////////////////////////
DtnExample::DtnExample() :
  seed(1),
  nodeNum(116),
  duration(3600),
  pcap(false),
  printRoutes(true)
{
  std::cout<<"DTNEX\n"; 
}

bool DtnExample::Configure(int argc, char **argv){
  std::cout<<"CONFIG\n";
  CommandLine cmd;

  cmd.AddValue("seed", "RNG seed.", seed);
  cmd.AddValue("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue("nodeNum", "Number of nodes.", nodeNum);
  cmd.AddValue("duration", "Simulation time, s.", duration);
  cmd.AddValue("traceFile", "Ns2 movement trace file", traceFile);
  cmd.AddValue("logFile", "Log file", logFile);
  
  cmd.Parse(argc, argv);
  SeedManager::SetSeed(seed); 
  return true;
}

void DtnExample::Run(){
  std::cout<<"RUN\n";
  Config::SetDefault("ns3::ArpCache::WaitReplyTimeout", StringValue("100000000ns")); // 0.1 s, default: 1.0 s
  Config::SetDefault("ns3::ArpCache::MaxRetries", UintegerValue(10)); // default: 3
  Config::SetDefault("ns3::ArpCache::AliveTimeout", StringValue("5000000000000ns")); // 5000 s, default: 120 s
  CreateNodes();
  CreateDevices();
  InstallInternetStack();
  InstallApplications();
  PopulateArpCache();
  std::cout << "Starting simulation for " << duration << " s, " <<
    "seed value " << seed << "\n";
  
  Simulator::Stop(Seconds(duration));
  AnimationInterface anim("animDTN2.xml");
  anim.SetBackgroundImage ("/home/dtn14/Documents/workspace/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP/bround.jpg", -10.5,-42,2.11,2.11,1);
  // anim.SetBackgroundImage ("/home/dtn14/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP/bround.jpg", -10.5,-42,2.11,2.11,1);
  Simulator::Run();
  myos.close(); // close log file
  ysaout.close();
  Simulator::Destroy();
}

// void DtnExample::Report(std::ostream &){ 
// }

void DtnExample::PacketIn(int locx, int locy, Ptr<Packet> pkt, int nodeId){
  Ptr<Packet> cpkt = pkt->Copy();
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(bndlHeader);


  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REQ);

  socket.connect("tcp://localhost:5555");
  int recvcount = 1;
  while (recvcount<2){

    //WORKING ON PACKET IN
    
    struct json_object *object, *tmp;

    object = json_object_new_object();

   //EVENT TYPE
    tmp = json_object_new_int(0);
    json_object_object_add(object, "event_type", tmp);


    //MOBILE ID
    tmp = json_object_new_int(nodeId);
    json_object_object_add(object, "mobile_id", tmp);


    //MOBILE IP ADDRESS
    char mobileIpAddress[1024]="";
    sprintf(mobileIpAddress,"10.0.0.%d",(nodeId + 1));
    tmp = json_object_new_string(mobileIpAddress);
    json_object_object_add(object, "ip_address", tmp);


    //SENSOR ID
    tmp = json_object_new_int((int)bndlHeader.GetSensorID());
    json_object_object_add(object, "sensor_id", tmp);


    std::stringstream sensorIP;
    bndlHeader.GetDst().Print(sensorIP);
    tmp = json_object_new_string(sensorIP.str().c_str());
    json_object_object_add(object, "sensor_ip_address", tmp);

    //dataAve
    tmp = json_object_new_int(bndlHeader.GetDataAverage());
    json_object_object_add(object, "data_ave", tmp);

    //largestVal
    tmp = json_object_new_int(bndlHeader.GetLargestVal());
    json_object_object_add(object, "largest_val", tmp);

    //smallestVal
    tmp = json_object_new_int(bndlHeader.GetSmallestVal());
    json_object_object_add(object, "smallest_val", tmp);

    //timestamp
    tmp = json_object_new_int(Simulator::Now().GetSeconds()-2);
    json_object_object_add(object, "time_received", tmp);


    uint8_t *buffer1 = new uint8_t[cpkt->GetSize()+1];
    cpkt->CopyData(buffer1, cpkt->GetSize());
    buffer1[cpkt->GetSize()]='\0';
            
    std::string s = std::string(buffer1, buffer1+cpkt->GetSize());

    tmp = json_object_new_string(s.c_str());

    std::cout<<"string is :"<<s<<"\n";
    json_object_object_add(object, "datapoints", tmp);

    //writing 
    printf("%s\n", json_object_to_json_string(object));
    printf("size: %u \n", (unsigned)strlen(json_object_to_json_string(object)));

  


    zmq::message_t request(strlen(json_object_to_json_string(object)));

    memcpy(request.data(), json_object_to_json_string(object), strlen(json_object_to_json_string(object)));

    
    socket.send(request);




    //REPLY HANDLING

    zmq::message_t reply;
    socket.recv(&reply);

    json_object *jstring = json_tokener_parse(static_cast<char*>(reply.data()));

    app[nodeId]->HandleReply(jstring);
    
    app[nodeId]->flowTable.listPrinter();
    

    recvcount++;
  }
}

void DtnExample::InsertToMobile(int nodeId, std::string flow[], int priority){
  app[nodeId]->flowTable.insertWithPriority(priority,flow);
  app[nodeId]->flowTable.listPrinter();
}

uint32_t DtnExample::GetNodeNum(){
  return nodeNum;
}

void DtnExample::CreateNodes(){
  std::cout<<"CREATENODES\n";
  Ns2MobilityHelper ns2 = Ns2MobilityHelper(traceFile);
  myos.open(logFile.c_str());
  ysaout.open("ysaout.txt");
  ysaout<<"YSAOUT\n";
  std::cout << "Creating " << nodeNum << " nodes.\n";
  nodes.Create(nodeNum);
  // Name nodes
  for(uint32_t i = 0; i < nodeNum; ++i){
    std::ostringstream os;
    os << "node-" << i;
    Names::Add(os.str(), nodes.Get(i));
  }
  ns2.Install();
  Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
  MakeBoundCallback(&CourseChange, &myos));
}

void DtnExample::CreateDevices(){
  std::cout<<"CREATEDEVICES\n";
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue("ErpOfdmRate6Mbps"));
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel(wifiChannel.Create());
  QosWifiMacHelper wifiMac = QosWifiMacHelper::Default();
  wifi.SetRemoteStationManager("ns3::IdealWifiManager");
  wifiPhy.Set("TxPowerLevels", UintegerValue(1) ); // default: 1
  wifiPhy.Set("TxPowerStart",DoubleValue(9.1)); // default: 16.0206
  wifiPhy.Set("TxPowerEnd", DoubleValue(9.1)); // default: 16.0206
  wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-74.5) ); // default: -96
  wifiPhy.Set("CcaMode1Threshold", DoubleValue(-77.5) ); // default: -99
  wifiPhy.Set("RxNoiseFigure", DoubleValue(7) ); // default: 7
  wifiPhy.Set("TxGain", DoubleValue(1.0) ); // default: 1.0
  wifiPhy.Set("RxGain", DoubleValue(1.0) ); // deafult: 1.0
  wifiMac.SetType("ns3::AdhocWifiMac");
  devices = wifi.Install(wifiPhy, wifiMac, nodes);
  
  // NetDeviceContainer aps;
  // Ptr<NetDevice> sensor1 = devices.Get(1);
  // Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (sensor1);
  // Ptr<YansWifiPhy> phy = DynamicCast<YansWifiPhy>(wifiDevice->GetPhy());
  // phy->SetTxPowerStart(3.2);
  // phy->SetTxPowerEnd(3.2);


  if(pcap)
    wifiPhy.EnablePcapAll(std::string("rtprot"));
}

void DtnExample::InstallInternetStack(){
  std::cout<<"INSTALLINTERNETSTACK\n";
  Ipv4StaticRoutingHelper rtprot;
  InternetStackHelper stack;
  stack.SetRoutingHelper(rtprot);
  stack.Install(nodes);
  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign(devices);
  
  if(printRoutes){
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("rtprot.routes", std::ios::out);
    rtprot.PrintRoutingTableAllAt(Seconds(8), routingStream);
  }
}

void DtnExample::InstallApplications(){
  std::cout<<"INSTALLAPP\n";
  uint32_t node_num;
  uint32_t numOfEntries;
  uint32_t entrySize;
  float secondsIntervalinput;

  app = new Ptr<Mobile>[nodeNum];
  TypeId udp_tid = TypeId::LookupByName("ns3::UdpSocketFactory");



  //set up base

  for(uint32_t i = 0; i < nodeNum; ++i){ 
    // if(i<=nodeNum-3){
    if(i==1){
      std::cout<<"SENSOR: "<<"\n";
      Ptr<Sensor> app1;
      app1 = CreateObject<Sensor>();  
      app1->SensorSetup(nodes.Get(i), this);
      app1->destinationNode=2;

      std::cout << "Opening Sensor Buffer Details"<< " \n";
      // bufferInput.open("/home/dtn14/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP/sensorBufferDetails");
      bufferInput.open("/home/dtn14/Documents/workspace/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP/sensorBufferDetails");
      if(bufferInput.is_open()){
        while(bufferInput >> node_num >> numOfEntries >> entrySize >> secondsIntervalinput){
          if(node_num==i){
            app1->bufferCount=0;
            app1->entryLength = entrySize;
            app1->secondsInterval = secondsIntervalinput;
            app1->bufferLength = numOfEntries;
          std::cout<<"seconds interval" <<secondsIntervalinput<<"\n";
          }
        }
      }
      else{
        std::cout<<"Unable to open Sensor Buffer Details\n";
      }
      bufferInput.close();


      nodes.Get(i)->AddApplication(app1);
      app1->SetStartTime(Seconds(0.5 + 0.00001*i));
      app1->SetStopTime(Seconds(5000.));
      Ptr<Socket> dst = Socket::CreateSocket(nodes.Get(i), udp_tid);
      char dststring[1024]="";
      sprintf(dststring,"10.0.0.%d",(i + 1));
      InetSocketAddress dstlocaladdr(Ipv4Address(dststring), 50000);
      dst->Bind(dstlocaladdr);
      dst->SetRecvCallback(MakeCallback(&DtnApp::ReceiveBundle, app1));
      
      Ptr<Socket> source = Socket::CreateSocket(nodes.Get(i), udp_tid);
      InetSocketAddress remote(Ipv4Address("255.255.255.255"), 80);
      source->SetAllowBroadcast(true);
      source->Connect(remote);
      std::cout<< "node "<< i <<" getnode "<< dst->GetNode()<< " dst-> "<< dst <<" "<< dststring<< " source-> " <<source<<"\n";

      app1->GenerateData(1);

      Ptr<Socket> recvSink = Socket::CreateSocket(nodes.Get(i), udp_tid);
      InetSocketAddress local(Ipv4Address::GetAny(), 80);
      recvSink->Bind(local);
      recvSink->SetRecvCallback(MakeCallback(&Sensor::ReceiveHello, app1));
    }
    // else if(i==nodeNum-2){
    else if(i==0){
      std::cout<<"MOBILE: "<<"\n";
      // Ptr<Mobile> app;
      app[i] = CreateObject<Mobile>();  
      app[i]->MobileSetup(nodes.Get(i), this);

      nodes.Get(i)->AddApplication(app[i]);
      app[i]->SetStartTime(Seconds(0.5 + 0.00001*i));
      app[i]->SetStopTime(Seconds(5000.));
      Ptr<Socket> dst = Socket::CreateSocket(nodes.Get(i), udp_tid);
      char dststring[1024]="";
      sprintf(dststring,"10.0.0.%d",(i + 1));
      InetSocketAddress dstlocaladdr(Ipv4Address(dststring), 50000);
      dst->Bind(dstlocaladdr);
      dst->SetRecvCallback(MakeCallback(&Mobile::ReceiveBundle, app[i]));
      
      Ptr<Socket> source = Socket::CreateSocket(nodes.Get(i), udp_tid);
      InetSocketAddress remote(Ipv4Address("255.255.255.255"), 80);
      source->SetAllowBroadcast(true);
      source->Connect(remote);
      std::cout<< "node "<< i <<" getnode "<< dst->GetNode()<< " dst-> "<< dst <<" "<< dststring<< " source-> " <<source<<"\n";
      
      Ptr<Socket> recvSink = Socket::CreateSocket(nodes.Get(i), udp_tid);
      InetSocketAddress local(Ipv4Address::GetAny(), 80);
      recvSink->Bind(local);
      recvSink->SetRecvCallback(MakeCallback(&Mobile::ReceiveHello, app[i]));

      app[i]->SendHello(source, duration, Seconds(0.1 + 0.00085*i), 1);
      // TriggerInsertFlow();
      // std::cout <<"TRIGGERRRRR\n";
      // Simulator::Schedule(Seconds(1.0), &Mobile::TriggerInsertFlow, this);
      // std::cout << "At time " << Simulator::Now().GetSeconds() << " scheduled insert of flow\n";
      app[i]->ScheduleTx();
      app[i]->Alive(1);

      

    

    }
    // else if(i==nodeNum-1){
    else if(i==2){
      std::cout<<"BASE: "<<"\n";
      // Ptr<Base> basenode;
      basenode = CreateObject<Base>();  
      basenode->BaseSetup(nodes.Get(i), this);

      nodes.Get(i)->AddApplication(basenode);
      basenode->SetStartTime(Seconds(0.5 + 0.00001*i));
      basenode->SetStopTime(Seconds(5000.));
      Ptr<Socket> dst = Socket::CreateSocket(nodes.Get(i), udp_tid);
      char dststring[1024]="";
      sprintf(dststring,"10.0.0.%d",(i + 1));
      InetSocketAddress dstlocaladdr(Ipv4Address(dststring), 50000);
      dst->Bind(dstlocaladdr);
      dst->SetRecvCallback(MakeCallback(&DtnApp::ReceiveBundle, basenode));
      
      Ptr<Socket> source = Socket::CreateSocket(nodes.Get(i), udp_tid);
      InetSocketAddress remote(Ipv4Address("255.255.255.255"), 80);
      source->SetAllowBroadcast(true);
      source->Connect(remote);
      std::cout<< "node "<< i <<" getnode "<< dst->GetNode()<< " dst-> "<< dst <<" "<< dststring<< " source-> " <<source<<"\n";

      Ptr<Socket> recvSink = Socket::CreateSocket(nodes.Get(i), udp_tid);
      InetSocketAddress local(Ipv4Address::GetAny(), 80);
      recvSink->Bind(local);
      recvSink->SetRecvCallback(MakeCallback(&Base::ReceiveHello, basenode));

      basenode->SendHello(source, duration, Seconds(0.1 + 0.00085*i), 1);

    }
  }

}

void DtnExample::PopulateArpCache(){ 
  std::cout<<"POPULATEARPCACHE\n";
  Ptr<ArpCache> arp = CreateObject<ArpCache>(); 
  arp->SetAliveTimeout(Seconds(3600 * 24 * 365)); 
  for(NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i){ 
    Ptr<Ipv4L3Protocol> ip =(*i)->GetObject<Ipv4L3Protocol>(); 
    NS_ASSERT(ip !=0); 
    ObjectVectorValue interfaces; 
    ip->GetAttribute("InterfaceList", interfaces); 
    for(uint32_t j = 0; j != ip->GetNInterfaces(); j ++){
      Ptr<Ipv4Interface> ipIface = ip->GetInterface(j);
      NS_ASSERT(ipIface != 0); 
      Ptr<NetDevice> device = ipIface->GetDevice(); 
      NS_ASSERT(device != 0); 
      Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress()); 
      for(uint32_t k = 0; k < ipIface->GetNAddresses(); k ++){ 
        Ipv4Address ipAddr = ipIface->GetAddress(k).GetLocal(); 
        if(ipAddr == Ipv4Address::GetLoopback()) 
          continue; 
        ArpCache::Entry * entry = arp->Add(ipAddr); 
        entry->MarkWaitReply(0); 
        entry->MarkAlive(addr); 
      } 
    } 
  } 
  for(NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i){ 
    Ptr<Ipv4L3Protocol> ip =(*i)->GetObject<Ipv4L3Protocol>(); 
    NS_ASSERT(ip !=0); 
    ObjectVectorValue interfaces; 
    ip->GetAttribute("InterfaceList", interfaces);
    for(uint32_t j = 0; j != ip->GetNInterfaces(); j ++){
      Ptr<Ipv4Interface> ipIface = ip->GetInterface(j);
      ipIface->SetAttribute("ArpCache", PointerValue(arp)); 
    } 
  } 
}





/////////////////////////////////////////////////////////////////////////////
//////////////////////////////DTN APP FUNCTION DEFS//////////////////////////
/////////////////////////////////////////////////////////////////////////////
DtnApp::DtnApp()
  : m_socket(0),
    newpkt(0),
    retxpkt(0),
    m_antipacket_queue(0),
    m_queue(0),
    m_helper_queue(0),
    m_packetin_queue(0),
    m_dtb_queue(0),
    m_base_queue(0),
    mac_queue(0),
    m_peer(),
    m_sendEvent(),
    m_running(false),
    m_serverReadSize(200), /* Is this OK? */
    neighbors(0),
    neighbor_address(0),
    neighbor_last_seen(0),
    currentServerRxBytes(0),
    neighbor_hello_bundles(0),
    neighbor_sent_bundles(0),
    neighbor_sent_aps(0),
    neighbor_sent_ap_when(0),
    bundles(0),
    bundle_address(0),
    bundle_seqno(0),
    bundle_retx(0),
    bundle_size(0),
    bundle_ts(0),
    sendTos(0),
    NumFlows(0),
    drops(0),
    t_c(0.8),
    b_s(1000000),
    b_a(0),
    rp(0), // 0: Epidemic, 1: Spray and Wait
    cc(0)  // 0: No congestion control, 1: Static t_c, 2: Dynamic t_c
{
  std::cout<<"DTNAPP\n";
}

DtnApp::~DtnApp(){
  std::cout<<"~DTNAPP\n";  
  m_socket = 0;
}

void DtnApp::ReceiveBundle(Ptr<Socket> socket){

  Address ownaddress;

  socket->GetSockName(ownaddress);
  InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom(ownaddress); //receiver address   
  while(socket->GetRxAvailable() > 0){
    Address from;
    Ptr<Packet> p = socket->RecvFrom(from);
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from); //sender address

    // hello here
    uint32_t i = 0;
    uint32_t found = 0;
    while((i < neighbors) &&(found == 0)){
      if(address.GetIpv4() == neighbor_address[i].GetIpv4()){
        found = 1;
      } 
      else
        i++;
    }
    if(found == 0){
      ++neighbors;
      neighbor_address=(InetSocketAddress*)realloc(neighbor_address,neighbors*sizeof(InetSocketAddress));
      neighbor_address[i]=address.GetIpv4();
      neighbor_last_seen=(double*)realloc(neighbor_last_seen,neighbors*sizeof(double));
      b_a=(uint32_t*)realloc(b_a,neighbors*sizeof(uint32_t));
      neighbor_hello_bundles=(int32_t**)realloc(neighbor_hello_bundles,neighbors*sizeof(int32_t*));
      neighbor_hello_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_bundles=(int32_t**)realloc(neighbor_sent_bundles,neighbors*sizeof(int32_t*));
      neighbor_sent_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_aps=(int32_t**)realloc(neighbor_sent_aps,neighbors*sizeof(int32_t*));
      neighbor_sent_aps[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_ap_when=(double**)realloc(neighbor_sent_ap_when,neighbors*sizeof(double*));
      neighbor_sent_ap_when[i]=(double*)calloc(1000,sizeof(double));
      for(uint32_t j=0; j < 1000; j++){
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();



    int src_seqno = 0;
    QosTag tag;
    int packet_type = 0;
    if(p->PeekPacketTag(tag))
      packet_type = tag.GetTid();
    if(packet_type == 5){ // Ack
      p->RemoveAllByteTags();
      p->RemoveAllPacketTags();
      uint8_t *msg=new uint8_t[p->GetSize()+1];
      p->CopyData(msg, p->GetSize());
      msg[p->GetSize()]='\0';
      const char *src=reinterpret_cast<const char *>(msg);
      char word[1024];
      strcpy(word, "");
      int j=0, n=0;
      int32_t id = 0;
      int32_t retx = 0;
      while(sscanf(src, "%1023s%n", word, &n) == 1){
        if(j == 0)
          id=strtol(word,NULL,16);
        else
          retx=strtol(word,NULL,16);
        strcpy(word,"");
        src += n;
        j++;
      }
      delete [] msg;
      SendMore(address.GetIpv4(), id, retx);
      return;
    } 
    else{
      FlowIdTag ftag = 0;
      if(p->PeekPacketTag(ftag))
        src_seqno = ftag.GetFlowId();
      std::stringstream msg;
      msg.clear();
      msg.str("");
      char seqnostring[1024]="";
      sprintf(seqnostring," %x %x", src_seqno, packet_type); // Add: how much data received; will be used by the sender. If total received > bundle size: discard packet.
      msg << seqnostring;
      Ptr<Packet> ack = Create<Packet>((uint8_t*) msg.str().c_str(), msg.str().length());
      if(m_socket == 0){
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
        Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
        Ipv4Address ipaddr =(ipv4->GetAddress(1, 0)).GetLocal();
        InetSocketAddress local = InetSocketAddress(ipaddr, 50000);
        m_socket->Bind(local);    
      }
      ack->AddPacketTag(QosTag(5));
      InetSocketAddress ackaddr(address.GetIpv4(), 50000);
      m_socket->SendTo(ack, 0, ackaddr);
    }
    p->RemoveAllByteTags();
    p->RemoveAllPacketTags();

    i = 0;
    found = 0;
    while((i < bundles) &&(found == 0)){
      if((address.GetIpv4() == bundle_address[i].GetIpv4()) &&(src_seqno == bundle_seqno[i]) &&(packet_type == bundle_retx[i])){
        found = 1;
      } 
      else
      i++;
    }
    if(found == 0){
      i = 0;
      while((i < bundles) &&(found == 0)){
        if(currentServerRxBytes[i] == 0){
          found = 1;
          bundle_address[i] = address.GetIpv4();
          bundle_seqno[i] = src_seqno;
          bundle_retx[i] = packet_type;
          bundle_ts[i] = Simulator::Now().GetSeconds();
          currentServerRxBytes[i] = 0;
        } 
        else
          i++;
      }
    }
    if(found == 0){
      ++bundles;
      bundle_address=(InetSocketAddress*)realloc(bundle_address,bundles*sizeof(InetSocketAddress));
      bundle_address[i] = address.GetIpv4();
      bundle_seqno=(int32_t*)realloc(bundle_seqno,bundles*sizeof(int32_t));
      bundle_seqno[i] = src_seqno;
      bundle_retx=(int32_t*)realloc(bundle_retx,bundles*sizeof(int32_t));
      bundle_retx[i] = packet_type;
      currentServerRxBytes=(uint32_t*)realloc(currentServerRxBytes,bundles*sizeof(uint32_t));
      currentServerRxBytes[i] = 0;
      bundle_size=(uint32_t*)realloc(bundle_size,bundles*sizeof(uint32_t));
      bundle_ts=(double*)realloc(bundle_ts,bundles*sizeof(double));
      bundle_ts[i] = Simulator::Now().GetSeconds();
      newpkt.push_back(p->Copy());
    }
    if(p == 0 && socket->GetErrno() != Socket::ERROR_NOTERROR)
      NS_FATAL_ERROR("Server could not read stream at byte " << currentServerRxBytes[i]);
    if(currentServerRxBytes[i] == 0){
      currentServerRxBytes[i] += p->GetSize();
      newpkt[i] = p;
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      newpkt[i]->RemoveHeader(tHeader);
      if(tHeader.Get() == mypacket::MYTYPE_AP){
        mypacket::APHeader apHeader;
        newpkt[i]->RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize() + 26;
        newpkt[i]->AddHeader(apHeader);
      } 
      else{
        if(tHeader.Get() == mypacket::MYTYPE_BNDL){
          mypacket::BndlHeader bndlHeader;
          newpkt[i]->RemoveHeader(bndlHeader);
          bundle_size[i] = bndlHeader.GetBundleSize() + 29 + 12;
          newpkt[i]->AddHeader(bndlHeader);
        } 
        else{
          // Bundle fragments arrive in wrong order; no bundle header
          currentServerRxBytes[i] = 0;
          return;
        }
      }
      newpkt[i]->AddHeader(tHeader);
    } 
    else{
      if(currentServerRxBytes[i] > bundle_size[i]){
        std::cout<<"Current server bytes: "<<currentServerRxBytes[i]<<" Bundle size: "<<bundle_size[i]<<"\n";
        std::cout << "WTF at time " << Simulator::Now().GetSeconds() <<
          " received " << p->GetSize() <<
          " bytes at " << owniaddress.GetIpv4() <<
          " total bytes " << currentServerRxBytes[i] <<
          " from " << address.GetIpv4() <<
          " seqno " << src_seqno << "\n";
      } 
      else{
        currentServerRxBytes[i] += p->GetSize();
        newpkt[i]->AddAtEnd(p);
      }
    }
    if(currentServerRxBytes[i] == bundle_size[i]){
      currentServerRxBytes[i] = 0;
      Ptr<Packet> qpkt = newpkt[i]->Copy();
      Ptr<Packet> qpkt1 = newpkt[i]->Copy();

      // cout<<"Received:"<<s<<endl;

      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      newpkt[i]->RemoveHeader(tHeader);
      if(tHeader.Get() == mypacket::MYTYPE_AP){
        mypacket::APHeader apHeader;
        newpkt[i]->RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize();
        if((IsDuplicate(qpkt, m_antipacket_queue) == 0) &&((Simulator::Now().GetSeconds() - apHeader.GetSrcTimestamp().GetSeconds()) < 1000.0)){
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
          qpkt->RemoveHeader(tHeader);
          mypacket::APHeader apHeader;
          qpkt->RemoveHeader(apHeader);
          apHeader.SetHopTimestamp(Simulator::Now());
          apHeader.SetHopCount(apHeader.GetHopCount() + 1);
          qpkt->AddHeader(apHeader);
          qpkt->AddHeader(tHeader);
          // bool success = m_antipacket_queue->Enqueue(qpkt);
          // if(success){
          // }
          RemoveBundle(qpkt); 
        }
      } 
      else{
        mypacket::BndlHeader bndlHeader;
        newpkt[i]->RemoveHeader(bndlHeader);
        
        bundle_size[i] = bndlHeader.GetBundleSize();
        if(IsDuplicate(qpkt, m_queue) == 1)
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " received duplicate " << newpkt[i]->GetSize() <<
            " bytes at " << owniaddress.GetIpv4() <<
            " from " << address.GetIpv4() <<
            " bundle hop count: "  <<(unsigned)bndlHeader.GetHopCount() <<
            " sequence number: "  << bndlHeader.GetOriginSeqno() <<
            " bundle queue occupancy: " << m_queue->GetNBytes() << "\n";


        if((IsDuplicate(qpkt, m_queue) == 0) &&(AntipacketExists(qpkt) == 0) &&((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < 1000.0)){
          if(bndlHeader.GetDst() == owniaddress.GetIpv4()){
            float time = Simulator::Now().GetSeconds();
            float delay = Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds() + 1000.0*(bndlHeader.GetNretx());
            std::cout << "At time " << time<<
              " received " << newpkt[i]->GetSize() <<
              " bytes at " << owniaddress.GetIpv4() <<
              "(final dst) from " << address.GetIpv4() <<
              " delay: "  << delay <<
              " bundle hop count: "  <<(unsigned)bndlHeader.GetHopCount() + 1 <<
              " sequence number: "  << bndlHeader.GetOriginSeqno() <<
              " bundle queue occupancy: " << m_queue->GetNBytes() << "\n";
              uint8_t *buffer1 = new uint8_t[newpkt[i]->GetSize()+1];
              newpkt[i]->CopyData(buffer1, newpkt[i]->GetSize());
              buffer1[newpkt[i]->GetSize()]='\0';
              
              std::string s = std::string(buffer1, buffer1+newpkt[i]->GetSize());

              std::cout<<"string is :"<<s<<"\n";
              // std::ofstream datapoints;
              // datapoints.open("datapoints.txt",std::ios_base::app);
              // if(datapoints.is_open()){
              //   std::cout<<"Hello\n";
              //   datapoints<<s;
              //   datapoints<<"HI THERE FAM\n";
              // }
              // datapoints.close();
            
              std::cout<<"y"<<bndlHeader.GetOriginSeqno()<<","<<time<<","<<delay<<std::endl;

              std::string delimiter = ",";
              std::string::size_type sz;
              size_t pos = 0;
              std::string token;
              while((pos = s.find(delimiter)) != std::string::npos){
                  token = s.substr(0, pos);


                  std::cout <<"x"<< token.substr(5,3)<<","<<token.substr(0,4) <<","<< time<<std::endl;
                  s.erase(0, pos + delimiter.length());
              }


              

          } 
          else{
            mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
            qpkt->RemoveHeader(tHeader);
            mypacket::BndlHeader bndlHeader;
            qpkt->RemoveHeader(bndlHeader);
            bndlHeader.SetHopTimestamp(Simulator::Now());
            bndlHeader.SetHopCount(bndlHeader.GetHopCount() + 1);
            qpkt->AddHeader(bndlHeader);
            qpkt->AddHeader(tHeader);
            if((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + qpkt->GetSize()) <= b_s){
              bool success = m_queue->Enqueue(qpkt);

              if(success){
              }
            } 
            else{
              drops++;
              std::cout << "At time " << Simulator::Now().GetSeconds() <<
              " dropped " << newpkt[i]->GetSize() <<
              " bytes at " << owniaddress.GetIpv4() <<
              " from " << address.GetIpv4() <<
              " bundle hop count: "  <<(unsigned)bndlHeader.GetHopCount() <<
              " sequence number: "  << bndlHeader.GetOriginSeqno() <<
              " bundle queue occupancy: " << m_queue->GetNBytes() << "\n";
            }
          }
        }
      }
    }
  }
}

void DtnApp::ReceiveHello(Ptr<Socket> socket){
  Ptr<Packet> packet;
  Address from;
  while(packet = socket->RecvFrom(from)){
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    uint32_t i = 0;
    uint32_t found = 0;
    while((i < neighbors) &&(found == 0)){
      if(address.GetIpv4() == neighbor_address[i].GetIpv4()){
        found = 1;
      } 
      else
        i++;
    }
    if(found == 0){
      ++neighbors;
      neighbor_address=(InetSocketAddress*)realloc(neighbor_address,neighbors*sizeof(InetSocketAddress));
      neighbor_address[i]=address.GetIpv4();
      neighbor_last_seen=(double*)realloc(neighbor_last_seen,neighbors*sizeof(double));
      b_a=(uint32_t*)realloc(b_a,neighbors*sizeof(uint32_t));
      neighbor_hello_bundles=(int32_t**)realloc(neighbor_hello_bundles,neighbors*sizeof(int32_t*));
      neighbor_hello_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_bundles=(int32_t**)realloc(neighbor_sent_bundles,neighbors*sizeof(int32_t*));
      neighbor_sent_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_aps=(int32_t**)realloc(neighbor_sent_aps,neighbors*sizeof(int32_t*));
      neighbor_sent_aps[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_ap_when=(double**)realloc(neighbor_sent_ap_when,neighbors*sizeof(double*));
      neighbor_sent_ap_when[i]=(double*)calloc(1000,sizeof(double));
      for(uint32_t j=0; j < 1000; j++){
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();
    for(uint32_t j=0; j < 1000; j++)
      neighbor_hello_bundles[i][j]=0;
    
    uint8_t *msg=new uint8_t[packet->GetSize()+1];
    packet->CopyData(msg, packet->GetSize());
    msg[packet->GetSize()]='\0';
    const char *src=reinterpret_cast<const char *>(msg);
    char word[1024];
    strcpy(word, "");
    int j=0, n=0;
    int bundle_ids = 0;
    while(sscanf(src, "%1023s%n", word, &n) == 1){
      if(j == 0){
        b_a[i]=atoi(word);
      } 
      else{
        if(j == 1){
          bundle_ids=atoi(word);
        } 
        else{
          if(j <=(bundle_ids + 1)) 
            neighbor_hello_bundles[i][j-2]=strtol(word,NULL,16);
          else
            neighbor_hello_bundles[i][j-2]=-strtol(word,NULL,16);
          int m=0, sent_found=0;
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_hello_bundles[i][j-2] == neighbor_sent_aps[i][m]){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_aps[i][m] != 0) &&(m < 999)){
                neighbor_sent_aps[i][m]=neighbor_sent_aps[i][m+1];
                neighbor_sent_ap_when[i][m]=neighbor_sent_ap_when[i][m+1];
                m++;
              }
              neighbor_sent_aps[i][999]=0;
              neighbor_sent_ap_when[i][999]=0;
            }
          }
        }
      }
      strcpy(word,"");
      src += n;
      j++;
    }
    delete [] msg;
  }
}

void DtnApp::Retransmit(InetSocketAddress sendTo, int32_t id, int32_t retx){
  // Check that this is last call for retransmit, otherwise return

  int index = 0, found = 0;
  while((found == 0) &&(index < NumFlows)){
    if((sendTos[index] == sendTo) &&(ids[index] == id) &&(retxs[index] == retx) &&((Simulator::Now().GetSeconds() - firstSendTime[index]) < 1000.0)){
      found = 1;
      if((Simulator::Now().GetSeconds() - lastSendTime[index] < 1.0)){
        return;
      }
    } 
    else
      index++;
  }
  if(found == 0)
    return;
  
  // Check that we are able to send, otherwise re-schedule and return
  uint32_t i = 0, neighbor_found = 0;
  while((i < neighbors) &&(neighbor_found == 0)){
    if(neighbor_address[i].GetIpv4() == sendTo){
      neighbor_found = 1;
      if((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) > 0.1){
        Simulator::Schedule(Seconds(1.0), &DtnApp::Retransmit, this, sendTo, id, retx);
        return;
      }
    } 
    else{
      i++;
    }
  }
  if(neighbor_found == 0)
    return;
  
  // Retransmit
  currentTxBytes[index] -= lastTxBytes[index];
  SendMore(sendTo, id, retx);
}

void DtnApp::SendMore(InetSocketAddress sendTo, int32_t id, int32_t retx){
  int index = 0, found = 0;
  while((found == 0) &&(index < NumFlows)){
    if((sendTos[index] == sendTo) &&(ids[index] == id) &&(retxs[index] == retx) &&((Simulator::Now().GetSeconds() - firstSendTime[index]) < 1000.0)){
      found = 1;
    } 
    else
      index++;
  }
  if(found == 0)
    return;
  
  if(currentTxBytes[index] < totalTxBytes[index]){ 
    uint32_t left = totalTxBytes[index] - currentTxBytes[index];
    uint32_t dataOffset = currentTxBytes[index] % 1472;
    uint32_t toWrite = 1472 - dataOffset;
    toWrite = std::min(toWrite, left);
    Ptr<Packet> packet = Create<Packet>(toWrite);
    if(currentTxBytes[index] == 0)
      packet = retxpkt[index]->Copy();
    packet->AddPacketTag(FlowIdTag(id));
    packet->AddPacketTag(QosTag(retx));
    InetSocketAddress addr(sendTo.GetIpv4(), 50000);
    m_socket->SendTo(packet, 0, addr);
    currentTxBytes[index] += toWrite;
    lastTxBytes[index] = toWrite;
    lastSendTime[index] = Simulator::Now().GetSeconds();
    Simulator::Schedule(Seconds(1.0), &DtnApp::Retransmit, this, sendTo, id, retx);
  } 
  else
    lastTxBytes[index] = 0;

}

// void DtnApp::ScheduleTx(){
//   // m_sendEvent = Simulator::Schedule (tNext, &DtnApp::SendBundle, this, dstnode, packetsize);
//   m_sendEvent = Simulator::Schedule(Seconds(1.0), &Mobile::TriggerInsertFlow, this);
//   // m_sendEvent = Simulator::Schedule (tNext, &DtnApp::SendBundle, this, dstnode, packetsize);
// }

void DtnApp::StopApplication(void){
  std::cout<<"STOP APP\n";
  m_running = false;

  if(m_sendEvent.IsRunning())
    Simulator::Cancel(m_sendEvent);
  
  if(m_socket)
    m_socket->Close();
}

void DtnApp::PrintBuffers(void){
  uint32_t i=0, currentNeighbors=0;
  while(i < neighbors){
    if((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1)
      currentNeighbors++;
    i++;
  }
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel>();
  std::cout << Simulator::Now().GetSeconds() <<
    " " << m_node->GetId() << ":"<<
    // " " << mac_queue->GetSize() <<
    // " " << m_antipacket_queue->GetNPackets() <<
    " " << m_queue->GetNPackets() << 
    // " " << m_queue->GetNBytes() <<
    // " " << m_dtb_queue->GetNPackets() <<
    // " " << m_packetin_queue->GetNPackets() <<
    " " << currentNeighbors << 
    " " << mobility->GetPosition().x <<
    " " << mobility->GetPosition().y << "\n";
  Simulator::Schedule(Seconds(1.0), &DtnApp::PrintBuffers, this);
}

void DtnApp::CheckQueues(uint32_t bundletype){
  Ptr<Packet> packet;
  Ptr<Packet> firstpacket;

  uint32_t i = 0, n = 0, pkts = 0, send_bundle = 0;
  mypacket::APHeader apHeader;
  mypacket::BndlHeader bndlHeader;

  // Remove expired antipackets and bundles -- do this check only once
  if(bundletype == 2){


    pkts = m_queue->GetNPackets();
    n = 0;

    //iterating through packets in m_queue
    while(n < pkts){
      n++;
      packet = m_queue->Dequeue();
      Ptr<Packet> cpkt = packet->Copy();

      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      packet->RemoveHeader(tHeader);
      packet->RemoveHeader(bndlHeader);
      //if less 750, keep


      if(((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < 1000.0) ||(bndlHeader.GetHopCount() == 0)){
        packet->AddHeader(bndlHeader);
        packet->AddHeader(tHeader);
        bool success = m_queue->Enqueue(packet);
        if(success){
        }
      } 
      else{
        uint32_t d=0;
        while(d < neighbors){
          uint32_t m=0, sent_found=0;
          //rearranging neighbor_sent_bundles, if nasend na ang bundle sa final destination
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_sent_bundles[d][m] ==(int32_t)bndlHeader.GetOriginSeqno()){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_bundles[d][m] != 0) &&(m < 999)){
                //deleting theat sequence number from neighbor_sent_bundles
                neighbor_sent_bundles[d][m]=neighbor_sent_bundles[d][m+1];
                m++;
              }
              neighbor_sent_bundles[d][999]=0;
            }
          }
          d++;
        }
      }
    
    } 
  }
  
  ////////////////////////////////// end of removing expired bundles and antipackets



  //ano meron sa mac_queue
  if(mac_queue->GetSize() < 2){
    
    if(bundletype == 2){
      pkts = m_antipacket_queue->GetNPackets();
      n = 0;

      //iterating
      // while((n < pkts) &&(send_bundle == 0)){
    
      //   n++;
      //   ///// copying apheaders, packet and theader then enqueueing packet again /////
      //   packet = m_antipacket_queue->Dequeue();
      //   mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
      //   packet->RemoveHeader(tHeader);
      //   packet->RemoveHeader(apHeader);
      //   packet->AddHeader(apHeader);
      //   packet->AddHeader(tHeader);
      //   Ptr<Packet> qp = packet->Copy();

      //   //////////////////////////////////////////////////////////////////
        


      //   if((Simulator::Now().GetSeconds() - apHeader.GetHopTimestamp().GetSeconds()) > 0.2){
      //     i = 0;
      //     while((i < neighbors) &&(send_bundle == 0)){
      //       //if bago lang nakita si neighbor, and aneighbor is not origin of antipacket
      //       if(((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.5) &&(neighbor_address[i].GetIpv4() != apHeader.GetOrigin())){
      //         // if(stationary == 0)
      //         int neighbor_has_bundle = 0, ap_sent = 0, j=0;
              
      //         while((neighbor_has_bundle == 0) &&(neighbor_hello_bundles[i][j] != 0) &&(j < 1000)){
      //           //check if neighbor has the antipacket
      //           // std::cout <<neighbor_hello_bundles[i][j]<<" "<<-(int32_t)apHeader.GetOriginSeqno()<<"\n";
      //           if(neighbor_hello_bundles[i][j] == -(int32_t)apHeader.GetOriginSeqno()){
      //             neighbor_has_bundle = 1;
      //           }
      //           else
      //             j++;
      //         }
      //         j=0;

      //         while((neighbor_has_bundle == 0) &&(ap_sent == 0) &&(neighbor_sent_aps[i][j] != 0) &&(j < 1000)){
      //           if((neighbor_sent_aps[i][j] == -(int32_t)apHeader.GetOriginSeqno()) &&(Simulator::Now().GetSeconds() - neighbor_sent_ap_when[i][j] < 1.5))
      //             ap_sent = 1;
      //           else
      //             j++;
      //         }
      //         // if((neighbor_has_bundle == 0) &&(ap_sent == 0)){
      //         if((neighbor_has_bundle == 0) &&(ap_sent == 0)){
      //           //sending antipacket to this person
      //           // std::cout <<"Sending antipacket with sequence number "<<apHeader.GetOriginSeqno()<<" to " <<neighbor_address[i].GetIpv4()<< "\n";
      //           if(stationary ==0)
      //             send_bundle = 1;
      //           j = 0;
      //           while((neighbor_sent_aps[i][j] != 0) &&(neighbor_sent_aps[i][j] != -(int32_t)apHeader.GetOriginSeqno()) &&(j < 999))
      //             j++; //positioning i and j
      //           neighbor_sent_aps[i][j] = -(int32_t)apHeader.GetOriginSeqno();
      //           neighbor_sent_ap_when[i][j] = Simulator::Now().GetSeconds();
      //         } 
      //         else
      //           i++;
      //       } 
      //       else
      //         i++;
      //     }
      //   }
      //   if(send_bundle ==0){
      //     bool success = m_antipacket_queue->Enqueue(qp);
      //     if(success){
      //     }
      //   }
      // }
    }

     
    else{ //if not bundle type ==2
      pkts = m_queue->GetNPackets();
      n = 0;
      while(n < pkts){
        n++;
        packet = m_queue->Dequeue();
        // packet = m_queue->Dequeue();
        mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
        packet->RemoveHeader(tHeader);
        packet->RemoveHeader(bndlHeader);
        //if not old ang bundle
        if((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < 10000.0){
          // ano lang basta di pa bago ang bundle narating sa node na itu
          if(n==1){
            if((Simulator::Now().GetSeconds() - bndlHeader.GetHopTimestamp().GetSeconds()) > 0.2){

              Ipv4Address dst = bndlHeader.GetDst();
              uint8_t spray = bndlHeader.GetSpray();
              i = 0;
              //iterating through neighbors
              while((i < neighbors) &&(send_bundle == 0)){
                // if dest of bundle is neighboraddress, tas bago lang nakita ang neighbor
                if((((bundletype == 0) &&(spray > 0) &&((cc == 0)||(b_a[i] > packet->GetSize()))) ||(dst == neighbor_address[i].GetIpv4())) && \
                 ((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1) &&(neighbor_address[i].GetIpv4() != bndlHeader.GetOrigin())){
                  int neighbor_has_bundle = 0, bundle_sent = 0, j=0;
                  //check kung meron ba siya nung bundle
                  if ((Simulator::Now().GetSeconds()>700)&&(m_node->GetId()==1)){
                   std::cout<<"HEllo\n";
                  }
                  while((neighbor_has_bundle == 0) &&(neighbor_hello_bundles[i][j] != 0) &&(j < 1000)){
                    if((unsigned)neighbor_hello_bundles[i][j] == bndlHeader.GetOriginSeqno())
                      neighbor_has_bundle = 1;
                    else
                      j++;
                  }
                  j = 0;
                  //check if nasend na ba yung bundle sa kanya
                  while((neighbor_has_bundle == 0) &&(bundle_sent == 0) &&(neighbor_sent_bundles[i][j] != 0) &&(j < 1000)){
                    if(neighbor_sent_bundles[i][j] ==(int32_t)bndlHeader.GetOriginSeqno())
                      bundle_sent = 1;
                    else
                      j++;
                  }
                  if((neighbor_has_bundle == 0) &&(bundle_sent == 0)){
                    if(bundletype == 0){
                      if(rp == 1)
                        bndlHeader.SetSpray(spray/2);
                      if(cc > 0){
                        if(packet->GetSize() >= b_a[i])
                          b_a[i] = 0;
                        else
                          b_a[i] -= packet->GetSize();
                      }
                    } 
                    else{
                      // Wait 5.0 seconds before forwarding to other(than dst) nodes
                      bndlHeader.SetHopTimestamp(Simulator::Now() + Seconds(5.0));
                    }

                    send_bundle = 1;
                    j = 0;
                    while((neighbor_sent_bundles[i][j] != 0) &&(j < 999))
                      j++;
                    neighbor_sent_bundles[i][j] =(int32_t)bndlHeader.GetOriginSeqno();
                  } 
                  else
                    i++;
                } 
                else
                  i++;
              }
            }
          }
          packet->AddHeader(bndlHeader);
          packet->AddHeader(tHeader);
          Ptr<Packet> qp = packet->Copy();
          if(n==1){
            firstpacket= packet->Copy();
            if(send_bundle!=1){
            m_queue->Enqueue(qp);
            }
          }
          else{
            bool success = m_queue->Enqueue(qp);
            if(success){
            }
          }
        } 
        else{
          //erase bundle kung di ikaw source tas old na 
          if(((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) > 1000.0) &&(bndlHeader.GetHopCount() == 0) &&(bndlHeader.GetNretx() < 3)){
            bndlHeader.SetSrcTimestamp(Simulator::Now());
            bndlHeader.SetNretx(bndlHeader.GetNretx() + 1);
            uint32_t n=0;
            while(n < neighbors){
              uint32_t m=0, sent_found=0;
              while((m < 1000) &&(sent_found == 0)){
                if(neighbor_sent_bundles[n][m] ==(int32_t)bndlHeader.GetOriginSeqno()){
                  sent_found=1;
                } 
                else
                  m++;
                if(sent_found == 1){
                  while((neighbor_sent_bundles[n][m] != 0) &&(m < 999)){
                    neighbor_sent_bundles[n][m]=neighbor_sent_bundles[n][m+1];
                    m++;
                  }
                  neighbor_sent_bundles[n][999]=0;
                }
              }
              n++;
            }
          }
          if((bndlHeader.GetHopCount() == 0) &&((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) <= 1000.0)){
            packet->AddHeader(bndlHeader);
            packet->AddHeader(tHeader);
            bool success = m_queue->Enqueue(packet);
            if(success){
            }
          }
        }
      }
      packet = firstpacket;


    }
  } 
  else{
    bundletype = 0;
  }
  if(send_bundle == 1){
    if(m_socket == 0){
      m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
      Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
      Ipv4Address ipaddr =(ipv4->GetAddress(1, 0)).GetLocal();
      InetSocketAddress local = InetSocketAddress(ipaddr, 50000);
      m_socket->Bind(local);    
    }    

    InetSocketAddress dstremoteaddr(neighbor_address[i].GetIpv4(), 50000);    
    if(bundletype < 2){
      packet = firstpacket->Copy();
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      Ptr<Packet> anotherp = packet->Copy();

      anotherp->RemoveHeader(tHeader);
      anotherp->RemoveHeader(bndlHeader);
      anotherp->AddHeader(bndlHeader);
      anotherp->AddHeader(tHeader);
      NumFlows++;
      sendTos=(InetSocketAddress*)realloc(sendTos,NumFlows*sizeof(InetSocketAddress));
      sendTos[NumFlows-1] = dstremoteaddr.GetIpv4();
      ids[NumFlows-1] = bndlHeader.GetOriginSeqno();
      retxs[NumFlows-1] = bndlHeader.GetNretx();
      currentTxBytes[NumFlows-1] = std::min((uint32_t)1472, packet->GetSize());
      lastTxBytes[NumFlows-1] = std::min((uint32_t)1472, packet->GetSize());
      firstSendTime[NumFlows-1] = Simulator::Now().GetSeconds();
      lastSendTime[NumFlows-1] = Simulator::Now().GetSeconds();
      totalTxBytes[NumFlows-1] = packet->GetSize();
      if(packet->GetSize() > 1472)
        packet->RemoveAtEnd(packet->GetSize() - 1472);
      packet->AddPacketTag(FlowIdTag(bndlHeader.GetOriginSeqno()));
      packet->AddPacketTag(QosTag(bndlHeader.GetNretx()));
      retxpkt.push_back(packet->Copy());
      Simulator::Schedule(Seconds(1.0), &DtnApp::Retransmit, this, sendTos[NumFlows-1], ids[NumFlows-1], retxs[NumFlows-1]);
    } 
    else{    
      packet->AddPacketTag(FlowIdTag(-apHeader.GetOriginSeqno()));
      packet->AddPacketTag(QosTag(4));
    }
    
    m_socket->SendTo(packet, 0, dstremoteaddr);
    Address ownaddress;
    m_socket->GetSockName(ownaddress);
    //InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom(ownaddress);    
  }
  if(bundletype == 2){
    if(send_bundle == 0)
      CheckQueues(1);
    else //pag may sinend 
      Simulator::Schedule(Seconds(2.0), &DtnApp::CheckQueues, this, 2);
  }
  if(bundletype == 1){

    if(send_bundle == 0)
      CheckQueues(0);
    else
      Simulator::Schedule(Seconds(2.0), &DtnApp::CheckQueues, this, 2);
  }
  if(bundletype == 0){
    if(send_bundle == 0)
      Simulator::Schedule(Seconds(0.01), &DtnApp::CheckQueues, this, 2);
    else
      Simulator::Schedule(Seconds(2.0), &DtnApp::CheckQueues, this, 2);
  }
}

void DtnApp::SendAP(Ipv4Address srcstring, Ipv4Address dststring, uint32_t seqno, Time srctimestamp){
  Ptr<Packet> packet = Create<Packet>(10);
  mypacket::APHeader apHeader;
  apHeader.SetOrigin(srcstring);
  apHeader.SetDst(dststring);
  apHeader.SetOriginSeqno(seqno);
  apHeader.SetHopCount(0);
  apHeader.SetBundleSize(10);
  double newtimestamp = srctimestamp.GetSeconds() -(250.0 -(Simulator::Now().GetSeconds() - srctimestamp.GetSeconds()));
  if(newtimestamp < 0.0)
    newtimestamp = 0.0;
  if((Simulator::Now().GetSeconds() - srctimestamp.GetSeconds()) < 250.0)
    apHeader.SetSrcTimestamp(Seconds(newtimestamp));
  else
    apHeader.SetSrcTimestamp(srctimestamp);
    apHeader.SetHopTimestamp(Simulator::Now());
    packet->AddHeader(apHeader);
    mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
    packet->AddHeader(tHeader);
    bool success = m_antipacket_queue->Enqueue(packet);
    if(success){
      // std::cout << "At time " << Simulator::Now().GetSeconds() <<
      //   " send antipacket with sequence number " <<  apHeader.GetOriginSeqno() <<
      //   " original ts " <<  srctimestamp.GetSeconds() <<
      //   " new ts " <<  apHeader.GetSrcTimestamp().GetSeconds() <<
      //   " from " <<  apHeader.GetOrigin() <<
      // " to " << apHeader.GetDst() << "\n";
      
    }
}

int DtnApp::IsDuplicate(Ptr<Packet> pkt, Ptr<Queue> queue){
  Ptr<Packet> cpkt = pkt->Copy();
  int duplicate = 0; 
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(bndlHeader);
  uint32_t seqno = bndlHeader.GetOriginSeqno();
  uint32_t pkts = queue->GetNPackets();
  uint32_t i = 0;
  while(i < pkts){
    Ptr<Packet> p = queue->Dequeue();
    if(duplicate == 0){
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      p->RemoveHeader(tHeader);
      if(tHeader.Get() == mypacket::MYTYPE_AP)
        mypacket::APHeader bndlHeader;
      else
        mypacket::BndlHeader bndlHeader;
        p->RemoveHeader(bndlHeader);
      if(bndlHeader.GetOriginSeqno() == seqno)
        duplicate = 1;
      p->AddHeader(bndlHeader);
      p->AddHeader(tHeader);
    }
    bool success = queue->Enqueue(p);
    if(success){
    } 
    i++;
  }
  return(duplicate);
}

int DtnApp::AntipacketExists(Ptr<Packet> pkt){
  Ptr<Packet> cpkt = pkt->Copy();
  int apExists = 0;
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(bndlHeader);
  uint32_t seqno = bndlHeader.GetOriginSeqno();
  uint32_t pkts = m_antipacket_queue->GetNPackets();
  uint32_t i = 0;
  while(i < pkts){
    Ptr<Packet> p = m_antipacket_queue->Dequeue();
    if(apExists == 0){
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
      mypacket::APHeader apHeader;
      p->RemoveHeader(tHeader);
      p->RemoveHeader(apHeader);
      if(apHeader.GetOriginSeqno() == seqno)
        apExists = 1;
      p->AddHeader(apHeader);
      p->AddHeader(tHeader);
    }
    // bool success = m_antipacket_queue->Enqueue(p);
    // if(success){
    // }
    i++;
  }
  return(apExists);
}

void DtnApp::RemoveBundle(Ptr<Packet> pkt){
  Ptr<Packet> cpkt = pkt->Copy(); 
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
  mypacket::APHeader apHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(apHeader);
  uint32_t seqno = apHeader.GetOriginSeqno();
  uint32_t pkts = m_queue->GetNPackets();
  uint32_t i = 0, found = 0;
  while(i < pkts){
    Ptr<Packet> p = m_queue->Dequeue();
    if(found == 0){
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      mypacket::BndlHeader bndlHeader;
      p->RemoveHeader(tHeader);
      p->RemoveHeader(bndlHeader);
      if(bndlHeader.GetOriginSeqno() != seqno){
        p->AddHeader(bndlHeader);
        p->AddHeader(tHeader);
        bool success = m_queue->Enqueue(p);
        if(success){
        }  
      } 
      else{
        std::cout<<"Removing bundle of sequence "<<bndlHeader.GetOriginSeqno()<<" from node "<<m_node->GetId()<<"\n";
        found = 1;
        uint32_t n=0;
        while(n < neighbors){
          uint32_t m=0, sent_found=0;
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_sent_bundles[n][m] ==(int32_t)seqno){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_bundles[n][m] != 0) &&(m < 999)){
                neighbor_sent_bundles[n][m]=neighbor_sent_bundles[n][m+1];
                m++;
              }
              neighbor_sent_bundles[n][999]=0;
            }
          }
          n++;
        }
      }
    } 
    else{
      bool success = m_queue->Enqueue(p);
      if(success){
      }
    }
    i++;
  }
  int index = 0;
  while(index < NumFlows){
    if(ids[index] ==(int32_t)seqno)
      ids[index] = 0;
    index++;
  }
}





////////////////////////////////////////////////////////////////////////////
//////////////////////////////SENSOR FUNCTION DEFS//////////////////////////
////////////////////////////////////////////////////////////////////////////
void Sensor::SensorSetup(Ptr<Node> node, DtnExample *dtnEx){
  dtnExample = dtnEx;
  dataSum = 0;
  largestData = 0;
  smallestData = 2000000;
  m_node = node;
  m_antipacket_queue = CreateObject<DropTailQueue>();
  m_queue = CreateObject<DropTailQueue>();
  m_helper_queue = CreateObject<DropTailQueue>();
  m_antipacket_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_helper_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  stationary = 1;
  dataSizeInBundle=5;
  bundleCount =0;
  dataTimeSize=4;
  dataIDSize=dataSizeInBundle-2;
  nextID=000;
  maxID=pow(10,dataIDSize)-1;
  for(int i = 0; i < 10000; i++){
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
  b_s = 1375000 + y->GetInteger(0, 1)*9625000;
}

void Sensor::StartApplication(void){
  std::cout<<"SENSOR START APP\n";
  m_running = true;
  Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice>(m_node->GetDevice(0));
  NS_ASSERT(dev != NULL);
  PointerValue ptr;
  dev->GetAttribute("Mac",ptr);
  Ptr<AdhocWifiMac> mac = ptr.Get<AdhocWifiMac>();
  NS_ASSERT(mac != NULL);
  Ptr<EdcaTxopN> edcaqueue = mac->GetBEQueue();
  NS_ASSERT(edcaqueue != NULL);
  mac_queue = edcaqueue->GetEdcaQueue();    
  NS_ASSERT(mac_queue != NULL);
  mac_queue->SetAttribute("MaxPacketNumber", UintegerValue(1000));
  CheckQueues(2);
  PrintBuffers();
}

void Sensor::ReceiveHello(Ptr<Socket> socket){
  Ptr<Packet> packet;
  Address from;
  while(packet = socket->RecvFrom(from)){
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    uint32_t i = 0;
    uint32_t found = 0;
    while((i < neighbors) &&(found == 0)){
      if(address.GetIpv4() == neighbor_address[i].GetIpv4()){
        found = 1;
      } 
      else
        i++;
    }
    if(found == 0){
      ++neighbors;
      neighbor_address=(InetSocketAddress*)realloc(neighbor_address,neighbors*sizeof(InetSocketAddress));
      neighbor_address[i]=address.GetIpv4();
      neighbor_last_seen=(double*)realloc(neighbor_last_seen,neighbors*sizeof(double));
      b_a=(uint32_t*)realloc(b_a,neighbors*sizeof(uint32_t));
      neighbor_hello_bundles=(int32_t**)realloc(neighbor_hello_bundles,neighbors*sizeof(int32_t*));
      neighbor_hello_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_bundles=(int32_t**)realloc(neighbor_sent_bundles,neighbors*sizeof(int32_t*));
      neighbor_sent_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_aps=(int32_t**)realloc(neighbor_sent_aps,neighbors*sizeof(int32_t*));
      neighbor_sent_aps[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_ap_when=(double**)realloc(neighbor_sent_ap_when,neighbors*sizeof(double*));
      neighbor_sent_ap_when[i]=(double*)calloc(1000,sizeof(double));
      for(uint32_t j=0; j < 1000; j++){
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();
    for(uint32_t j=0; j < 1000; j++)
      neighbor_hello_bundles[i][j]=0;
    
    uint8_t *msg=new uint8_t[packet->GetSize()+1];
    packet->CopyData(msg, packet->GetSize());
    msg[packet->GetSize()]='\0';
    const char *src=reinterpret_cast<const char *>(msg);
    char word[1024];
    strcpy(word, "");
    int j=0, n=0;
    int bundle_ids = 0;
    while(sscanf(src, "%1023s%n", word, &n) == 1){
      if(j == 0){
        b_a[i]=atoi(word);
      } 
      else{
        if(j == 1){
          bundle_ids=atoi(word);
        } 
        else{
          if(j <=(bundle_ids + 1)) 
            neighbor_hello_bundles[i][j-2]=strtol(word,NULL,16);
          else
            neighbor_hello_bundles[i][j-2]=-strtol(word,NULL,16);
          int m=0, sent_found=0;
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_hello_bundles[i][j-2] == neighbor_sent_aps[i][m]){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_aps[i][m] != 0) &&(m < 999)){
                neighbor_sent_aps[i][m]=neighbor_sent_aps[i][m+1];
                neighbor_sent_ap_when[i][m]=neighbor_sent_ap_when[i][m+1];
                m++;
              }
              neighbor_sent_aps[i][999]=0;
              neighbor_sent_ap_when[i][999]=0;
            }
          }
        }
      }
      strcpy(word,"");
      src += n;
      j++;
    }
    delete [] msg;
  }
}

void Sensor::GenerateData(uint32_t first){
  if(first==0){
    // if(bufferCount<bufferLength){
    const char alphanum[] = "0123456789";
    
    std::stringstream holder;
    holder <<Simulator::Now().GetSeconds();
    std::string tempo=std::string(dataTimeSize - holder.str().length(), '0') + holder.str();

    // std::cout << "YSAAAAAAAAAAAA" <<" "<<Simulator::Now().GetSeconds()<<" "<<tempo<<"\n";
    // std::cout<<"Generated Data for node "<<m_node->GetId()<<" at time :"<<Simulator::Now()<<" with data ";
    std::stringstream holder2;
    holder2 << nextID;
    std::string tempor=tempo+"-";
    tempor+=std::string(dataIDSize - holder2.str().length(), '0') + holder2.str();
    tempor+="-";
    if(nextID==maxID){
      nextID=000;
    }
    else{
      nextID++;
    }
    int numOfDigits = entryLength-dataIDSize-dataTimeSize-2;
    float currentSum =0;
    for(int i=0; i<(entryLength-dataIDSize-dataTimeSize-2); i++){
      int randnum = rand() % 10;

      //fixing entry to be less than 10 always
      if (i < (entryLength-dataIDSize-dataTimeSize-3)){
        tempor+= alphanum[0];
      }

      else{
        tempor += alphanum[randnum];
        currentSum +=((int)alphanum[randnum]-48)*(pow(10,(numOfDigits-i-1)));
      }
    }
    // std::cout<<"\n"<<currentSum<<"\n";
    if(currentSum<smallestData){
      smallestData = currentSum;
    }
    if(currentSum>largestData){
      largestData = currentSum;
    }
    dataSum+=currentSum;
    // std::cout << "TEMPOR IS  "<<tempor <<"\n";
    StoreInBuffer(tempor);

    Simulator::Schedule(Seconds(secondsInterval), &Sensor::GenerateData, this, 0);
  }
  else{
    Simulator::Schedule(Seconds(secondsInterval), &Sensor::GenerateData, this, 0);
  }
}

void Sensor::StoreInBuffer(std::string tempor){
    if(buffer.getSize() <= bufferLength ){
      buffer.enqueue(tempor);
      if(buffer.getSize() >= dataSizeInBundle){
        CreateBundle();
      }
    }
    //eviction policy?????????????????
}

void Sensor::CreateBundle(){
  std::string payload="";
  // payload+=Simulator::Now().GetSeconds();
  // payload+=std::to_string(dataSizeInBundle);\n
  for(int y=0; y<dataSizeInBundle; y++){
    payload+=buffer.get(0);
    payload+=",";
    buffer.dequeue();
    // buffer.listPrinter();
  }
  float dataAve =(float)dataSum/(float)dataSizeInBundle;
  // std::cout<<"PAYLOAD: "<<payload;
  // int bndlSize=100000;//?????????????????? how to compute hehe
  std::stringstream bndlData;
  bndlData << payload ;
  uint16_t bndlSize = bndlData.str().length();
  
  // std::cout << "IN UINT8: "<< int(bndlData) <<"\n";
  // Ptr<Packet> packet = Create<Packet>((uint8_t*) msgx.str().c_str(), packetSize);
  Ptr<Packet> packet = Create<Packet>((uint8_t*) bndlData.str().c_str(), bndlSize);
  mypacket::BndlHeader bndlHeader;
  uint8_t cnt =(uint8_t)m_node->GetId();
  char srcstring[1024]="";
  sprintf(srcstring,"10.0.0.%d",(m_node->GetId() + 1));
  char dststring[1024]="";
  sprintf(dststring,"10.0.0.%d",(destinationNode+1));
  bndlHeader.SetOrigin(srcstring);
  bndlHeader.SetDst(dststring);
  bndlHeader.SetOriginSeqno(bundleCount);
  bndlHeader.SetHopCount(0);
  bndlHeader.SetSpray(4);
  bndlHeader.SetNretx(0);
  bndlHeader.SetBundleSize(bndlSize);
  bndlHeader.SetSrcTimestamp(Simulator::Now());
  bndlHeader.SetHopTimestamp(Simulator::Now());
  bndlHeader.SetSensorID(cnt);
  bndlHeader.SetDataAverage((float)dataAve);
  bndlHeader.SetLargestVal(largestData);
  bndlHeader.SetSmallestVal(smallestData);
  packet->AddHeader(bndlHeader);
  mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
  packet->AddHeader(tHeader);
  bundleCount++;
  dataSum=0;
  largestData=0;
  smallestData =2000000;
  if((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + packet->GetSize()) <= b_s){

    // HERE IS TO COMMENT OUT IF BACK TO 1000 PACKET SENSOR
    // if (m_queue->GetNPackets()==10){
    //   Ptr<Packet> pkt = m_queue->Dequeue();
    //   mypacket::TypeHeader tHeader2(mypacket::MYTYPE_BNDL);
    //   mypacket::BndlHeader bndlHeader2;
    //   pkt->RemoveHeader(tHeader2);
    //   pkt->RemoveHeader(bndlHeader2);
    //   std::cout<<"Dropped bundle of sequence number: "<<bndlHeader2.GetOriginSeqno()<<" because queue is full.\n";

    // }
    // END HERE

    bool success = m_queue->Enqueue(packet);
    if(success){
      std::cout << "At time " << Simulator::Now().GetSeconds() <<
        " created bundle with sequence number " <<  bndlHeader.GetOriginSeqno() <<
        " from " <<  bndlHeader.GetOrigin() <<
        " to " << bndlHeader.GetDst() << "\n";
    }
  } 
  else{
    std::cout << "At time " << Simulator::Now().GetSeconds() <<
      " tried to send bundle with sequence number " <<  bndlHeader.GetOriginSeqno() <<
      " from " <<  bndlHeader.GetOrigin() <<
      " to " << bndlHeader.GetDst() << "\n";
  }
}





////////////////////////////////////////////////////////////////////////////
//////////////////////////////MOBILE FUNCTION DEFS//////////////////////////
////////////////////////////////////////////////////////////////////////////
void Mobile::MobileSetup(Ptr<Node> node, DtnExample *dtnEx){
  // std::cout <<"PUMASOK SA MOBILE SETUP\n";
  dtnExample = dtnEx;
  m_node = node;
  m_antipacket_queue = CreateObject<DropTailQueue>();
  m_queue = CreateObject<DropTailQueue>();
  m_helper_queue = CreateObject<DropTailQueue>();
  m_packetin_queue = CreateObject<DropTailQueue>();
  m_dtb_queue = CreateObject<DropTailQueue>();
  m_antipacket_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_dtb_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_helper_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_packetin_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  stationary = 0;
  for(int i = 0; i < 10000; i++){
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
  b_s = 1375000 + y->GetInteger(0, 1)*9625000;

  // std::string tempArr[]={"10.0.0.8", "50", "300", "101", "102", "101", "102", "101", "102", "101", "102", "0"};
  // std::string tempArr2[]={"10.0.0.5", "*", "0", "*", "*", "*", "*", "*", "*", "*", "*", "2"};
  // std::string tempArr3[]={"10.0.0.5", "*", "*", "*", "*", "*", "*", "*", "*", "*", "*", "1"};
  // std::string tempArr2[]={"10.0.0.22", "*", "0", "*", "*", "*", "*", "*", "*", "*", "*", "2"};
  // std::string tempArr3[]={"10.0.0.12", "*", "*", "*", "*", "*", "*", "*", "*", "*", "*", "1"};
  // flowTable.insertWithPriority(50, tempArr);
  // std::cout <<"NEWNEWNEW\n";
  // flowTable.listPrinter();
  // flowTable.insertWithPriority(100, tempArr2);
  // flowTable.insertWithPriority(150, tempArr3);
  // flowTable.listPrinter();
}

void Mobile::StartApplication(void){
  std::cout<<"MOBILE START APP\n";
  m_running = true;
  Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice>(m_node->GetDevice(0));
  NS_ASSERT(dev != NULL);
  PointerValue ptr;
  dev->GetAttribute("Mac",ptr);
  Ptr<AdhocWifiMac> mac = ptr.Get<AdhocWifiMac>();
  NS_ASSERT(mac != NULL);
  Ptr<EdcaTxopN> edcaqueue = mac->GetBEQueue();
  NS_ASSERT(edcaqueue != NULL);
  mac_queue = edcaqueue->GetEdcaQueue();    
  NS_ASSERT(mac_queue != NULL);
  mac_queue->SetAttribute("MaxPacketNumber", UintegerValue(1000));
  CheckQueues(2);
  CheckPacketInQueues();
  PrintBuffers();
}

void Mobile::SendHello(Ptr<Socket> socket, double endTime, Time pktInterval, uint32_t first){
  if(first == 0){
    double now(Simulator::Now().GetSeconds());
    if(now < endTime){
      std::stringstream msg;
      msg.clear();
      msg.str("");
      char seqnostring[1024]="";
      if(cc == 2){
        if((drops == 0) &&(t_c < 0.9)){
          t_c += 0.01;
        } 
        else{
          if((drops > 0) &&(t_c > 0.5))
            t_c = t_c * 0.8;
          drops = 0;
        }
      }
      if((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes()) >=(uint32_t)(t_c * b_s))
        sprintf(seqnostring,"%d",0);
      else
        sprintf(seqnostring,"%d",((uint32_t)(t_c * b_s) - m_queue->GetNBytes() - m_antipacket_queue->GetNBytes()));
      msg << seqnostring;
      uint32_t pkts = m_queue->GetNPackets();   
      // Reorder packets: put the least forwarded first
      uint32_t n = 0;
      Ptr<Packet> packet;
      while(n < pkts){
        n++;
        packet = m_queue->Dequeue();
        bool success = m_helper_queue->Enqueue(packet);
        if(success){
        }
      }
      uint32_t m = 0;
      while(m < pkts){
        m++;
        uint32_t min_count = 10000, min_seqno = 0, helper_pkts = m_helper_queue->GetNPackets();
        n = 0;
        while(n < helper_pkts){
          n++;
          packet = m_helper_queue->Dequeue();
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet->RemoveHeader(tHeader);
          packet->RemoveHeader(bndlHeader);
          int index = 0;
          uint32_t count = 0;
          while(index < NumFlows){
            if(ids[index] ==(int32_t)bndlHeader.GetOriginSeqno())
              count++;
            index++;
          }
          if(count < min_count){
            min_count = count;
            min_seqno = bndlHeader.GetOriginSeqno();
          }
          packet->AddHeader(bndlHeader);
          packet->AddHeader(tHeader);
          bool success = m_helper_queue->Enqueue(packet);
          if(success){
          }
        }
        int min_found = 0;
        while(min_found == 0){
          packet = m_helper_queue->Dequeue();
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet->RemoveHeader(tHeader);
          packet->RemoveHeader(bndlHeader);
          if(bndlHeader.GetOriginSeqno() == min_seqno){
            min_found = 1;
            packet->AddHeader(bndlHeader);
            packet->AddHeader(tHeader);
            bool success = m_queue->Enqueue(packet);
            if(success){
            }
          } 
          else{
            packet->AddHeader(bndlHeader);
            packet->AddHeader(tHeader);
            bool success = m_helper_queue->Enqueue(packet);
            if(success){
            }
          }
        }
      }
      // End of reorder  
      char seqnostring_b[1024]="";
      sprintf(seqnostring_b," %d",pkts);
      msg << seqnostring_b;
      for(uint32_t i = 0; i < pkts; ++i){
        Ptr<Packet> p = m_queue->Dequeue();
        if(msg.str().length() < 2280){
        // The default value of MAC-level MTU is 2296
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          p->RemoveHeader(tHeader);
          p->RemoveHeader(bndlHeader);
          uint32_t src_seqno = bndlHeader.GetOriginSeqno();
          char seqnostring_a[1024]="";
          sprintf(seqnostring_a," %x",(src_seqno));
          msg << seqnostring_a;
          p->AddHeader(bndlHeader);
          p->AddHeader(tHeader);
        } 
        else{
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " too big Hello(B)(" << msg.str().length() << ") bytes.\n";
        }
        bool success = m_queue->Enqueue(p);
        if(success){
        }  
      }
      uint32_t apkts = m_antipacket_queue->GetNPackets();
      for(uint32_t i = 0; i < apkts; ++i){
        Ptr<Packet> p = m_antipacket_queue->Dequeue();
        if(msg.str().length() < 2280){
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
          mypacket::APHeader apHeader;
          p->RemoveHeader(tHeader);
          p->RemoveHeader(apHeader);
          uint32_t src_seqno = apHeader.GetOriginSeqno();
          char seqnostring_a[1024]="";
          sprintf(seqnostring_a," %x",(src_seqno));
          msg << seqnostring_a;
          p->AddHeader(apHeader);
          p->AddHeader(tHeader);
        } 
        else{
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " too big Hello(AP)(" << msg.str().length() << ") bytes.\n";                   
        }
        bool success = m_antipacket_queue->Enqueue(p);
        if(success){
        }
      }
      Ptr<Packet> pkt = Create<Packet>((uint8_t*) msg.str().c_str(), msg.str().length());
      pkt->AddPacketTag(QosTag(6)); // High priority 
      socket->Send(pkt);
      Simulator::Schedule(Seconds(0.1), &Mobile::SendHello, this, socket, endTime, Seconds(0.1), 0);
    } 
    else
      socket->Close();
  } 
  else
    Simulator::Schedule(pktInterval, &Mobile::SendHello, this, socket, endTime, pktInterval, 0);
}

void Mobile::ReceiveHello(Ptr<Socket> socket){
  Ptr<Packet> packet;
  Address from;
  while(packet = socket->RecvFrom(from)){
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    uint32_t i = 0;
    uint32_t found = 0;
    while((i < neighbors) &&(found == 0)){
      if(address.GetIpv4() == neighbor_address[i].GetIpv4()){
        found = 1;
      } 
      else
        i++;
    }
    if(found == 0){
      ++neighbors;
      neighbor_address=(InetSocketAddress*)realloc(neighbor_address,neighbors*sizeof(InetSocketAddress));
      neighbor_address[i]=address.GetIpv4();
      neighbor_last_seen=(double*)realloc(neighbor_last_seen,neighbors*sizeof(double));
      b_a=(uint32_t*)realloc(b_a,neighbors*sizeof(uint32_t));
      neighbor_hello_bundles=(int32_t**)realloc(neighbor_hello_bundles,neighbors*sizeof(int32_t*));
      neighbor_hello_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_bundles=(int32_t**)realloc(neighbor_sent_bundles,neighbors*sizeof(int32_t*));
      neighbor_sent_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_aps=(int32_t**)realloc(neighbor_sent_aps,neighbors*sizeof(int32_t*));
      neighbor_sent_aps[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_ap_when=(double**)realloc(neighbor_sent_ap_when,neighbors*sizeof(double*));
      neighbor_sent_ap_when[i]=(double*)calloc(1000,sizeof(double));
      for(uint32_t j=0; j < 1000; j++){
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();
    for(uint32_t j=0; j < 1000; j++)
      neighbor_hello_bundles[i][j]=0;
    
    uint8_t *msg=new uint8_t[packet->GetSize()+1];
    packet->CopyData(msg, packet->GetSize());
    msg[packet->GetSize()]='\0';
    const char *src=reinterpret_cast<const char *>(msg);
    char word[1024];
    strcpy(word, "");
    int j=0, n=0;
    int bundle_ids = 0;
    while(sscanf(src, "%1023s%n", word, &n) == 1){
      if(j == 0){
        b_a[i]=atoi(word);
      } 
      else{
        if(j == 1){
          bundle_ids=atoi(word);
        } 
        else{
          if(j <=(bundle_ids + 1)) 
            neighbor_hello_bundles[i][j-2]=strtol(word,NULL,16);
          else
            neighbor_hello_bundles[i][j-2]=-strtol(word,NULL,16);
          int m=0, sent_found=0;
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_hello_bundles[i][j-2] == neighbor_sent_aps[i][m]){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_aps[i][m] != 0) &&(m < 999)){
                neighbor_sent_aps[i][m]=neighbor_sent_aps[i][m+1];
                neighbor_sent_ap_when[i][m]=neighbor_sent_ap_when[i][m+1];
                m++;
              }
              neighbor_sent_aps[i][999]=0;
              neighbor_sent_ap_when[i][999]=0;
            }
          }
        }
      }
      strcpy(word,"");
      src += n;
      j++;
    }
    delete [] msg;
  }
}

void Mobile::ReceiveBundle(Ptr<Socket> socket){
  // std::cout << "SA MOBILE\n ";
  //m_node or GetNode() is yung receiver
  Address ownaddress;
  socket->GetSockName(ownaddress);
  InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom(ownaddress); //receiver address   
  while(socket->GetRxAvailable() > 0){
    Address from;
    Ptr<Packet> p = socket->RecvFrom(from);
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from); //sender address

    // hello here
    uint32_t i = 0;
    uint32_t found = 0;
    while((i < neighbors) &&(found == 0)){
      if(address.GetIpv4() == neighbor_address[i].GetIpv4()){
        found = 1;
      } 
      else
        i++;
    }
    if(found == 0){
      ++neighbors;
      neighbor_address=(InetSocketAddress*)realloc(neighbor_address,neighbors*sizeof(InetSocketAddress));
      neighbor_address[i]=address.GetIpv4();
      neighbor_last_seen=(double*)realloc(neighbor_last_seen,neighbors*sizeof(double));
      b_a=(uint32_t*)realloc(b_a,neighbors*sizeof(uint32_t));
      neighbor_hello_bundles=(int32_t**)realloc(neighbor_hello_bundles,neighbors*sizeof(int32_t*));
      neighbor_hello_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_bundles=(int32_t**)realloc(neighbor_sent_bundles,neighbors*sizeof(int32_t*));
      neighbor_sent_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_aps=(int32_t**)realloc(neighbor_sent_aps,neighbors*sizeof(int32_t*));
      neighbor_sent_aps[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_ap_when=(double**)realloc(neighbor_sent_ap_when,neighbors*sizeof(double*));
      neighbor_sent_ap_when[i]=(double*)calloc(1000,sizeof(double));
      for(uint32_t j=0; j < 1000; j++){
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();



    int src_seqno = 0;
    QosTag tag;
    int packet_type = 0;
    if(p->PeekPacketTag(tag))
      packet_type = tag.GetTid();
    

    if(packet_type == 5){ // Ack
      p->RemoveAllByteTags();
      p->RemoveAllPacketTags();
      uint8_t *msg=new uint8_t[p->GetSize()+1];
      p->CopyData(msg, p->GetSize());
      msg[p->GetSize()]='\0';
      const char *src=reinterpret_cast<const char *>(msg);
      char word[1024];
      strcpy(word, "");
      int j=0, n=0;
      int32_t id = 0;
      int32_t retx = 0;
      while(sscanf(src, "%1023s%n", word, &n) == 1){
        if(j == 0)
          id=strtol(word,NULL,16);
        else
          retx=strtol(word,NULL,16);
        strcpy(word,"");
        src += n;
        j++;
      }
      delete [] msg;
      SendMore(address.GetIpv4(), id, retx);
      return;
    } 
    else{
      FlowIdTag ftag = 0;
      if(p->PeekPacketTag(ftag))
        src_seqno = ftag.GetFlowId();
      std::stringstream msg;
      msg.clear();
      msg.str("");
      char seqnostring[1024]="";
      sprintf(seqnostring," %x %x", src_seqno, packet_type); // Add: how much data received; will be used by the sender. If total received > bundle size: discard packet.
      msg << seqnostring;
      Ptr<Packet> ack = Create<Packet>((uint8_t*) msg.str().c_str(), msg.str().length());
      if(m_socket == 0){
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
        Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
        Ipv4Address ipaddr =(ipv4->GetAddress(1, 0)).GetLocal();
        InetSocketAddress local = InetSocketAddress(ipaddr, 50000);
        m_socket->Bind(local);    
      }
      ack->AddPacketTag(QosTag(5));
      InetSocketAddress ackaddr(address.GetIpv4(), 50000);
      m_socket->SendTo(ack, 0, ackaddr);
    }
    p->RemoveAllByteTags();
    p->RemoveAllPacketTags();

    i = 0;
    found = 0;
    while((i < bundles) &&(found == 0)){
      if((address.GetIpv4() == bundle_address[i].GetIpv4()) &&(src_seqno == bundle_seqno[i]) &&(packet_type == bundle_retx[i])){
        found = 1;
      } 
      else
      i++;
    }
    if(found == 0){
      i = 0;
      while((i < bundles) &&(found == 0)){
        if(currentServerRxBytes[i] == 0){
          found = 1;
          bundle_address[i] = address.GetIpv4();
          bundle_seqno[i] = src_seqno;
          bundle_retx[i] = packet_type;
          bundle_ts[i] = Simulator::Now().GetSeconds();
          currentServerRxBytes[i] = 0;
        } 
        else
          i++;
      }
    }
    if(found == 0){
      ++bundles;
      bundle_address=(InetSocketAddress*)realloc(bundle_address,bundles*sizeof(InetSocketAddress));
      bundle_address[i] = address.GetIpv4();
      bundle_seqno=(int32_t*)realloc(bundle_seqno,bundles*sizeof(int32_t));
      bundle_seqno[i] = src_seqno;
      bundle_retx=(int32_t*)realloc(bundle_retx,bundles*sizeof(int32_t));
      bundle_retx[i] = packet_type;
      currentServerRxBytes=(uint32_t*)realloc(currentServerRxBytes,bundles*sizeof(uint32_t));
      currentServerRxBytes[i] = 0;
      bundle_size=(uint32_t*)realloc(bundle_size,bundles*sizeof(uint32_t));
      bundle_ts=(double*)realloc(bundle_ts,bundles*sizeof(double));
      bundle_ts[i] = Simulator::Now().GetSeconds();
      newpkt.push_back(p->Copy());
    }
    if(p == 0 && socket->GetErrno() != Socket::ERROR_NOTERROR)
      NS_FATAL_ERROR("Server could not read stream at byte " << currentServerRxBytes[i]);
    if(currentServerRxBytes[i] == 0){
      currentServerRxBytes[i] += p->GetSize();
      newpkt[i] = p;
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      newpkt[i]->RemoveHeader(tHeader);
      if(tHeader.Get() == mypacket::MYTYPE_AP){
        mypacket::APHeader apHeader;
        newpkt[i]->RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize() + 26;
        newpkt[i]->AddHeader(apHeader);
      } 
      else{
        if(tHeader.Get() == mypacket::MYTYPE_BNDL){
          mypacket::BndlHeader bndlHeader;
          newpkt[i]->RemoveHeader(bndlHeader);
          bundle_size[i] = bndlHeader.GetBundleSize() + 29+12;
          newpkt[i]->AddHeader(bndlHeader);
        } 
        else{
          // Bundle fragments arrive in wrong order; no bundle header
          currentServerRxBytes[i] = 0;
          return;
        }
      }
      newpkt[i]->AddHeader(tHeader);
    } 
    else{
      if(currentServerRxBytes[i] > bundle_size[i]){
        std::cout<<"Current server bytes: "<<currentServerRxBytes[i]<<" Bundle size: "<<bundle_size[i]<<"\n";
        std::cout << "WTF at time " << Simulator::Now().GetSeconds() <<
          " received " << p->GetSize() <<
          " bytes at " << owniaddress.GetIpv4() <<
          " total bytes " << currentServerRxBytes[i] <<
          " from " << address.GetIpv4() <<
          " seqno " << src_seqno << "\n";
      } 
      else{
        currentServerRxBytes[i] += p->GetSize();
        newpkt[i]->AddAtEnd(p);
      }
    }
    if(currentServerRxBytes[i] == bundle_size[i]){
      currentServerRxBytes[i] = 0;
      Ptr<Packet> qpkt = newpkt[i]->Copy();
      Ptr<Packet> qpkt1 = newpkt[i]->Copy();

      // cout<<"Received:"<<s<<endl;

      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      newpkt[i]->RemoveHeader(tHeader);
      if(tHeader.Get() == mypacket::MYTYPE_AP){
        mypacket::APHeader apHeader;
        newpkt[i]->RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize();
        if((IsDuplicate(qpkt, m_antipacket_queue) == 0) &&((Simulator::Now().GetSeconds() - apHeader.GetSrcTimestamp().GetSeconds()) < 1000.0)){
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
          qpkt->RemoveHeader(tHeader);
          mypacket::APHeader apHeader;
          qpkt->RemoveHeader(apHeader);
          apHeader.SetHopTimestamp(Simulator::Now());
          apHeader.SetHopCount(apHeader.GetHopCount() + 1);
          qpkt->AddHeader(apHeader);
          qpkt->AddHeader(tHeader);
          // bool success = m_antipacket_queue->Enqueue(qpkt);
          // if(success){
          // }
          RemoveBundle(qpkt); 
        }
      } 
      else{
        mypacket::BndlHeader bndlHeader;
        newpkt[i]->RemoveHeader(bndlHeader);
        // std::cout<<"DATA COUNT "<<unsigned(bndlHeader.GetDataCount())<<"\n";
        bundle_size[i] = bndlHeader.GetBundleSize();
        if(IsDuplicate(qpkt, m_queue) == 1)
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " received duplicate " << newpkt[i]->GetSize() <<
            " bytes at " << owniaddress.GetIpv4() <<
            " from " << address.GetIpv4() <<
            " bundle hop count: "  <<(unsigned)bndlHeader.GetHopCount() <<
            " sequence number: "  << bndlHeader.GetOriginSeqno() <<
            " bundle queue occupancy: " << m_queue->GetNBytes() << "\n";


        if((IsDuplicate(qpkt, m_queue) == 0) &&(AntipacketExists(qpkt) == 0) &&((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < 1000.0)){
          if(bndlHeader.GetDst() == owniaddress.GetIpv4()){
            float time = Simulator::Now().GetSeconds();
            float delay = Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds() + 1000.0*(bndlHeader.GetNretx());
            std::cout << "At time " << time<<
              " received " << newpkt[i]->GetSize() <<
              " bytes at " << owniaddress.GetIpv4() <<
              "(final dst) from " << address.GetIpv4() <<
              " delay: "  << delay <<
              " bundle hop count: "  <<(unsigned)bndlHeader.GetHopCount() + 1 <<
              " sequence number: "  << bndlHeader.GetOriginSeqno() <<
              " bundle queue occupancy: " << m_queue->GetNBytes() << "\n";
              uint8_t *buffer1 = new uint8_t[newpkt[i]->GetSize()+1];
              newpkt[i]->CopyData(buffer1, newpkt[i]->GetSize());
              buffer1[newpkt[i]->GetSize()]='\0';
              
              std::string s = std::string(buffer1, buffer1+newpkt[i]->GetSize());

              // std::cout<<"string is :"<<s<<"\n";
              // std::ofstream datapoints;
              // datapoints.open("datapoints.txt",std::ios_base::app);
              // if(datapoints.is_open()){
              //   std::cout<<"Hello\n";
              //   datapoints<<s;
              //   datapoints<<"HI THERE FAM\n";
              // }
              // datapoints.close();
            

              // std::string delimiter = ",";
              // std::string::size_type sz;
              // size_t pos = 0;
              // std::string token;
              // while((pos = s.find(delimiter)) != std::string::npos){
              //     token = s.substr(0, pos);
              //     std::cout << token.substr(0,3) <<","<< time<<","<<delay<<std::endl;
              //     s.erase(0, pos + delimiter.length());
              // }


              // SendAP(bndlHeader.GetDst(), bndlHeader.GetOrigin(), bndlHeader.GetOriginSeqno(), bndlHeader.GetSrcTimestamp());
              

          } 
          else{
            mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
            qpkt->RemoveHeader(tHeader);
            mypacket::BndlHeader bndlHeader;
            qpkt->RemoveHeader(bndlHeader);
            bndlHeader.SetHopTimestamp(Simulator::Now());
            bndlHeader.SetHopCount(bndlHeader.GetHopCount() + 1);
            qpkt->AddHeader(bndlHeader);
            qpkt->AddHeader(tHeader);
            if((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + qpkt->GetSize()) <= b_s){
              std::stringstream forcheck;
              address.GetIpv4().Print(forcheck);
              forcheck.str();
              std::ostringstream ave;
              std::ostringstream largest;
              std::ostringstream smallest;
              std::ostringstream sensorid;
              ave << bndlHeader.GetDataAverage();
              largest << bndlHeader.GetLargestVal();
              smallest << bndlHeader.GetSmallestVal();
              sensorid << (int)bndlHeader.GetSensorID();
              std::string ichcheck[11];
              ichcheck[0]=forcheck.str();
              ichcheck[1]=sensorid.str();
              ichcheck[2]=ave.str(); //ave
              ichcheck[3]=ave.str(); //ave
              ichcheck[4]=ave.str(); //ave
              ichcheck[5]=largest.str(); //smallest
              ichcheck[6]=largest.str(); //smallest
              ichcheck[7]=largest.str(); //smallest
              ichcheck[8]=smallest.str(); //largest
              ichcheck[9]=smallest.str(); //largest
              ichcheck[10]=smallest.str(); //largest
              std::cout<<"At time "<<Simulator::Now().GetSeconds()<<" mobile node "<<m_node->GetId()<<" received bundle of sequence no. "<<bndlHeader.GetOriginSeqno()<<".\n";
              std::cout<<"Checking with Flowtable...\n";
              int result=CheckMatch(ichcheck);
              std::cout << "Returned Action: " << result << "\n";
              bool success;
              if(result ==0){  //drop
                std::cout << "ACTION: DROP\n";
                std::cout << "Bundle dropped.\n";
                std::cout<<m_queue->GetNPackets()<<"\n";
              }
              else if(result == 1){

                std::cout << "ACTION: DIRECT TO BASE\n";
                std::cout << "Enqueueing to dtb queue. m_dtb_queue size: " << m_dtb_queue->GetNPackets() << "\n";
                m_dtb_queue->Enqueue(qpkt);
   
                std::cout<<"Number of packets in dtb_queue:  "<<m_dtb_queue->GetNPackets()<<" \n";
              }
              else if (result == 2){//packet in
                std::cout << "ACTION: PACKET IN\n";
                qpkt->RemoveHeader(tHeader);
                qpkt->RemoveHeader(bndlHeader);
                qpkt->AddHeader (bndlHeader);
                qpkt->AddHeader (tHeader);
                std::cout<<bndlHeader.GetDataAverage()<<" is the data average\n";
                success = m_packetin_queue->Enqueue (qpkt);
                if (success){
                  std::cout<<"Successfully enqueued packet to packet in queue.\n";
                }                

              }
              else if(result==255){
                std::cout<<"ACTION: SPREAD\n";

                // HERE IS TO COMMENT OUT IF BACK TO 1000 SI MOBILE
                // if (m_queue->GetNPackets()==10){
                //   Ptr<Packet> pkt = m_queue->Dequeue();
                //   mypacket::TypeHeader tHeader2(mypacket::MYTYPE_BNDL);
                //   mypacket::BndlHeader bndlHeader2;
                //   pkt->RemoveHeader(tHeader2);
                //   pkt->RemoveHeader(bndlHeader2);
                //   std::cout<<"Dropped bundle of sequence number: "<<bndlHeader2.GetOriginSeqno()<<" because queue is full.\n";

                // }
                // // END HERE
                m_queue->Enqueue(qpkt);
                std::cout<<"Bundle successfully enqueued to m_queue.\n";                

              }

            } 
            else{
              drops++;
              std::cout << "At time " << Simulator::Now().GetSeconds() <<
              " dropped " << newpkt[i]->GetSize() <<
              " bytes at " << owniaddress.GetIpv4() <<
              " from " << address.GetIpv4() <<
              " bundle hop count: "  <<(unsigned)bndlHeader.GetHopCount() <<
              " sequence number: "  << bndlHeader.GetOriginSeqno() <<
              " bundle queue occupancy: " << m_queue->GetNBytes() << "\n";
            }
          }
        }
      }
    }
  }
}

int Mobile::CheckMatch(std::string ichcheck[]){
  //RULES
  //0 ip address ==
  //1 sensor id ==
  //2 data Ave >
  //3 data Ave ==
  //4 data Ave <
  //5 smallest data >
  //6 smallest data ==
  //7 smallest data <
  //8 largest data >
  //9 largest data ==
  //10 largest data <
  // flowTable.listPrinter();
  int matchFlag;
  int ftmEntry;
  int toCheck;
  int irereturn;

  for(int x=0; x<flowTable.getSize(); x++){
    matchFlag=1;
    std::string* flowTableMatchEntry=flowTable.get(x);
    // std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~INDEX NG FLOW TABLE MATCH: " << x << " \n";
    
    for(int y=0; y<11; y++){
      // "-" means null; I tried na NULL gamitin, magulo
      // std::cout <<"Y: "<< y << "\n";
      if(flowTableMatchEntry[y]!="*"){
        //for conditions with int comparables
        if(y>0){
          // std::cout << y << " \n";
          std::stringstream ftmEntryStream(flowTableMatchEntry[y]);
          ftmEntryStream >> ftmEntry;

          std::stringstream ichcheckStream(ichcheck[y]);
          ichcheckStream >> toCheck;

          // std::cout<< "At flow index "<<x<<"\n";
          // std::cout << "ENTRY: " << ftmEntry << "\n";
          // std::cout << "ICHCHECK: " << toCheck << "\n";
        }
        if(y==0){
          if(ichcheck[y]==flowTableMatchEntry[y]){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==1){
          if(ftmEntry==toCheck){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==2){
          if(toCheck>ftmEntry){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==3){
          if(ftmEntry==toCheck){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==4){
          if(toCheck<ftmEntry){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==5){
          if(toCheck>ftmEntry){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==6){
          if(ftmEntry==toCheck){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==7){
          if(toCheck<ftmEntry){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==8){
          if(toCheck>ftmEntry){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==9){
          if(ftmEntry==toCheck){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        else if(y==10){
          if(toCheck<ftmEntry){
            matchFlag=matchFlag*1;
          }
          else{
            matchFlag=0;
          }
        }
        // if(matchFlag==1){
        //  std::cout <<"1 "<< "\n";

        // }
        if(matchFlag==1){
          // std::cout <<"0 "<< "\n";

          break;
        }
      }
      // else{
      //  std::cout <<"----\n";
      // }
    }


    if(matchFlag==1){
      std::string* matchEntry=flowTable.get(x);
      std::cout << "MATCH; Flow index: " << x << " ACTION: " << matchEntry[11] << "\n";
      std::stringstream matchEntryStream(matchEntry[11]);
      matchEntryStream >> irereturn;
      return irereturn;
      // break;
    }
  }
  if(matchFlag==0){
    std::cout << "NO MATCH OR WILDCARD; Flow index: " << flowTable.getSize() << "\n";
    return 255;
  }
  return 000;
}

void Mobile::CheckPacketInQueues(){
  // std::cout<< "PUMASOK CheckPacketInQueues\n";
  Ptr<Packet> packet;
  uint32_t pkts=0, n=0;
  
  mypacket::APHeader apHeader;
  mypacket::BndlHeader bndlHeader;

  //iterate over contents of m_packetin_queue
  pkts=m_packetin_queue->GetNPackets();
  n=0;
  // std::cout <<"CURRENT SIZE OF QUEUE" << m_packetin_queue->GetNPackets() << "\n";
  while(n<pkts){
    std::cout <<"\n"<< n << " CheckPacketInQueues\n";
    n++;
    // std::cout<<"BEFORE packetIn " <<m_packetin_queue->GetNPackets()<<"\n";
    packet = m_packetin_queue->Dequeue();
    Ptr<Packet> cpkt = packet->Copy();
    //???
    mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
    packet->RemoveHeader(tHeader);
    packet->RemoveHeader(apHeader);
    Simulator::Schedule(Seconds(3.0), &DtnExample::PacketIn, dtnExample, 1,1,cpkt,m_node->GetId());
    // dtnExample->PacketIn(1,1,cpkt,m_node->GetId());
    // std::cout<<"AFTER packetIn " <<m_packetin_queue->GetNPackets()<<"\n\n";
  }
  //recalling so it always checks m_packetin_queue
  //if queue still contains bundles; packetIn na
  if(m_packetin_queue->GetNPackets()!=0){
    Simulator::Schedule(Seconds(0.001), &Mobile::CheckPacketInQueues, this);
  }
  //if queue is empty; not that urgent to packetIn
  else{
    Simulator::Schedule(Seconds(0.01), &Mobile::CheckPacketInQueues, this);
  }
}

void Mobile::TriggerInsertFlow(){
  std::cout << "\nAt time " << Simulator::Now().GetSeconds() << " INSERTING FLOW {\"10.0.0.99\", \"99\", \"99\", \"99\", \"99\", \"99\", \"99\", \"99\", \"99\", \"99\", \"99\", \"255\"}\n";
  std::string tempArr[]={"10.0.0.99", "*", "*", "*", "*", "*", "*", "*", "*", "*", "*", "255"};
  flowTable.insertWithPriority(25, tempArr);
  // flowTable.listPrinter();
  flowTable.listPrinter();
  std::cout<<"\n";
}

void Mobile::ScheduleTx(){
  // m_sendEvent = Simulator::Schedule (tNext, &DtnApp::SendBundle, this, dstnode, packetsize);
  m_sendEvent = Simulator::Schedule(Seconds(1.0), &Mobile::TriggerInsertFlow, this);
  // m_sendEvent = Simulator::Schedule (tNext, &DtnApp::SendBundle, this, dstnode, packetsize);
}


void Mobile::CheckQueues(uint32_t bundletype){
  Ptr<Packet> packet;
  Ptr<Packet> firstpacket;

  uint32_t i = 0, n = 0, pkts = 0, send_bundle = 0;
  mypacket::APHeader apHeader;
  mypacket::BndlHeader bndlHeader;

  // Remove expired antipackets and bundles -- do this check only once
  if(bundletype == 2){


    pkts = m_queue->GetNPackets();
    n = 0;

    //iterating through packets in m_queue
    while(n < pkts){
      n++;
      packet = m_queue->Dequeue();
      Ptr<Packet> cpkt = packet->Copy();

      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      packet->RemoveHeader(tHeader);
      packet->RemoveHeader(bndlHeader);
      //if less 750, keep


      if(((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < 1000.0) ||(bndlHeader.GetHopCount() == 0)){
        packet->AddHeader(bndlHeader);
        packet->AddHeader(tHeader);
        bool success = m_queue->Enqueue(packet);
        if(success){
        }
      } 
      else{
        uint32_t d=0;
        while(d < neighbors){
          uint32_t m=0, sent_found=0;
          //rearranging neighbor_sent_bundles, if nasend na ang bundle sa final destination
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_sent_bundles[d][m] ==(int32_t)bndlHeader.GetOriginSeqno()){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_bundles[d][m] != 0) &&(m < 999)){
                //deleting theat sequence number from neighbor_sent_bundles
                neighbor_sent_bundles[d][m]=neighbor_sent_bundles[d][m+1];
                m++;
              }
              neighbor_sent_bundles[d][999]=0;
            }
          }
          d++;
        }
      }
    
    } 
  }
  
  ////////////////////////////////// end of removing expired bundles and antipackets



  //ano meron sa mac_queue
  if(mac_queue->GetSize() < 2){
    if(bundletype == 2){
      pkts = m_dtb_queue->GetNPackets();
      n = 0;
      while (n < pkts){
        n++;
        // std::cout<<"hi"<<n;
        packet = m_dtb_queue->Dequeue();
        mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
        packet->RemoveHeader(tHeader);
        packet->RemoveHeader(bndlHeader);
        packet->AddHeader(bndlHeader);
        packet->AddHeader(tHeader);
        if((Simulator::Now().GetSeconds() - bndlHeader.GetHopTimestamp().GetSeconds()) > 0.2){

          Ipv4Address dst = bndlHeader.GetDst();
          i = 0;     
          while ((i < neighbors) && (send_bundle ==0)){
            if ((dst == neighbor_address[i].GetIpv4()) && ((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1)){
              send_bundle=1;
              // std::cout<< "HELLO\n";
              // std::cout<<neighbor_address[i].GetIpv4()<<" "<<i<<"\n";
              break;
            }
            i++;
          }
        }
        Ptr<Packet> qp = packet->Copy();        

        if (send_bundle==0){
          m_dtb_queue->Enqueue(qp);
        }
        else{
          break;
        }
      }
      // std::cout<<"\n";
      
    }

     
    else{ //if not bundle type ==2
      pkts = m_queue->GetNPackets();
      n = 0;
      while(n < pkts){
        n++;
        packet = m_queue->Dequeue();
        mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
        packet->RemoveHeader(tHeader);
        packet->RemoveHeader(bndlHeader);
       
        //if not old ang bundle
        if((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) < 1000.0){
          // ano lang basta di pa bago ang bundle narating sa node na itu
          if(n==1){
            if((Simulator::Now().GetSeconds() - bndlHeader.GetHopTimestamp().GetSeconds()) > 0.2){

              Ipv4Address dst = bndlHeader.GetDst();
              uint8_t spray = bndlHeader.GetSpray();
              i = 0;
              //iterating through neighbors
              while((i < neighbors) &&(send_bundle == 0)){
                // if dest of bundle is neighboraddress, tas bago lang nakita ang neighbor
                if((((bundletype == 0) &&(spray > 0) &&((cc == 0)||(b_a[i] > packet->GetSize()))) ||(dst == neighbor_address[i].GetIpv4())) && \
                 ((Simulator::Now().GetSeconds() - neighbor_last_seen[i]) < 0.1) &&(neighbor_address[i].GetIpv4() != bndlHeader.GetOrigin())){
                  int neighbor_has_bundle = 0, bundle_sent = 0, j=0;
                  //check kung meron ba siya nung bundle
                  while((neighbor_has_bundle == 0) &&(neighbor_hello_bundles[i][j] != 0) &&(j < 1000)){
                    if((unsigned)neighbor_hello_bundles[i][j] == bndlHeader.GetOriginSeqno())
                      neighbor_has_bundle = 1;
                    else
                      j++;
                  }
                  j = 0;
                  //check if nasend na ba yung bundle sa kanya
                  while((neighbor_has_bundle == 0) &&(bundle_sent == 0) &&(neighbor_sent_bundles[i][j] != 0) &&(j < 1000)){
                    if(neighbor_sent_bundles[i][j] ==(int32_t)bndlHeader.GetOriginSeqno())
                      bundle_sent = 1;
                    else
                      j++;
                  }
                  if((neighbor_has_bundle == 0) &&(bundle_sent == 0)){
                    if(bundletype == 0){
                      if(rp == 1)
                        bndlHeader.SetSpray(spray/2);
                      if(cc > 0){
                        if(packet->GetSize() >= b_a[i])
                          b_a[i] = 0;
                        else
                          b_a[i] -= packet->GetSize();
                      }
                    } 
                    else{
                      // Wait 5.0 seconds before forwarding to other(than dst) nodes
                      bndlHeader.SetHopTimestamp(Simulator::Now() + Seconds(5.0));
                    }

                    send_bundle = 1;
                    j = 0;
                    while((neighbor_sent_bundles[i][j] != 0) &&(j < 999))
                      j++;
                    neighbor_sent_bundles[i][j] =(int32_t)bndlHeader.GetOriginSeqno();
                  } 
                  else
                    i++;
                } 
                else
                  i++;
              }
            }
          }
          packet->AddHeader(bndlHeader);
          packet->AddHeader(tHeader);
          Ptr<Packet> qp = packet->Copy();
          if(n==1){
            firstpacket= packet->Copy();
            if(send_bundle!=1){
              m_queue->Enqueue(qp);
            }
          }
          else{
            bool success = m_queue->Enqueue(qp);
            if(success){
            }
          }
        } 
        else{
          //erase bundle kung di ikaw source tas old na 
          if(((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) > 1000.0) &&(bndlHeader.GetHopCount() == 0) &&(bndlHeader.GetNretx() < 3)){
            bndlHeader.SetSrcTimestamp(Simulator::Now());
            bndlHeader.SetNretx(bndlHeader.GetNretx() + 1);
            uint32_t n=0;
            while(n < neighbors){
              uint32_t m=0, sent_found=0;
              while((m < 1000) &&(sent_found == 0)){
                if(neighbor_sent_bundles[n][m] ==(int32_t)bndlHeader.GetOriginSeqno()){
                  sent_found=1;
                } 
                else
                  m++;
                if(sent_found == 1){
                  while((neighbor_sent_bundles[n][m] != 0) &&(m < 999)){
                    neighbor_sent_bundles[n][m]=neighbor_sent_bundles[n][m+1];
                    m++;
                  }
                  neighbor_sent_bundles[n][999]=0;
                }
              }
              n++;
            }
          }
          if((bndlHeader.GetHopCount() == 0) &&((Simulator::Now().GetSeconds() - bndlHeader.GetSrcTimestamp().GetSeconds()) <= 1000.0)){
            packet->AddHeader(bndlHeader);
            packet->AddHeader(tHeader);
            bool success = m_queue->Enqueue(packet);
            if(success){
            }
          }
        }
      }
      packet = firstpacket;


    }
  } 
  else{
    bundletype = 0;
  }
  if(send_bundle == 1){
    // std::cout<<"SENDBUNDLE1\n";
    if(m_socket == 0){
      m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
      Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
      Ipv4Address ipaddr =(ipv4->GetAddress(1, 0)).GetLocal();
      InetSocketAddress local = InetSocketAddress(ipaddr, 50000);
      m_socket->Bind(local);    
    }    

    InetSocketAddress dstremoteaddr(neighbor_address[i].GetIpv4(), 50000);
    // std::cout<<"retransmitting to i "<<i<<" "<<neighbor_address[i].GetIpv4()<<"\n";    
    if(bundletype < 2){
      packet = firstpacket->Copy();
      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      Ptr<Packet> anotherp = packet->Copy();

      anotherp->RemoveHeader(tHeader);
      anotherp->RemoveHeader(bndlHeader);
      anotherp->AddHeader(bndlHeader);
      anotherp->AddHeader(tHeader);
      NumFlows++;
      sendTos=(InetSocketAddress*)realloc(sendTos,NumFlows*sizeof(InetSocketAddress));
      sendTos[NumFlows-1] = dstremoteaddr.GetIpv4();
      ids[NumFlows-1] = bndlHeader.GetOriginSeqno();
      retxs[NumFlows-1] = bndlHeader.GetNretx();
      currentTxBytes[NumFlows-1] = std::min((uint32_t)1472, packet->GetSize());
      lastTxBytes[NumFlows-1] = std::min((uint32_t)1472, packet->GetSize());
      firstSendTime[NumFlows-1] = Simulator::Now().GetSeconds();
      lastSendTime[NumFlows-1] = Simulator::Now().GetSeconds();
      totalTxBytes[NumFlows-1] = packet->GetSize();
      if(packet->GetSize() > 1472)
        packet->RemoveAtEnd(packet->GetSize() - 1472);
      packet->AddPacketTag(FlowIdTag(bndlHeader.GetOriginSeqno()));
      packet->AddPacketTag(QosTag(bndlHeader.GetNretx()));
      retxpkt.push_back(packet->Copy());
      Simulator::Schedule(Seconds(1.0), &DtnApp::Retransmit, this, sendTos[NumFlows-1], ids[NumFlows-1], retxs[NumFlows-1]);
    } 
    else{  

      mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
      Ptr<Packet> anotherp = packet->Copy();

      anotherp->RemoveHeader(tHeader);
      anotherp->RemoveHeader(bndlHeader);
      anotherp->AddHeader(bndlHeader);
      anotherp->AddHeader(tHeader);
      // std::cout<<"Retransmitting from 10.0.0."<<m_node->GetId()+1<<" to "<<neighbor_address[i].GetIpv4()<<" sequence "<<bndlHeader.GetOriginSeqno()<<" size: "<<packet->GetSize()<< "bytes\n";

      NumFlows++;
      sendTos=(InetSocketAddress*)realloc(sendTos,NumFlows*sizeof(InetSocketAddress));
      sendTos[NumFlows-1] = dstremoteaddr.GetIpv4();
      ids[NumFlows-1] = bndlHeader.GetOriginSeqno();
      retxs[NumFlows-1] = bndlHeader.GetNretx();
      currentTxBytes[NumFlows-1] = std::min((uint32_t)1472, packet->GetSize());
      lastTxBytes[NumFlows-1] = std::min((uint32_t)1472, packet->GetSize());
      firstSendTime[NumFlows-1] = Simulator::Now().GetSeconds();
      lastSendTime[NumFlows-1] = Simulator::Now().GetSeconds();
      totalTxBytes[NumFlows-1] = packet->GetSize();
      if(packet->GetSize() > 1472)
        packet->RemoveAtEnd(packet->GetSize() - 1472);
      packet->AddPacketTag(FlowIdTag(bndlHeader.GetOriginSeqno()));
      packet->AddPacketTag(QosTag(bndlHeader.GetNretx()));
      retxpkt.push_back(packet->Copy());
      Simulator::Schedule(Seconds(1.0), &DtnApp::Retransmit, this, sendTos[NumFlows-1], ids[NumFlows-1], retxs[NumFlows-1]);  
    //   packet->AddPacketTag(FlowIdTag(-apHeader.GetOriginSeqno()));
    //   packet->AddPacketTag(QosTag(4));
    }
    
    m_socket->SendTo(packet, 0, dstremoteaddr);
    Address ownaddress;
    m_socket->GetSockName(ownaddress);
    //InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom(ownaddress);    
  }
  if(bundletype == 2){
    if(send_bundle == 0)
      CheckQueues(1);
    else //pag may sinend 
      Simulator::Schedule(Seconds(2.0), &Mobile::CheckQueues, this, 2);
  }
  if(bundletype == 1){
    if(send_bundle == 0)
      CheckQueues(0);
    else
      Simulator::Schedule(Seconds(2.0), &Mobile::CheckQueues, this, 2);
  }
  if(bundletype == 0){
    if(send_bundle == 0)
      Simulator::Schedule(Seconds(0.01), &Mobile::CheckQueues, this, 2);
    else
      Simulator::Schedule(Seconds(2.0), &Mobile::CheckQueues, this, 2);
  }
}


void Mobile::HandleReply(json_object *jsonreply){


  std::cout<<"Received json from zmq:\n";
  printf("%s\n", json_object_to_json_string(jsonreply));

  struct json_object *install;
  struct json_object *toDelete;

  install = json_object_object_get(jsonreply, "install");
  toDelete = json_object_object_get(jsonreply, "delete");

  // std::cout<<json_object_get_string(json_object_array_get_idx(install,0))<<"\n";
  // std::cout<<json_object_get_string(json_object_array_get_idx(toDelete,0))<<"\n";

  std::cout<<json_object_array_length(install)<<" is the number of flows to be installed. \n";
  std::cout<<json_object_array_length(toDelete)<<" is the number of flows to be deleted. \n";

  int numToInstall = json_object_array_length(install);
  // int numToDelete = json_object_array_length(toDelete);

  //RULES
  //0 ip address ==
  //1 sensor id ==
  //2 data Ave >
  //3 data Ave ==
  //4 data Ave <
  //5 smallest data >
  //6 smallest data ==
  //7 smallest data <
  //8 largest data >
  //9 largest data ==
  //10 largest data <

  std::string tempArr[12];
  // INSTALLING FLOWS
  for (int i=0; i<numToInstall; i++){
    struct json_object *iterate;
    iterate = json_object_array_get_idx(install,i);
    tempArr[0]= json_object_get_string(json_object_object_get(iterate,"ip_address"));
    tempArr[1]=   json_object_get_string(json_object_object_get(iterate,"sensor_id"));
    tempArr[2]=    json_object_get_string(json_object_object_get(iterate,"gt_data_ave"));
    tempArr[3]=    json_object_get_string(json_object_object_get(iterate,"eq_data_ave"));
    tempArr[4]=    json_object_get_string(json_object_object_get(iterate,"lt_data_ave"));
    tempArr[5]=    json_object_get_string(json_object_object_get(iterate,"gt_smallest_val"));
    tempArr[6]=    json_object_get_string(json_object_object_get(iterate,"eq_smallest_val"));
    tempArr[7]=    json_object_get_string(json_object_object_get(iterate,"lt_smallest_val"));
    tempArr[8]=    json_object_get_string(json_object_object_get(iterate,"gt_largest_val"));
    tempArr[9]=    json_object_get_string(json_object_object_get(iterate,"eq_largest_val"));
    tempArr[10]=    json_object_get_string(json_object_object_get(iterate,"lt_largest_val"));
    tempArr[11]=    json_object_get_string(json_object_object_get(iterate,"action"));
    

    flowTable.insertWithPriority(json_object_get_int(json_object_object_get(iterate,"priority")), tempArr);
    flowTable.listPrinter();

  }

  // DELETING FLOWS (IN PROGRESS)  

}


void Mobile::Alive(int first){
  //boot
  if (first == 0){
    std::cout<<"BOOT\n";

    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    socket.connect("tcp://localhost:5555");
    int recvcount = 1;
    while(recvcount<2){

      struct json_object *object, *tmp;

      object = json_object_new_object();


     //EVENT TYPE
      tmp = json_object_new_int(2);
      json_object_object_add(object, "event_type", tmp);


      //MOBILE ID
      tmp = json_object_new_int(m_node->GetId());
      json_object_object_add(object, "mobile_id", tmp);


      //MOBILE IP ADDRESS
      char mobileIpAddress[1024]="";
      sprintf(mobileIpAddress,"10.0.0.%d",(m_node->GetId() + 1));
      tmp = json_object_new_string(mobileIpAddress);
      json_object_object_add(object, "ip_address", tmp);


      //writing 
      std::cout<<"Sending JSON to zmq: \n";
      printf("%s\n", json_object_to_json_string(object));
      // printf("size: %u \n", (unsigned)strlen(json_object_to_json_string(object)));


      zmq::message_t request(strlen(json_object_to_json_string(object)));

      memcpy(request.data(), json_object_to_json_string(object), strlen(json_object_to_json_string(object)));

      
      socket.send(request);



      //REPLY HANDLING

      zmq::message_t reply;
      socket.recv(&reply);

      json_object *jstring = json_tokener_parse(static_cast<char*>(reply.data()));
      HandleReply(jstring);

      recvcount++;

    }



    Simulator::Schedule(Seconds(300.0),&Mobile::Alive, this, 2);
  }
  //pagtawag ng boot
  else if (first ==1){
    std::cout<<"SCHEDULING BOOT 1 MIN FROM NOW.\n";
    Simulator::Schedule(Seconds(1.0),&Mobile::Alive, this, 0);
  }
  // alive every five minutes
  else{
    std::cout<<"ALIVE\n";

    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    socket.connect("tcp://localhost:5555");
    int recvcount = 1;
    while(recvcount<2){

      struct json_object *object, *tmp;

      object = json_object_new_object();


     //EVENT TYPE
      tmp = json_object_new_int(1);
      json_object_object_add(object, "event_type", tmp);


      //MOBILE ID
      tmp = json_object_new_int(m_node->GetId());
      json_object_object_add(object, "mobile_id", tmp);


      //MOBILE IP ADDRESS
      char mobileIpAddress[1024]="";
      sprintf(mobileIpAddress,"10.0.0.%d",(m_node->GetId() + 1));
      tmp = json_object_new_string(mobileIpAddress);
      json_object_object_add(object, "ip_address", tmp);


      //writing 
      std::cout<<"Sending JSON to zmq: \n";
      printf("%s\n", json_object_to_json_string(object));
      // printf("size: %u \n", (unsigned)strlen(json_object_to_json_string(object)));


      zmq::message_t request(strlen(json_object_to_json_string(object)));

      memcpy(request.data(), json_object_to_json_string(object), strlen(json_object_to_json_string(object)));

      
      socket.send(request);



      //REPLY HANDLING

      zmq::message_t reply;
      socket.recv(&reply);

      json_object *jstring = json_tokener_parse(static_cast<char*>(reply.data()));
      HandleReply(jstring);
 
      recvcount++;
    }


    Simulator::Schedule(Seconds(300.0), &Mobile::Alive, this, 2);
  }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////BASE FUNCTION DEFS//////////////////////////
//////////////////////////////////////////////////////////////////////////
void Base::BaseSetup(Ptr<Node> node, DtnExample *dtnEx){
  dtnExample = dtnEx;
  m_node = node;
  m_antipacket_queue = CreateObject<DropTailQueue>();
  m_queue = CreateObject<DropTailQueue>();
  m_helper_queue = CreateObject<DropTailQueue>();
  m_base_queue = CreateObject<DropTailQueue>();
  m_antipacket_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_helper_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  m_base_queue->SetAttribute("MaxPackets", UintegerValue(1000));
  stationary = 1;
  for(int i = 0; i < 10000; i++){
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
  b_s = 1375000 + y->GetInteger(0, 1)*9625000;
}

void Base::StartApplication(void){
  std::cout<<"BASE START APP\n";
  m_running = true;
  Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice>(m_node->GetDevice(0));
  NS_ASSERT(dev != NULL);
  PointerValue ptr;
  dev->GetAttribute("Mac",ptr);
  Ptr<AdhocWifiMac> mac = ptr.Get<AdhocWifiMac>();
  NS_ASSERT(mac != NULL);
  Ptr<EdcaTxopN> edcaqueue = mac->GetBEQueue();
  NS_ASSERT(edcaqueue != NULL);
  mac_queue = edcaqueue->GetEdcaQueue();    
  NS_ASSERT(mac_queue != NULL);
  mac_queue->SetAttribute("MaxPacketNumber", UintegerValue(1000));
  CheckQueues(2);
  PrintBuffers();
}

void Base::SendHello(Ptr<Socket> socket, double endTime, Time pktInterval, uint32_t first){
  if(first == 0){
    double now(Simulator::Now().GetSeconds());
    if(now < endTime){
      std::stringstream msg;
      msg.clear();
      msg.str("");
      char seqnostring[1024]="";
      if(cc == 2){
        if((drops == 0) &&(t_c < 0.9)){
          t_c += 0.01;
        } 
        else{
          if((drops > 0) &&(t_c > 0.5))
            t_c = t_c * 0.8;
          drops = 0;
        }
      }
      if((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes()) >=(uint32_t)(t_c * b_s))
        sprintf(seqnostring,"%d",0);
      else
        sprintf(seqnostring,"%d",((uint32_t)(t_c * b_s) - m_queue->GetNBytes() - m_antipacket_queue->GetNBytes()));
      msg << seqnostring;
      uint32_t pkts = m_queue->GetNPackets();   
      // Reorder packets: put the least forwarded first
      uint32_t n = 0;
      Ptr<Packet> packet;
      while(n < pkts){
        n++;
        packet = m_queue->Dequeue();
        bool success = m_helper_queue->Enqueue(packet);
        if(success){
        }
      }
      uint32_t m = 0;
      while(m < pkts){
        m++;
        uint32_t min_count = 10000, min_seqno = 0, helper_pkts = m_helper_queue->GetNPackets();
        n = 0;
        while(n < helper_pkts){
          n++;
          packet = m_helper_queue->Dequeue();
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet->RemoveHeader(tHeader);
          packet->RemoveHeader(bndlHeader);
          int index = 0;
          uint32_t count = 0;
          while(index < NumFlows){
            if(ids[index] ==(int32_t)bndlHeader.GetOriginSeqno())
              count++;
            index++;
          }
          if(count < min_count){
            min_count = count;
            min_seqno = bndlHeader.GetOriginSeqno();
          }
          packet->AddHeader(bndlHeader);
          packet->AddHeader(tHeader);
          bool success = m_helper_queue->Enqueue(packet);
          if(success){
          }
        }
        int min_found = 0;
        while(min_found == 0){
          packet = m_helper_queue->Dequeue();
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet->RemoveHeader(tHeader);
          packet->RemoveHeader(bndlHeader);
          if(bndlHeader.GetOriginSeqno() == min_seqno){
            min_found = 1;
            packet->AddHeader(bndlHeader);
            packet->AddHeader(tHeader);
            bool success = m_queue->Enqueue(packet);
            if(success){
            }
          } 
          else{
            packet->AddHeader(bndlHeader);
            packet->AddHeader(tHeader);
            bool success = m_helper_queue->Enqueue(packet);
            if(success){
            }
          }
        }
      }
      // End of reorder  
      char seqnostring_b[1024]="";
      sprintf(seqnostring_b," %d",pkts);
      msg << seqnostring_b;
      for(uint32_t i = 0; i < pkts; ++i){
        Ptr<Packet> p = m_queue->Dequeue();
        if(msg.str().length() < 2280){
        // The default value of MAC-level MTU is 2296
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          p->RemoveHeader(tHeader);
          p->RemoveHeader(bndlHeader);
          uint32_t src_seqno = bndlHeader.GetOriginSeqno();
          char seqnostring_a[1024]="";
          sprintf(seqnostring_a," %x",(src_seqno));
          msg << seqnostring_a;
          p->AddHeader(bndlHeader);
          p->AddHeader(tHeader);
        } 
        else{
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " too big Hello(B)(" << msg.str().length() << ") bytes.\n";
        }
        bool success = m_queue->Enqueue(p);
        if(success){
        }  
      }
      uint32_t apkts = m_antipacket_queue->GetNPackets();
      for(uint32_t i = 0; i < apkts; ++i){
        Ptr<Packet> p = m_antipacket_queue->Dequeue();
        if(msg.str().length() < 2280){
          mypacket::TypeHeader tHeader(mypacket::MYTYPE_AP);
          mypacket::APHeader apHeader;
          p->RemoveHeader(tHeader);
          p->RemoveHeader(apHeader);
          uint32_t src_seqno = apHeader.GetOriginSeqno();
          char seqnostring_a[1024]="";
          sprintf(seqnostring_a," %x",(src_seqno));
          msg << seqnostring_a;
          p->AddHeader(apHeader);
          p->AddHeader(tHeader);
        } 
        else{
          std::cout << "At time " << Simulator::Now().GetSeconds() <<
            " too big Hello(AP)(" << msg.str().length() << ") bytes.\n";                   
        }
        bool success = m_antipacket_queue->Enqueue(p);
        if(success){
        }
      }
      Ptr<Packet> pkt = Create<Packet>((uint8_t*) msg.str().c_str(), msg.str().length());
      pkt->AddPacketTag(QosTag(6)); // High priority 
      socket->Send(pkt);
      Simulator::Schedule(Seconds(0.1), &Base::SendHello, this, socket, endTime, Seconds(0.1), 0);
    } 
    else
      socket->Close();
  } 
  else
    Simulator::Schedule(pktInterval, &Base::SendHello, this, socket, endTime, pktInterval, 0);
}

void Base::ReceiveHello(Ptr<Socket> socket){
  Ptr<Packet> packet;
  Address from;
  while(packet = socket->RecvFrom(from)){
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    uint32_t i = 0;
    uint32_t found = 0;
    while((i < neighbors) &&(found == 0)){
      if(address.GetIpv4() == neighbor_address[i].GetIpv4()){
        found = 1;
      } 
      else
        i++;
    }
    if(found == 0){
      ++neighbors;
      neighbor_address=(InetSocketAddress*)realloc(neighbor_address,neighbors*sizeof(InetSocketAddress));
      neighbor_address[i]=address.GetIpv4();
      neighbor_last_seen=(double*)realloc(neighbor_last_seen,neighbors*sizeof(double));
      b_a=(uint32_t*)realloc(b_a,neighbors*sizeof(uint32_t));
      neighbor_hello_bundles=(int32_t**)realloc(neighbor_hello_bundles,neighbors*sizeof(int32_t*));
      neighbor_hello_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_bundles=(int32_t**)realloc(neighbor_sent_bundles,neighbors*sizeof(int32_t*));
      neighbor_sent_bundles[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_aps=(int32_t**)realloc(neighbor_sent_aps,neighbors*sizeof(int32_t*));
      neighbor_sent_aps[i]=(int32_t*)calloc(1000,sizeof(int32_t));
      neighbor_sent_ap_when=(double**)realloc(neighbor_sent_ap_when,neighbors*sizeof(double*));
      neighbor_sent_ap_when[i]=(double*)calloc(1000,sizeof(double));
      for(uint32_t j=0; j < 1000; j++){
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now().GetSeconds();
    for(uint32_t j=0; j < 1000; j++)
      neighbor_hello_bundles[i][j]=0;
    
    uint8_t *msg=new uint8_t[packet->GetSize()+1];
    packet->CopyData(msg, packet->GetSize());
    msg[packet->GetSize()]='\0';
    const char *src=reinterpret_cast<const char *>(msg);
    char word[1024];
    strcpy(word, "");
    int j=0, n=0;
    int bundle_ids = 0;
    while(sscanf(src, "%1023s%n", word, &n) == 1){
      if(j == 0){
        b_a[i]=atoi(word);
      } 
      else{
        if(j == 1){
          bundle_ids=atoi(word);
        } 
        else{
          if(j <=(bundle_ids + 1)) 
            neighbor_hello_bundles[i][j-2]=strtol(word,NULL,16);
          else
            neighbor_hello_bundles[i][j-2]=-strtol(word,NULL,16);
          int m=0, sent_found=0;
          while((m < 1000) &&(sent_found == 0)){
            if(neighbor_hello_bundles[i][j-2] == neighbor_sent_aps[i][m]){
              sent_found=1;
            } 
            else
              m++;
            if(sent_found == 1){
              while((neighbor_sent_aps[i][m] != 0) &&(m < 999)){
                neighbor_sent_aps[i][m]=neighbor_sent_aps[i][m+1];
                neighbor_sent_ap_when[i][m]=neighbor_sent_ap_when[i][m+1];
                m++;
              }
              neighbor_sent_aps[i][999]=0;
              neighbor_sent_ap_when[i][999]=0;
            }
          }
        }
      }
      strcpy(word,"");
      src += n;
      j++;
    }
    delete [] msg;
  }
}

// void Base::ReceiveTeleport(Ptr<Packet> pkt){
//   Ptr<Packet> cpkt = pkt->Copy();
//   mypacket::TypeHeader tHeader(mypacket::MYTYPE_BNDL);
//   mypacket::BndlHeader bndlHeader;
//   cpkt->RemoveHeader(tHeader);
//   cpkt->RemoveHeader(bndlHeader);
//   uint32_t seqno = bndlHeader.GetOriginSeqno();
//   bool success = m_base_queue->Enqueue(cpkt);
//   if(success){
//     std::cout<<"Received bundle seqno "<<seqno<<" at the base station "<<m_base_queue->GetNPackets()<<"\n";
//   }
//   else{
//     std::cout<<"Received FAILED\n";
//   }
// }





////////////////////////////////////////////////////////////////
//////////////////////////////MAIN//////////////////////////////
////////////////////////////////////////////////////////////////
int main(int argc, char **argv){
  //LogComponentEnable("Ns2MobilityHelper",LOG_LEVEL_DEBUG);




  DtnExample test;
  if(! test.Configure(argc, argv)) 
    NS_FATAL_ERROR("Configuration failed. Aborted.");
  
  test.Run();
  // test.Report(std::cout);
  // for (uint32_t i=0; i<test.GetNodeNum(); i++){
  //   delete app[i];
  // }
  // delete[] app;
  return 0;
}