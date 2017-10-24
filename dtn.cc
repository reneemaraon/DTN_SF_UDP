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
#include <sstream>

using namespace ns3;

static void CourseChange (std::ostream *myos, std::string foo, Ptr<const MobilityModel> mobility){
  Ptr<Node> node = mobility->GetObject<Node> ();
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity
  
  std::cout.precision(5);
  *myos << Simulator::Now () << "; NODE: " << node->GetId() << "; POS: x=" << pos.x << ", y=" << pos.y
    << ", z=" << pos.z << "; VEL: x=" << vel.x << ", y=" << vel.y
    << ", z=" << vel.z << std::endl;
}

typedef std::map<Ptr<Socket>,int> sockOrder;

class DtnApp : public Application{

public:
  
  DtnApp ();
  virtual ~DtnApp();
  
  void Setup (Ptr<Node> node);
  // void DstHandleConnectionCreated (Ptr<Socket> s, const Address & addr); //DI NAMAN NAGAGAMIT
  void ReceiveHello (Ptr<Socket> socket);
  // void ScheduleTx (uint32_t dstnode, Time tNext, uint32_t packetsize); //DI NAMAN NAGAGAMIT
  void SendHello (Ptr<Socket> socket, double endTime, Time pktInterval, uint32_t first);
  void Retransmit (InetSocketAddress sendTo, int32_t id, int32_t retx); //CALLED NG SEND HELLO AND CHECK QUEUES
  void SendMore (InetSocketAddress sendTo, int32_t id, int32_t retx); // CALLED NG RETRANSMIT AND RECEIVE BUNDLE
  // void ConnectionSucceeds (Ptr<Socket> localSocket); //DI NAMAN NAGAGAMIT
  // void ConnectionFails (Ptr<Socket> localSocket); //DI NAMAN NAGAGAMIT
  void ReceiveBundle (Ptr<Socket> socket);
  // int IsStationary(); //BYE
  // void setStationary(int value); //BYE

// protected:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  
  void SendBundle (uint32_t dstnode, uint32_t packetsize);
  void SendAP (Ipv4Address srcaddr, Ipv4Address dstaddr, uint32_t seqno, Time srctimestamp); //CALLED NG RECEIVE BUNDLE
  void PrintBuffers (void); //CALLED NG START APPLICATION
  void CheckQueues (uint32_t bundletype); //CALLED NG CHECK QUEUES AND START APPLICATION
  int IsDuplicate (Ptr<Packet> pkt, Ptr<Queue> queue); //CALLED NG RECEIVE BUNDLE
  int AntipacketExists (Ptr<Packet> pkt); //CALLED NG RECEIVE BUNDLE
  void RemoveBundle (Ptr<Packet> pkt); //CALLED NG RECEIVE BUNDLE
  
  Ptr<Node>         m_node;
  Ptr<Socket>       m_socket;
  std::vector<Ptr<Packet> > newpkt;
  std::vector<Ptr<Packet> > retxpkt;
  Ptr<Queue>        m_antipacket_queue;
  Ptr<Queue>        m_queue;
  Ptr<Queue>        m_helper_queue;
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


DtnApp::DtnApp ()
  : m_socket (0),
    newpkt (0),
    retxpkt (0),
    m_antipacket_queue (0),
    m_queue (0),
    m_helper_queue (0),
    mac_queue(0),
    m_peer (),
    m_sendEvent (),
    m_running (false),
    m_serverReadSize (200), /* Is this OK? */
    neighbors (0),
    neighbor_address (0),
    neighbor_last_seen (0),
    currentServerRxBytes (0),
    neighbor_hello_bundles (0),
    neighbor_sent_bundles (0),
    neighbor_sent_aps (0),
    neighbor_sent_ap_when (0),
    bundles (0),
    bundle_address (0),
    bundle_seqno (0),
    bundle_retx (0),
    bundle_size (0),
    bundle_ts (0),
    sendTos (0),
    NumFlows(0),
    drops (0),
    t_c (0.8),
    b_s (1000000),
    b_a (0),
    rp (0), // 0: Epidemic, 1: Spray and Wait
    cc (0)  // 0: No congestion control, 1: Static t_c, 2: Dynamic t_c
{
}

DtnApp::~DtnApp(){
  m_socket = 0;
}

void DtnApp::Setup (Ptr<Node> node){
  m_node = node;
  m_antipacket_queue = CreateObject<DropTailQueue> ();
  m_queue = CreateObject<DropTailQueue> ();
  m_helper_queue = CreateObject<DropTailQueue> ();
  m_antipacket_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  m_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  m_helper_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  stationary =0;
  for(int i = 0; i < 10000; i++) {
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
  b_s = 1375000 + y->GetInteger(0, 1)*9625000;
}


// int DtnApp::IsStationary(){
//   return stationary;

// }

// void DtnApp::setStationary(int value){  
//   stationary = value;
// }

void DtnApp::StartApplication (void){
  m_running = true;
  Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (m_node->GetDevice (0));
  NS_ASSERT (dev != NULL);
  PointerValue ptr;
  dev->GetAttribute ("Mac",ptr);
  Ptr<AdhocWifiMac> mac = ptr.Get<AdhocWifiMac> ();
  NS_ASSERT (mac != NULL);
  Ptr<EdcaTxopN> edcaqueue = mac->GetBEQueue ();
  NS_ASSERT (edcaqueue != NULL);
  mac_queue = edcaqueue->GetEdcaQueue ();    
  NS_ASSERT (mac_queue != NULL);
  mac_queue->SetAttribute ("MaxPacketNumber", UintegerValue (1000));
  CheckQueues (2);
  PrintBuffers ();
}

void DtnApp::StopApplication (void){
  m_running = false;

  if (m_sendEvent.IsRunning ())
    Simulator::Cancel (m_sendEvent);
  
  if (m_socket)
    m_socket->Close ();
}

// void DtnApp::ConnectionSucceeds (Ptr<Socket> localSocket){
//   //std::cout << "TCP connection succeeds at time " << Simulator::Now ().GetSeconds () <<
//   //" at node " << m_node->GetId () << "\n";
// }

// void DtnApp::ConnectionFails (Ptr<Socket> localSocket){
//   std::cout << "TCP connection fails at time " << Simulator::Now ().GetSeconds () <<
//     " at node " << m_node->GetId () << "\n";
// }

void DtnApp::Retransmit (InetSocketAddress sendTo, int32_t id, int32_t retx){
  // Check that this is last call for retransmit, otherwise return

  int index = 0, found = 0;
  while ((found == 0) && (index < NumFlows)) {
    if ((sendTos[index] == sendTo) && (ids[index] == id) && (retxs[index] == retx) && ((Simulator::Now ().GetSeconds () - firstSendTime[index]) < 750.0)) {
      found = 1;
      if ((Simulator::Now ().GetSeconds () - lastSendTime[index] < 1.0)){
        return;
      }
    } 
    else
      index++;
  }
  if (found == 0)
    return;
  
  // Check that we are able to send, otherwise re-schedule and return
  uint32_t i = 0, neighbor_found = 0;
  while ((i < neighbors) && (neighbor_found == 0)) {
    if (neighbor_address[i].GetIpv4() == sendTo) {
      neighbor_found = 1;
      if ((Simulator::Now ().GetSeconds () - neighbor_last_seen[i]) > 0.1) {
        Simulator::Schedule (Seconds (1.0), &DtnApp::Retransmit, this, sendTo, id, retx);
        return;
      }
    } 
    else {
      i++;
    }
  }
  if (neighbor_found == 0)
    return;
  
  // Retransmit
  currentTxBytes[index] -= lastTxBytes[index];
  SendMore (sendTo, id, retx);
}

void DtnApp::SendMore (InetSocketAddress sendTo, int32_t id, int32_t retx){
  int index = 0, found = 0;
  while ((found == 0) && (index < NumFlows)) {
    if ((sendTos[index] == sendTo) && (ids[index] == id) && (retxs[index] == retx) && ((Simulator::Now ().GetSeconds () - firstSendTime[index]) < 750.0)) {
      found = 1;
    } 
    else
      index++;
  }
  if (found == 0)
    return;
  
  if (currentTxBytes[index] < totalTxBytes[index]) { 
    uint32_t left = totalTxBytes[index] - currentTxBytes[index];
    uint32_t dataOffset = currentTxBytes[index] % 1472;
    uint32_t toWrite = 1472 - dataOffset;
    toWrite = std::min (toWrite, left);
    Ptr<Packet> packet = Create<Packet> (toWrite);
    if (currentTxBytes[index] == 0)
      packet = retxpkt[index]->Copy ();
    packet->AddPacketTag (FlowIdTag (id));
    packet->AddPacketTag (QosTag (retx));
    InetSocketAddress addr (sendTo.GetIpv4(), 50000);
    m_socket->SendTo (packet, 0, addr);
    currentTxBytes[index] += toWrite;
    lastTxBytes[index] = toWrite;
    lastSendTime[index] = Simulator::Now ().GetSeconds ();
    Simulator::Schedule (Seconds (1.0), &DtnApp::Retransmit, this, sendTo, id, retx);
  } 
  else
    lastTxBytes[index] = 0;
}

void DtnApp::PrintBuffers (void) {
  uint32_t i=0, currentNeighbors=0;
  while (i < neighbors) {
    if ((Simulator::Now ().GetSeconds () - neighbor_last_seen[i]) < 0.1)
      currentNeighbors++;
    i++;
  }
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  std::cout << Simulator::Now ().GetSeconds () <<
    " " << mac_queue->GetSize() <<
    " " << m_antipacket_queue->GetNPackets() <<
    " " << m_queue->GetNPackets() <<
    " " << m_queue->GetNBytes() <<
    " " << currentNeighbors << 
    " " << mobility->GetPosition ().x <<
    " " << mobility->GetPosition ().y << "\n";
  Simulator::Schedule (Seconds (1.0), &DtnApp::PrintBuffers, this);
}

void DtnApp::CheckQueues (uint32_t bundletype) {
  Ptr<Packet> packet;
  Ptr<Packet> firstpacket;

  uint32_t i = 0, n = 0, pkts = 0, send_bundle = 0;
  mypacket::APHeader apHeader;
  mypacket::BndlHeader bndlHeader;

  // Remove expired antipackets and bundles -- do this check only once
  if (bundletype == 2) {
    pkts = m_antipacket_queue->GetNPackets();
    n = 0;
    //iterating through antipacket queue
    while (n < pkts) {
      n++;
      packet = m_antipacket_queue->Dequeue ();
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
      packet->RemoveHeader(tHeader);
      packet->RemoveHeader(apHeader);
      //if antipacket is less than 1000 seconds old, enqueue ulit
      if ((Simulator::Now ().GetSeconds () - apHeader.GetSrcTimestamp ().GetSeconds ()) < 1000.0) {
        packet->AddHeader (apHeader);
        packet->AddHeader (tHeader);
        bool success = m_antipacket_queue->Enqueue (packet);
        if (success) {
        }
      }
      // 
      else {
        uint32_t d=0;
        while (d < neighbors) {
          uint32_t m=0, sent_found=0;
          //rearranging neighbor_sent_aps, if nasend sa origin ng bundle being antipacketed. 
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_sent_aps[d][m] == -(int32_t)apHeader.GetOriginSeqno ()) {
              sent_found=1;
            } 
            else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_aps[d][m] != 0) && (m < 999)) {
                neighbor_sent_aps[d][m]=neighbor_sent_aps[d][m+1];
                neighbor_sent_ap_when[d][m]=neighbor_sent_ap_when[d][m+1];
                m++;
              }
              neighbor_sent_aps[d][999]=0;
              neighbor_sent_ap_when[d][999]=0;
            }
          }
          d++;
        }
      }
    }
    pkts = m_queue->GetNPackets();
    n = 0;

    //iterating through packets in m_queue
    while (n < pkts) {
      n++;
      packet = m_queue->Dequeue ();
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
      packet->RemoveHeader(tHeader);
      packet->RemoveHeader(bndlHeader);
      //if less 750, keep
      if (((Simulator::Now ().GetSeconds () - bndlHeader.GetSrcTimestamp ().GetSeconds ()) < 750.0) || (bndlHeader.GetHopCount () == 0)) {
        packet->AddHeader (bndlHeader);
        packet->AddHeader (tHeader);
        bool success = m_queue->Enqueue (packet);
        if (success) {
        }
      } 
      else {
        uint32_t d=0;
        while (d < neighbors) {
          uint32_t m=0, sent_found=0;
          //rearranging neighbor_sent_bundles, if nasend na ang bundle sa final destination
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_sent_bundles[d][m] == (int32_t)bndlHeader.GetOriginSeqno ()) {
              sent_found=1;
            } 
            else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_bundles[d][m] != 0) && (m < 999)) {
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
  if (mac_queue->GetSize () < 2) {
    
    if (bundletype == 2) {
      pkts = m_antipacket_queue->GetNPackets();
      n = 0;

      //iterating
      while ((n < pkts) && (send_bundle == 0)) {
    
        n++;
        ///// copying apheaders, packet and theader then enqueueing packet again /////
        packet = m_antipacket_queue->Dequeue ();
        mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
        packet->RemoveHeader(tHeader);
        packet->RemoveHeader(apHeader);
        packet->AddHeader (apHeader);
        packet->AddHeader (tHeader);
        Ptr<Packet> qp = packet->Copy();

        //////////////////////////////////////////////////////////////////
        


        if ((Simulator::Now ().GetSeconds () - apHeader.GetHopTimestamp ().GetSeconds ()) > 0.2) {
          i = 0;
          while ((i < neighbors) && (send_bundle == 0)) {
            //if bago lang nakita si neighbor, and aneighbor is not origin of antipacket
            if (((Simulator::Now ().GetSeconds () - neighbor_last_seen[i]) < 0.5) && (neighbor_address[i].GetIpv4() != apHeader.GetOrigin())) {
              // if (stationary == 0)
              int neighbor_has_bundle = 0, ap_sent = 0, j=0;
              
              while ((neighbor_has_bundle == 0) && (neighbor_hello_bundles[i][j] != 0) && (j < 1000)) {
                //check if neighbor has the antipacket
                std::cout <<neighbor_hello_bundles[i][j]<<" "<<-(int32_t)apHeader.GetOriginSeqno()<<"\n";
                if (neighbor_hello_bundles[i][j] == -(int32_t)apHeader.GetOriginSeqno ()){
                  std::cout<<"Neighbor has bundle\n";
                  neighbor_has_bundle = 1;
                }
                else
                  j++;
              }
              j=0;

              while ((neighbor_has_bundle == 0) && (ap_sent == 0) && (neighbor_sent_aps[i][j] != 0) && (j < 1000)) {
                if ((neighbor_sent_aps[i][j] == -(int32_t)apHeader.GetOriginSeqno ()) && (Simulator::Now ().GetSeconds () - neighbor_sent_ap_when[i][j] < 1.5))
                  ap_sent = 1;
                else
                  j++;
              }
              // if ((neighbor_has_bundle == 0) && (ap_sent == 0)) {
              if ((neighbor_has_bundle == 0) && (ap_sent == 0)) {
                //sending antipacket to this person
                std::cout <<"Sending antipacket with sequence number "<<apHeader.GetOriginSeqno()<<" to " <<neighbor_address[i].GetIpv4()<< "\n";
                if (stationary ==0)
                  send_bundle = 1;
                j = 0;
                while ((neighbor_sent_aps[i][j] != 0) && (neighbor_sent_aps[i][j] != -(int32_t)apHeader.GetOriginSeqno ()) && (j < 999))
                  j++; //positioning i and j
                neighbor_sent_aps[i][j] = -(int32_t)apHeader.GetOriginSeqno ();
                neighbor_sent_ap_when[i][j] = Simulator::Now ().GetSeconds ();
              } 
              else
                i++;
            } 
            else
              i++;
          }
        }
        if (send_bundle ==0){
          bool success = m_antipacket_queue->Enqueue (qp);
          if (success) {
          }
        }
      }
    }

     
    else { //if not bundle type ==2
      pkts = m_queue->GetNPackets();
      n = 0;
      while (n < pkts) {
        n++;
        packet = m_queue->Dequeue ();
        // packet = m_queue->Dequeue ();
        mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
        packet->RemoveHeader(tHeader);
        packet->RemoveHeader(bndlHeader);
        //if not old ang bundle
        if ((Simulator::Now ().GetSeconds () - bndlHeader.GetSrcTimestamp ().GetSeconds ()) < 750.0) {
          // ano lang basta di pa bago ang bundle narating sa node na itu
          if (n==1){
            if ((Simulator::Now ().GetSeconds () - bndlHeader.GetHopTimestamp ().GetSeconds ()) > 0.2) {

              Ipv4Address dst = bndlHeader.GetDst ();
              uint8_t spray = bndlHeader.GetSpray ();
              i = 0;
              //iterating through neighbors
              while ((i < neighbors) && (send_bundle == 0)) {
                // if dest of bundle is neighboraddress, tas bago lang nakita ang neighbor
                if (( ((bundletype == 0) && (spray > 0) && ((cc == 0)||(b_a[i] > packet->GetSize()))) || (dst == neighbor_address[i].GetIpv4())) && \
                  ((Simulator::Now ().GetSeconds () - neighbor_last_seen[i]) < 0.1) && (neighbor_address[i].GetIpv4() != bndlHeader.GetOrigin())) {
                  int neighbor_has_bundle = 0, bundle_sent = 0, j=0;
                  //check kung meron ba siya nung bundle
                  while ((neighbor_has_bundle == 0) && (neighbor_hello_bundles[i][j] != 0) && (j < 1000)) {
                    if ((unsigned)neighbor_hello_bundles[i][j] == bndlHeader.GetOriginSeqno ())
                      neighbor_has_bundle = 1;
                    else
                      j++;
                  }
                  j = 0;
                  //check if nasend na ba yung bundle sa kanya
                  while ((neighbor_has_bundle == 0) && (bundle_sent == 0) && (neighbor_sent_bundles[i][j] != 0) && (j < 1000)) {
                    if (neighbor_sent_bundles[i][j] == (int32_t)bndlHeader.GetOriginSeqno ())
                      bundle_sent = 1;
                    else
                      j++;
                  }
                  if ((neighbor_has_bundle == 0) && (bundle_sent == 0)) {
                    if (bundletype == 0) {
                      if (rp == 1)
                        bndlHeader.SetSpray (spray/2);
                      if (cc > 0) {
                        if (packet->GetSize() >= b_a[i])
                          b_a[i] = 0;
                        else
                          b_a[i] -= packet->GetSize();
                      }
                    } 
                    else {
                      // Wait 5.0 seconds before forwarding to other (than dst) nodes
                      bndlHeader.SetHopTimestamp (Simulator::Now () + Seconds (5.0));
                    }

                    send_bundle = 1;
                    j = 0;
                    while ((neighbor_sent_bundles[i][j] != 0) && (j < 999))
                      j++;
                    neighbor_sent_bundles[i][j] = (int32_t)bndlHeader.GetOriginSeqno ();
                  } 
                  else
                    i++;
                } 
                else
                  i++;
              }
            }
          }
          packet->AddHeader (bndlHeader);
          packet->AddHeader (tHeader);
          Ptr<Packet> qp = packet->Copy();
          if (n==1){
            firstpacket= packet->Copy();
          }
          bool success = m_queue->Enqueue (qp);
          if (success) {
          }
        } 
        else {
          //erase bundle kung di ikaw source tas old na 
          if (((Simulator::Now ().GetSeconds () - bndlHeader.GetSrcTimestamp ().GetSeconds ()) > 1000.0) && (bndlHeader.GetHopCount () == 0) && (bndlHeader.GetNretx () < 3)) {
            bndlHeader.SetSrcTimestamp (Simulator::Now ());
            bndlHeader.SetNretx (bndlHeader.GetNretx () + 1);
            uint32_t n=0;
            while (n < neighbors) {
              uint32_t m=0, sent_found=0;
              while ((m < 1000) && (sent_found == 0)) {
                if (neighbor_sent_bundles[n][m] == (int32_t)bndlHeader.GetOriginSeqno ()) {
                  sent_found=1;
                } 
                else
                  m++;
                if (sent_found == 1) {
                  while ((neighbor_sent_bundles[n][m] != 0) && (m < 999)) {
                    neighbor_sent_bundles[n][m]=neighbor_sent_bundles[n][m+1];
                    m++;
                  }
                  neighbor_sent_bundles[n][999]=0;
                }
              }
              n++;
            }
          }
          if ((bndlHeader.GetHopCount () == 0) && ((Simulator::Now ().GetSeconds () - bndlHeader.GetSrcTimestamp ().GetSeconds ()) <= 1000.0)) {
            packet->AddHeader (bndlHeader);
            packet->AddHeader (tHeader);
            bool success = m_queue->Enqueue (packet);
            if (success) {
            }
          }
        }
      }
      packet = firstpacket;


    }
  } 
  else {
    bundletype = 0;
  }
  if (send_bundle == 1) {
    if (m_socket == 0) {
      m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
      Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
      Ipv4Address ipaddr = (ipv4->GetAddress (1, 0)).GetLocal ();
      InetSocketAddress local = InetSocketAddress (ipaddr, 50000);
      m_socket->Bind (local);    
    }    

    InetSocketAddress dstremoteaddr (neighbor_address[i].GetIpv4(), 50000);    
    if (bundletype < 2) {
      packet = firstpacket->Copy();
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
      Ptr<Packet> anotherp = packet->Copy();

      anotherp->RemoveHeader(tHeader);
      anotherp->RemoveHeader(bndlHeader);
      anotherp->AddHeader(bndlHeader);
      anotherp->AddHeader(tHeader);
      std::cout<<"Retransmitting from 10.0.0."<<m_node->GetId()+1<<" to "<<neighbor_address[i].GetIpv4()<<" sequence "<<bndlHeader.GetOriginSeqno()<<"\n";
      NumFlows++;
      sendTos=(InetSocketAddress*)realloc(sendTos,NumFlows*sizeof(InetSocketAddress));
      sendTos[NumFlows-1] = dstremoteaddr.GetIpv4();
      ids[NumFlows-1] = bndlHeader.GetOriginSeqno ();
      retxs[NumFlows-1] = bndlHeader.GetNretx ();
      currentTxBytes[NumFlows-1] = std::min ((uint32_t)1472, packet->GetSize ());
      lastTxBytes[NumFlows-1] = std::min ((uint32_t)1472, packet->GetSize ());
      firstSendTime[NumFlows-1] = Simulator::Now ().GetSeconds ();
      lastSendTime[NumFlows-1] = Simulator::Now ().GetSeconds ();
      totalTxBytes[NumFlows-1] = packet->GetSize ();
      if (packet->GetSize () > 1472)
        packet->RemoveAtEnd (packet->GetSize () - 1472);
      packet->AddPacketTag (FlowIdTag (bndlHeader.GetOriginSeqno ()));
      packet->AddPacketTag (QosTag (bndlHeader.GetNretx ()));
      retxpkt.push_back (packet->Copy ());
      Simulator::Schedule (Seconds (1.0), &DtnApp::Retransmit, this, sendTos[NumFlows-1], ids[NumFlows-1], retxs[NumFlows-1]);
    } 
    else {    
      packet->AddPacketTag (FlowIdTag (-apHeader.GetOriginSeqno ()));
      packet->AddPacketTag (QosTag (4));
    }
    
    m_socket->SendTo (packet, 0, dstremoteaddr);
    Address ownaddress;
    m_socket->GetSockName (ownaddress);
    //InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom (ownaddress);    
  }
  if (bundletype == 2) {
    if (send_bundle == 0)
      CheckQueues(1);
    else
      Simulator::Schedule (Seconds (0.001), &DtnApp::CheckQueues, this, 2);
  }
  if (bundletype == 1) {
    if (send_bundle == 0)
      CheckQueues(0);
    else
      Simulator::Schedule (Seconds (0.001), &DtnApp::CheckQueues, this, 2);
  }
  if (bundletype == 0) {
    if (send_bundle == 0)
      Simulator::Schedule (Seconds (0.01), &DtnApp::CheckQueues, this, 2);
    else
      Simulator::Schedule (Seconds (0.001), &DtnApp::CheckQueues, this, 2);
  }
}

void DtnApp::SendBundle (uint32_t dstnode, uint32_t packetsize){
  // std::cout<< "SendBundle pasok. " ;
  Ptr<Packet> packet = Create<Packet> (packetsize);
  mypacket::BndlHeader bndlHeader;
  char srcstring[1024]="";
  sprintf(srcstring,"10.0.0.%d",(m_node->GetId () + 1));
  char dststring[1024]="";
  sprintf(dststring,"10.0.0.%d",(dstnode+1));
  // std::cout<< "SendBundle from " << m_node->GetId () <<" to " << dstnode <<" with size " << packetsize<<"\n";
  bndlHeader.SetOrigin (srcstring);
  bndlHeader.SetDst (dststring);
  bndlHeader.SetOriginSeqno (packet->GetUid());
  bndlHeader.SetHopCount (0);
  bndlHeader.SetSpray (4);
  bndlHeader.SetNretx (0);
  bndlHeader.SetBundleSize (packetsize);
  bndlHeader.SetSrcTimestamp (Simulator::Now ());
  bndlHeader.SetHopTimestamp (Simulator::Now ());
  packet->AddHeader (bndlHeader);
  mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
  packet->AddHeader (tHeader);
  if ((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + packet->GetSize()) <= b_s) {
    bool success = m_queue->Enqueue (packet);
    if (success) {
      std::cout << "At time " << Simulator::Now ().GetSeconds () <<
        " send bundle with sequence number " <<  bndlHeader.GetOriginSeqno () <<
        " from " <<  bndlHeader.GetOrigin () <<
        " to " << bndlHeader.GetDst () << "\n";
    }
  } 
  else {
    std::cout << "At time " << Simulator::Now ().GetSeconds () <<
      " tried to send bundle with sequence number " <<  bndlHeader.GetOriginSeqno () <<
      " from " <<  bndlHeader.GetOrigin () <<
      " to " << bndlHeader.GetDst () << "\n";
  }
}

void DtnApp::SendAP (Ipv4Address srcstring, Ipv4Address dststring, uint32_t seqno, Time srctimestamp){
  Ptr<Packet> packet = Create<Packet> (10);
  mypacket::APHeader apHeader;
  apHeader.SetOrigin (srcstring);
  apHeader.SetDst (dststring);
  apHeader.SetOriginSeqno (seqno);
  apHeader.SetHopCount (0);
  apHeader.SetBundleSize (10);
  double newtimestamp = srctimestamp.GetSeconds () - (250.0 - (Simulator::Now ().GetSeconds () - srctimestamp.GetSeconds ()));
  if (newtimestamp < 0.0)
    newtimestamp = 0.0;
  if ((Simulator::Now ().GetSeconds () - srctimestamp.GetSeconds ()) < 250.0)
    apHeader.SetSrcTimestamp (Seconds (newtimestamp));
  else
    apHeader.SetSrcTimestamp (srctimestamp);
    apHeader.SetHopTimestamp (Simulator::Now ());
    packet->AddHeader (apHeader);
    mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
    packet->AddHeader (tHeader);
    bool success = m_antipacket_queue->Enqueue (packet);
    if (success)
      std::cout << "At time " << Simulator::Now ().GetSeconds () <<
        " send antipacket with sequence number " <<  apHeader.GetOriginSeqno () <<
        " original ts " <<  srctimestamp.GetSeconds () <<
        " new ts " <<  apHeader.GetSrcTimestamp ().GetSeconds () <<
        " from " <<  apHeader.GetOrigin () <<
      " to " << apHeader.GetDst () << "\n";
}

// void DtnApp::ScheduleTx (uint32_t dstnode, Time tNext, uint32_t packetsize){
//   // std::cout<<"SCHEDULE SENDBUNDLE FROM "<< m_node->GetId ()  << " to " << dstnode <<"\n";
//   m_sendEvent = Simulator::Schedule (tNext, &DtnApp::SendBundle, this, dstnode, packetsize);
// }

// void DtnApp::DstHandleConnectionCreated (Ptr<Socket> s, const Address & addr){
//   s->SetRecvCallback (MakeCallback (&DtnApp::ReceiveBundle, this));
// }

int DtnApp::IsDuplicate (Ptr<Packet> pkt, Ptr<Queue> queue){
  Ptr<Packet> cpkt = pkt->Copy();
  int duplicate = 0; 
  mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(bndlHeader);
  uint32_t seqno = bndlHeader.GetOriginSeqno ();
  uint32_t pkts = queue->GetNPackets();
  uint32_t i = 0;
  while (i < pkts) {
    Ptr<Packet> p = queue->Dequeue ();
    if (duplicate == 0) {
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
      p->RemoveHeader(tHeader);
      if (tHeader.Get () == mypacket::MYTYPE_AP)
        mypacket::APHeader bndlHeader;
      else
        mypacket::BndlHeader bndlHeader;
        p->RemoveHeader(bndlHeader);
      if (bndlHeader.GetOriginSeqno () == seqno)
        duplicate = 1;
      p->AddHeader(bndlHeader);
      p->AddHeader(tHeader);
    }
    bool success = queue->Enqueue (p);
    if (success) {
    } 
    i++;
  }
  return (duplicate);
}

int DtnApp::AntipacketExists (Ptr<Packet> pkt){
  Ptr<Packet> cpkt = pkt->Copy();
  int apExists = 0;
  mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
  mypacket::BndlHeader bndlHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(bndlHeader);
  uint32_t seqno = bndlHeader.GetOriginSeqno ();
  uint32_t pkts = m_antipacket_queue->GetNPackets();
  uint32_t i = 0;
  while (i < pkts) {
    Ptr<Packet> p = m_antipacket_queue->Dequeue ();
    if (apExists == 0) {
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
      mypacket::APHeader apHeader;
      p->RemoveHeader(tHeader);
      p->RemoveHeader(apHeader);
      if (apHeader.GetOriginSeqno () == seqno)
        apExists = 1;
      p->AddHeader(apHeader);
      p->AddHeader(tHeader);
    }
    bool success = m_antipacket_queue->Enqueue (p);
    if (success) {
    }
    i++;
  }
  return (apExists);
}

void DtnApp::RemoveBundle (Ptr<Packet> pkt){
  Ptr<Packet> cpkt = pkt->Copy(); 
  mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
  mypacket::APHeader apHeader;
  cpkt->RemoveHeader(tHeader);
  cpkt->RemoveHeader(apHeader);
  uint32_t seqno = apHeader.GetOriginSeqno ();
  uint32_t pkts = m_queue->GetNPackets();
  uint32_t i = 0, found = 0;
  while (i < pkts) {
    Ptr<Packet> p = m_queue->Dequeue ();
    if (found == 0) {
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
      mypacket::BndlHeader bndlHeader;
      p->RemoveHeader(tHeader);
      p->RemoveHeader(bndlHeader);
      if (bndlHeader.GetOriginSeqno () != seqno) {
        p->AddHeader(bndlHeader);
        p->AddHeader(tHeader);
        bool success = m_queue->Enqueue (p);
        if (success) {
        }  
      } 
      else{
        std::cout<<"Removing bundle of sequence "<<bndlHeader.GetOriginSeqno()<<"\n";
        found = 1;
        uint32_t n=0;
        while (n < neighbors) {
          uint32_t m=0, sent_found=0;
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_sent_bundles[n][m] == (int32_t)seqno) {
              sent_found=1;
            } 
            else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_bundles[n][m] != 0) && (m < 999)) {
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
    else {
      bool success = m_queue->Enqueue (p);
      if (success) {
      }
    }
    i++;
  }
  int index = 0;
  while (index < NumFlows) {
    if (ids[index] == (int32_t)seqno)
      ids[index] = 0;
    index++;
  }
}

void DtnApp::ReceiveBundle (Ptr<Socket> socket){
  //m_node or GetNode() is yung receiver
  Address ownaddress;
  socket->GetSockName (ownaddress);
  InetSocketAddress owniaddress = InetSocketAddress::ConvertFrom (ownaddress); //receiver address   
  while (socket->GetRxAvailable () > 0) {
    Address from;
    Ptr<Packet> p = socket->RecvFrom (from);
    InetSocketAddress address = InetSocketAddress::ConvertFrom (from); //sender address

    // hello here
    uint32_t i = 0;
    uint32_t found = 0;
    while ((i < neighbors) && (found == 0)) {
      if (address.GetIpv4() == neighbor_address[i].GetIpv4()) {
        found = 1;
      } 
      else
        i++;
    }
    if (found == 0) {
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
      for(uint32_t j=0; j < 1000; j++) {
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now ().GetSeconds ();






    // std::cout<< "RcvBundle: rcvrNode "<< GetNode() <<"  rcvrSocket: "<<socket <<"  rcvrIP: "<<owniaddress.GetIpv4() <<"  sndrIP: "<<address.GetIpv4() <<"\n";
    int src_seqno = 0;
    QosTag tag;
    int packet_type = 0;
    if (p->PeekPacketTag (tag))
      packet_type = tag.GetTid ();
    if (packet_type == 5) { // Ack
      p->RemoveAllByteTags ();
      p->RemoveAllPacketTags ();
      uint8_t *msg=new uint8_t[p->GetSize()+1];
      p->CopyData (msg, p->GetSize());
      msg[p->GetSize()]='\0';
      const char *src=reinterpret_cast<const char *>(msg);
      char word[1024];
      strcpy(word, "");
      int j=0, n=0;
      int32_t id = 0;
      int32_t retx = 0;
      while (sscanf (src, "%1023s%n", word, &n) == 1) {
        if (j == 0)
          id=strtol(word,NULL,16);
        else
          retx=strtol(word,NULL,16);
        strcpy(word,"");
        src += n;
        j++;
      }
      delete [] msg;
      SendMore (address.GetIpv4(), id, retx);
      return;
    } 
    else {
      FlowIdTag ftag = 0;
      if (p->PeekPacketTag (ftag))
        src_seqno = ftag.GetFlowId ();
      std::stringstream msg;
      msg.clear ();
      msg.str ("");
      char seqnostring[1024]="";
      sprintf(seqnostring," %x %x", src_seqno, packet_type); // Add: how much data received; will be used by the sender. If total received > bundle size: discard packet.
      msg << seqnostring;
      Ptr<Packet> ack = Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());
      if (m_socket == 0) {
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
        Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
        Ipv4Address ipaddr = (ipv4->GetAddress (1, 0)).GetLocal ();
        InetSocketAddress local = InetSocketAddress (ipaddr, 50000);
        // std::cout<< "ip  node "<<ipaddr<<"\n";
          // std::cout<< "receiverNode: " << m_node <<"  msocket "<<m_socket <<"  receiverIP "<<ipaddr <<"\n";
        m_socket->Bind (local);    
      }
      ack->AddPacketTag (QosTag (5));
      InetSocketAddress ackaddr (address.GetIpv4(), 50000);
      m_socket->SendTo (ack, 0, ackaddr);
    }
    p->RemoveAllByteTags ();
    p->RemoveAllPacketTags ();

    i = 0;
    found = 0;
    while ((i < bundles) && (found == 0)) {
      if ((address.GetIpv4() == bundle_address[i].GetIpv4()) && (src_seqno == bundle_seqno[i]) && (packet_type == bundle_retx[i])) {
        found = 1;
      } 
      else
      i++;
    }
    if (found == 0) {
      i = 0;
      while ((i < bundles) && (found == 0)) {
        if (currentServerRxBytes[i] == 0) {
          found = 1;
          bundle_address[i] = address.GetIpv4();
          bundle_seqno[i] = src_seqno;
          bundle_retx[i] = packet_type;
          bundle_ts[i] = Simulator::Now ().GetSeconds ();
          currentServerRxBytes[i] = 0;
        } 
        else
          i++;
      }
    }
    if (found == 0) {
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
      bundle_ts[i] = Simulator::Now ().GetSeconds ();
      newpkt.push_back (p->Copy ());
    }
    if (p == 0 && socket->GetErrno () != Socket::ERROR_NOTERROR)
      NS_FATAL_ERROR ("Server could not read stream at byte " << currentServerRxBytes[i]);
    if (currentServerRxBytes[i] == 0) {
      currentServerRxBytes[i] += p->GetSize ();
      newpkt[i] = p;
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
      newpkt[i]->RemoveHeader(tHeader);
      if (tHeader.Get () == mypacket::MYTYPE_AP) {
        mypacket::APHeader apHeader;
        newpkt[i]->RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize () + 26;
        newpkt[i]->AddHeader(apHeader);
      } 
      else {
        if (tHeader.Get () == mypacket::MYTYPE_BNDL) {
          mypacket::BndlHeader bndlHeader;
          newpkt[i]->RemoveHeader(bndlHeader);
          bundle_size[i] = bndlHeader.GetBundleSize () + 28;
          newpkt[i]->AddHeader(bndlHeader);
        } 
        else {
          // Bundle fragments arrive in wrong order; no bundle header
          currentServerRxBytes[i] = 0;
          return;
        }
      }
      newpkt[i]->AddHeader(tHeader);
    } 
    else {
      if (currentServerRxBytes[i] > bundle_size[i]) {
        std::cout<<"Current server bytes: "<<currentServerRxBytes[i]<<" Bundle size: "<<bundle_size[i]<<"\n";
        std::cout << "WTF at time " << Simulator::Now ().GetSeconds () <<
          " received " << p->GetSize() <<
          " bytes at " << owniaddress.GetIpv4 () <<
          " total bytes " << currentServerRxBytes[i] <<
          " from " << address.GetIpv4() <<
          " seqno " << src_seqno << "\n";
      } 
      else {
        currentServerRxBytes[i] += p->GetSize ();
        newpkt[i]->AddAtEnd (p);
      }
    }
    if (currentServerRxBytes[i] == bundle_size[i]) {
      currentServerRxBytes[i] = 0;
      Ptr<Packet> qpkt = newpkt[i]->Copy ();
      mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
      newpkt[i]->RemoveHeader(tHeader);
      if (tHeader.Get () == mypacket::MYTYPE_AP) {
        mypacket::APHeader apHeader;
        newpkt[i]->RemoveHeader(apHeader);
        bundle_size[i] = apHeader.GetBundleSize ();
        if ((IsDuplicate (qpkt, m_antipacket_queue) == 0) && ((Simulator::Now ().GetSeconds () - apHeader.GetSrcTimestamp ().GetSeconds ()) < 1000.0)) {
          mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
          qpkt->RemoveHeader(tHeader);
          mypacket::APHeader apHeader;
          qpkt->RemoveHeader(apHeader);
          apHeader.SetHopTimestamp (Simulator::Now ());
          apHeader.SetHopCount (apHeader.GetHopCount () + 1);
          qpkt->AddHeader (apHeader);
          qpkt->AddHeader (tHeader);
          bool success = m_antipacket_queue->Enqueue (qpkt);
          if (success) {
          }
          RemoveBundle (qpkt); 
        }
      } 
      else {
        mypacket::BndlHeader bndlHeader;
        newpkt[i]->RemoveHeader(bndlHeader);
        bundle_size[i] = bndlHeader.GetBundleSize ();
        if (IsDuplicate (qpkt, m_queue) == 1)
          std::cout << "At time " << Simulator::Now ().GetSeconds () <<
            " received duplicate " << newpkt[i]->GetSize() <<
            " bytes at " << owniaddress.GetIpv4 () <<
            " from " << address.GetIpv4 () <<
            " bundle hop count: "  << (unsigned)bndlHeader.GetHopCount () <<
            " sequence number: "  << bndlHeader.GetOriginSeqno () <<
            " bundle queue occupancy: " << m_queue->GetNBytes () << "\n";
        if ((IsDuplicate (qpkt, m_queue) == 0) && (AntipacketExists (qpkt) == 0) && ((Simulator::Now ().GetSeconds () - bndlHeader.GetSrcTimestamp ().GetSeconds ()) < 750.0)) {
          if (bndlHeader.GetDst () == owniaddress.GetIpv4 ()) {
            std::cout << "At time " << Simulator::Now ().GetSeconds () <<
              " received " << newpkt[i]->GetSize() <<
              " bytes at " << owniaddress.GetIpv4 () <<
              " (final dst) from " << address.GetIpv4 () <<
              " delay: "  << Simulator::Now ().GetSeconds () - bndlHeader.GetSrcTimestamp ().GetSeconds () + 1000.0*(bndlHeader.GetNretx ()) <<
              " bundle hop count: "  << (unsigned)bndlHeader.GetHopCount () + 1 <<
              " sequence number: "  << bndlHeader.GetOriginSeqno () <<
              " bundle queue occupancy: " << m_queue->GetNBytes () << "\n";
            SendAP (bndlHeader.GetDst (), bndlHeader.GetOrigin (), bndlHeader.GetOriginSeqno (), bndlHeader.GetSrcTimestamp ());
          } 
          else {
            mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
            qpkt->RemoveHeader(tHeader);
            mypacket::BndlHeader bndlHeader;
            qpkt->RemoveHeader(bndlHeader);
            bndlHeader.SetHopTimestamp (Simulator::Now ());
            bndlHeader.SetHopCount (bndlHeader.GetHopCount () + 1);
            qpkt->AddHeader (bndlHeader);
            qpkt->AddHeader (tHeader);
            if ((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + qpkt->GetSize()) <= b_s) {
              bool success = m_queue->Enqueue (qpkt);
              if (success) {
              }
            } 
            else {
              drops++;
              std::cout << "At time " << Simulator::Now ().GetSeconds () <<
              " dropped " << newpkt[i]->GetSize() <<
              " bytes at " << owniaddress.GetIpv4 () <<
              " from " << address.GetIpv4 () <<
              " bundle hop count: "  << (unsigned)bndlHeader.GetHopCount () <<
              " sequence number: "  << bndlHeader.GetOriginSeqno () <<
              " bundle queue occupancy: " << m_queue->GetNBytes () << "\n";
            }
          }
        }
      }
    }
  }
}


void DtnApp::SendHello (Ptr<Socket> socket, double endTime, Time pktInterval, uint32_t first) {
  // std::cout<<"SendHello. SSocket "<< socket <<"\n";
  if (first == 0) {
    double now (Simulator::Now ().GetSeconds ());
    if (now < endTime) {
      std::stringstream msg;
      msg.clear ();
      msg.str ("");
      char seqnostring[1024]="";
      if (cc == 2) {
        if ((drops == 0) && (t_c < 0.9)) {
          t_c += 0.01;
        } 
        else {
          if ((drops > 0) && (t_c > 0.5))
            t_c = t_c * 0.8;
          drops = 0;
        }
      }
      if ((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes()) >= (uint32_t)(t_c * b_s))
        sprintf(seqnostring,"%d",0);
      else
        sprintf(seqnostring,"%d",((uint32_t)(t_c * b_s) - m_queue->GetNBytes() - m_antipacket_queue->GetNBytes()));
      msg << seqnostring;
      uint32_t pkts = m_queue->GetNPackets();   
      // Reorder packets: put the least forwarded first
      uint32_t n = 0;
      Ptr<Packet> packet;
      while (n < pkts) {
        n++;
        packet = m_queue->Dequeue ();
        bool success = m_helper_queue->Enqueue (packet);
        if (success) {
        }
      }
      uint32_t m = 0;
      while (m < pkts) {
        m++;
        uint32_t min_count = 10000, min_seqno = 0, helper_pkts = m_helper_queue->GetNPackets();
        n = 0;
        while (n < helper_pkts) {
          n++;
          packet = m_helper_queue->Dequeue ();
          mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet->RemoveHeader(tHeader);
          packet->RemoveHeader(bndlHeader);
          int index = 0;
          uint32_t count = 0;
          while (index < NumFlows) {
            if (ids[index] == (int32_t)bndlHeader.GetOriginSeqno ())
              count++;
            index++;
          }
          if (count < min_count) {
            min_count = count;
            min_seqno = bndlHeader.GetOriginSeqno ();
          }
          packet->AddHeader (bndlHeader);
          packet->AddHeader (tHeader);
          bool success = m_helper_queue->Enqueue (packet);
          if (success) {
          }
        }
        int min_found = 0;
        while (min_found == 0) {
          packet = m_helper_queue->Dequeue ();
          mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          packet->RemoveHeader(tHeader);
          packet->RemoveHeader(bndlHeader);
          if (bndlHeader.GetOriginSeqno () == min_seqno) {
            min_found = 1;
            packet->AddHeader (bndlHeader);
            packet->AddHeader (tHeader);
            bool success = m_queue->Enqueue (packet);
            if (success) {
            }
          } 
          else {
            packet->AddHeader (bndlHeader);
            packet->AddHeader (tHeader);
            bool success = m_helper_queue->Enqueue (packet);
            if (success) {
            }
          }
        }
      }
      // End of reorder  
      char seqnostring_b[1024]="";
      sprintf(seqnostring_b," %d",pkts);
      msg << seqnostring_b;
      for (uint32_t i = 0; i < pkts; ++i) {
        Ptr<Packet> p = m_queue->Dequeue ();
        if (msg.str().length() < 2280) {
        // The default value of MAC-level MTU is 2296
          mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
          mypacket::BndlHeader bndlHeader;
          p->RemoveHeader(tHeader);
          p->RemoveHeader(bndlHeader);
          uint32_t src_seqno = bndlHeader.GetOriginSeqno ();
          char seqnostring_a[1024]="";
          sprintf(seqnostring_a," %x",(src_seqno));
          msg << seqnostring_a;
          p->AddHeader(bndlHeader);
          p->AddHeader(tHeader);
        } 
        else {
          std::cout << "At time " << Simulator::Now ().GetSeconds () <<
            " too big Hello (B) (" << msg.str().length() << ") bytes.\n";
        }
        bool success = m_queue->Enqueue (p);
        if (success) {
        }  
      }
      uint32_t apkts = m_antipacket_queue->GetNPackets();
      for (uint32_t i = 0; i < apkts; ++i) {
        Ptr<Packet> p = m_antipacket_queue->Dequeue ();
        if (msg.str().length() < 2280) {
          mypacket::TypeHeader tHeader (mypacket::MYTYPE_AP);
          mypacket::APHeader apHeader;
          p->RemoveHeader(tHeader);
          p->RemoveHeader(apHeader);
          uint32_t src_seqno = apHeader.GetOriginSeqno ();
          char seqnostring_a[1024]="";
          sprintf(seqnostring_a," %x",(src_seqno));
          msg << seqnostring_a;
          p->AddHeader(apHeader);
          p->AddHeader(tHeader);
        } 
        else {
          std::cout << "At time " << Simulator::Now ().GetSeconds () <<
            " too big Hello (AP) (" << msg.str().length() << ") bytes.\n";                   
        }
        bool success = m_antipacket_queue->Enqueue (p);
        if (success) {
        }
      }
      Ptr<Packet> pkt = Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());
      pkt->AddPacketTag (QosTag (6)); // High priority 
      socket->Send (pkt);
      Simulator::Schedule (Seconds (0.1), &DtnApp::SendHello, this, socket, endTime, Seconds (0.1), 0);
    } 
    else
      socket->Close ();
  } 
  else
    Simulator::Schedule (pktInterval, &DtnApp::SendHello, this, socket, endTime, pktInterval, 0);
}


void DtnApp::ReceiveHello (Ptr<Socket> socket){
  // std::cout<<"ReceiveHello. RSocket "<< socket<<"\n";
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from)) {
    InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
    // std::cout<< "ReceiveHello. rcvrNode "<< GetNode() <<"  RSocket: "<<socket <<"  sIP: "<<address.GetIpv4() <<"\n";
    uint32_t i = 0;
    uint32_t found = 0;
    while ((i < neighbors) && (found == 0)) {
      if (address.GetIpv4() == neighbor_address[i].GetIpv4()) {
        found = 1;
      } 
      else
        i++;
    }
    if (found == 0) {
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
      for(uint32_t j=0; j < 1000; j++) {
        neighbor_sent_bundles[i][j]=0;
        neighbor_sent_aps[i][j]=0;
        neighbor_sent_ap_when[i][j]=0;
      }
    }
    neighbor_last_seen[i] = Simulator::Now ().GetSeconds ();
    for(uint32_t j=0; j < 1000; j++)
      neighbor_hello_bundles[i][j]=0;
    
    uint8_t *msg=new uint8_t[packet->GetSize()+1];
    packet->CopyData (msg, packet->GetSize());
    msg[packet->GetSize()]='\0';
    const char *src=reinterpret_cast<const char *>(msg);
    char word[1024];
    strcpy(word, "");
    int j=0, n=0;
    int bundle_ids = 0;
    while (sscanf (src, "%1023s%n", word, &n) == 1) {
      if (j == 0) {
        b_a[i]=atoi(word);
      } 
      else {
        if (j == 1) {
          bundle_ids=atoi(word);
        } 
        else {
          if (j <= (bundle_ids + 1)) 
            neighbor_hello_bundles[i][j-2]=strtol(word,NULL,16);
          else
            neighbor_hello_bundles[i][j-2]=-strtol(word,NULL,16);
          int m=0, sent_found=0;
          while ((m < 1000) && (sent_found == 0)) {
            if (neighbor_hello_bundles[i][j-2] == neighbor_sent_aps[i][m]) {
              sent_found=1;
            } 
            else
              m++;
            if (sent_found == 1) {
              while ((neighbor_sent_aps[i][m] != 0) && (m < 999)) {
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


class Stationary: public DtnApp {

public:
  void StationarySetup(Ptr<Node> node);
  void BufferSetup(uint32_t numOfEntries, uint32_t entrySize, float secondsIntervalinput);
  void GenerateData(uint32_t first);
  void StoreInBuffer(std::string tempor);
  void CreateBundle();

  int bufferCount;
  int entryLength;
  int bufferLength;
  uint32_t secondsInterval;

  // uint32_t  bundleSize;
  Ptr<Queue> bufferTest;
  QueueStruct buffer;

  int dataSizeInBundle;
  int dataIDSize;
  int nextID;
  int maxID;
  uint32_t destinationNode;
};


void Stationary::StationarySetup(Ptr<Node> node){
  m_node = node;
  m_antipacket_queue = CreateObject<DropTailQueue> ();
  m_queue = CreateObject<DropTailQueue> ();
  m_helper_queue = CreateObject<DropTailQueue> ();
  m_antipacket_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  m_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  m_helper_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  stationary = 1;
  dataSizeInBundle=5;
  dataIDSize=dataSizeInBundle-2;
  nextID=000;
  maxID=pow(10,dataIDSize)-1;
  // std::cout<< "nextID|||maxID "<<nextID<<"|||"<<maxID<<"\n";
  for(int i = 0; i < 10000; i++) {
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
  b_s = 1375000 + y->GetInteger(0, 1)*9625000;
}


void Stationary::GenerateData(uint32_t first){
  if (first==0){
    // if (bufferCount<bufferLength){
    const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::cout<<"Generated Data for node "<<m_node->GetId()<<" at time :"<<Simulator::Now ()<<" with data ";
    std::stringstream holder;
    holder << nextID;
    std::string tempor=std::string(dataIDSize - holder.str().length(), '0') + holder.str();
    if(nextID==maxID){
      nextID=000;
    }
    else{
      nextID++;
    }
    for (int i=0; i<(entryLength-dataIDSize); i++){
      tempor += alphanum[rand() % 36];
    }
    // std::cout<< "----------------------node is " << m_node->GetId()<<"\n" <<"BEFORE LIST";
    // buffer.listPrinter();
    // std::cout << "TEMPOR IS: "<< tempor <<"\n";
    std::cout << tempor <<"\n";
    StoreInBuffer(tempor);

    // std::cout<< "AFTER LIST NOW IS: ";
    // buffer.listPrinter();
    // std::cout<< "----------------------node is " << m_node->GetId()<<"\n";
    // bufferCount=bufferCount+1;
    Simulator::Schedule (Seconds (secondsInterval), &Stationary::GenerateData, this, 0);
    // }
  }
  else{
    Simulator::Schedule (Seconds (secondsInterval), &Stationary::GenerateData, this, 0);
  }
}


void Stationary::StoreInBuffer(std::string tempor){
    if(buffer.getSize() <= bufferLength ){
      buffer.enqueue(tempor);
      // std::cout<< "enqueue LIST NOW IS: ";
      // buffer.listPrinter();
      if(buffer.getSize() >= dataSizeInBundle){
        CreateBundle();
      }
    }
    //eviction policy?????????????????
}

void Stationary::CreateBundle(){
  std::string payload="";
  for(int y=0; y<dataSizeInBundle; y++){
    payload+=buffer.get(0);
    buffer.dequeue();
    // buffer.listPrinter();
  }
  int bndlSize=100000;//?????????????????? how to compute hehe
  std::stringstream bndlData;
  bndlData << payload ;

  // std::cout<<"payload: "<<payload <<" bndlData " <<bndlData<< " bndlData str ekek "<< (uint8_t*) bndlData.str().c_str()<<"\n";
  
  Ptr<Packet> packet = Create<Packet>((uint8_t*) bndlData.str().c_str(), bndlSize);
  mypacket::BndlHeader bndlHeader;
  char srcstring[1024]="";
  sprintf(srcstring,"10.0.0.%d",(m_node->GetId () + 1));
  char dststring[1024]="";
  sprintf(dststring,"10.0.0.%d",(destinationNode+1));
  // std::cout<< "SendBundle from " << m_node->GetId () <<" to " << destinationNode <<" with size " << bndlSize<<"\n";
  bndlHeader.SetOrigin (srcstring);
  bndlHeader.SetDst (dststring);
  bndlHeader.SetOriginSeqno (packet->GetUid());
  bndlHeader.SetHopCount (0);
  bndlHeader.SetSpray (4);
  bndlHeader.SetNretx (0);
  bndlHeader.SetBundleSize (bndlSize);
  bndlHeader.SetSrcTimestamp (Simulator::Now ());
  bndlHeader.SetHopTimestamp (Simulator::Now ());
  packet->AddHeader (bndlHeader);
  mypacket::TypeHeader tHeader (mypacket::MYTYPE_BNDL);
  packet->AddHeader (tHeader);

  if ((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + packet->GetSize()) <= b_s) {
    bool success = m_queue->Enqueue (packet);
    if (success) {
      std::cout << "At time " << Simulator::Now ().GetSeconds () <<
        " send bundle with sequence number " <<  bndlHeader.GetOriginSeqno () <<
        " from " <<  bndlHeader.GetOrigin () <<
        " to " << bndlHeader.GetDst () << "\n";
    }
  } 
  else {
    std::cout << "At time " << Simulator::Now ().GetSeconds () <<
      " tried to send bundle with sequence number " <<  bndlHeader.GetOriginSeqno () <<
      " from " <<  bndlHeader.GetOrigin () <<
      " to " << bndlHeader.GetDst () << "\n";
  }


  // if ((m_queue->GetNBytes() + m_antipacket_queue->GetNBytes() + packet->GetSize()) <= b_s) {
  //   bool success = m_queue->Enqueue (packet);
  //   if (success) {
  //     std::cout << "At time " << Simulator::Now ().GetSeconds () <<
  // " send bundle with sequence number " <<  bndlHeader.GetOriginSeqno () <<
  // " from " <<  bndlHeader.GetOrigin () <<
  // " to " << bndlHeader.GetDst () << "\n";
  //   }
  // } else {
  //   std::cout << "At time " << Simulator::Now ().GetSeconds () <<
  //     " tried to send bundle with sequence number " <<  bndlHeader.GetOriginSeqno () <<
  //     " from " <<  bndlHeader.GetOrigin () <<
  //     " to " << bndlHeader.GetDst () << "\n";
  // }
}

  //check if may # of bundles = bundleSize, gawa ng bundle 

class Mobile: public DtnApp {
public:
  void MobileSetup(Ptr<Node> node);
};


void Mobile::MobileSetup (Ptr<Node> node){
  m_node = node;
  m_antipacket_queue = CreateObject<DropTailQueue> ();
  m_queue = CreateObject<DropTailQueue> ();
  m_helper_queue = CreateObject<DropTailQueue> ();
  m_antipacket_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  m_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  m_helper_queue->SetAttribute ("MaxPackets", UintegerValue (1000));
  stationary = 0;
  for(int i = 0; i < 10000; i++) {
    firstSendTime[i] = 0;
    lastSendTime[i] = 0;
    lastTxBytes[i] = 0;
    currentTxBytes[i] = 0;
    totalTxBytes[i] = 0;
    ids[i] = 0;
    retxs[i] = 0;
  }
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
  b_s = 1375000 + y->GetInteger(0, 1)*9625000;
}





class DtnExample {
public:
  DtnExample ();
  bool Configure (int argc, char **argv);
  void Run ();
  void Report (std::ostream & os);
  std::string traceFile;
  std::string logFile;
  std::ofstream myos;
  std::ifstream bufferInput;

private:
  uint32_t seed;
  uint32_t nodeNum;
  double duration;
  bool pcap;
  bool printRoutes;
  
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  
private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
  void PopulateArpCache ();
};

int main (int argc, char **argv)
{
  //LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);
  DtnExample test;
  if (! test.Configure(argc, argv)) 
    NS_FATAL_ERROR ("Configuration failed. Aborted.");
  
  test.Run ();
  test.Report (std::cout);
  return 0;
}

DtnExample::DtnExample () :
  seed (1),
  nodeNum (116),
  duration (3600),
  pcap (false),
  printRoutes (true)
{
}

bool DtnExample::Configure (int argc, char **argv){
  CommandLine cmd;

  cmd.AddValue ("seed", "RNG seed.", seed);
  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("nodeNum", "Number of nodes.", nodeNum);
  cmd.AddValue ("duration", "Simulation time, s.", duration);
  cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);
  cmd.AddValue ("logFile", "Log file", logFile);
  
  cmd.Parse (argc, argv);
  SeedManager::SetSeed(seed); 
  return true;
}

void DtnExample::Run (){
  Config::SetDefault ("ns3::ArpCache::WaitReplyTimeout", StringValue ("100000000ns")); // 0.1 s, default: 1.0 s
  Config::SetDefault ("ns3::ArpCache::MaxRetries", UintegerValue (10)); // default: 3
  Config::SetDefault ("ns3::ArpCache::AliveTimeout", StringValue ("5000000000000ns")); // 5000 s, default: 120 s
  CreateNodes ();
  // std::cout <<"CN\n";
  CreateDevices ();
  // std::cout <<"CD\n";
  InstallInternetStack ();
  // std::cout <<"after IIS\n";
  InstallApplications ();
  // std::cout <<"after IA\n";
  PopulateArpCache ();
  // std::cout <<"YESPopu arp cache \n";
  
  std::cout << "Starting simulation for " << duration << " s, " <<
    "seed value " << seed << "\n";
  
  Simulator::Stop (Seconds (duration));
  // std::cout <<"STOP\n";
  AnimationInterface anim ("animDTN.xml");
  // std::cout <<"RUN\n";
  Simulator::Run ();
  myos.close (); // close log file
  Simulator::Destroy ();
  // std::cout <<"DEST\n";

}

void DtnExample::Report (std::ostream &){ 
}

void DtnExample::CreateNodes (){
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
  myos.open (logFile.c_str ());
  std::cout << "Creating " << nodeNum << " nodes.\n";
  nodes.Create (nodeNum);
  // Name nodes
  for (uint32_t i = 0; i < nodeNum; ++i) {
    std::ostringstream os;
    os << "node-" << i;
    Names::Add (os.str (), nodes.Get (i));
  }
  ns2.Install ();
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
  MakeBoundCallback (&CourseChange, &myos));
}

void DtnExample::CreateDevices (){
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue ("ErpOfdmRate6Mbps"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());
  QosWifiMacHelper wifiMac = QosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1) ); // default: 1
  wifiPhy.Set ("TxPowerStart",DoubleValue (12.5)); // default: 16.0206
  wifiPhy.Set ("TxPowerEnd", DoubleValue (12.5)); // default: 16.0206
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-74.5) ); // default: -96
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-77.5) ); // default: -99
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (7) ); // default: 7
  wifiPhy.Set ("TxGain", DoubleValue (1.0) ); // default: 1.0
  wifiPhy.Set ("RxGain", DoubleValue (1.0) ); // deafult: 1.0
  wifiMac.SetType ("ns3::AdhocWifiMac");
  devices = wifi.Install (wifiPhy, wifiMac, nodes);
  
  if (pcap)
    wifiPhy.EnablePcapAll (std::string ("rtprot"));
}

void DtnExample::InstallInternetStack () {
  Ipv4StaticRoutingHelper rtprot;
  InternetStackHelper stack;
  stack.SetRoutingHelper (rtprot);
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);
  
  if (printRoutes) {
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("rtprot.routes", std::ios::out);
    rtprot.PrintRoutingTableAllAt (Seconds (8), routingStream);
  }
}

void DtnExample::InstallApplications () {
  uint32_t node_num;
  uint32_t numOfEntries;
  uint32_t entrySize;
  float secondsIntervalinput;


  TypeId udp_tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  for (uint32_t i = 0; i < nodeNum; ++i) { 
    if(i!=nodeNum-1){
      Ptr<Stationary> app;
      app = CreateObject<Stationary> ();  
      app->StationarySetup (nodes.Get (i));
      app->destinationNode=2;

      // std::cout << "Opening Stationary Buffer Details"<< " \n";
      // bufferInput.open("/home/dtn14/Documents/workspace/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP/stationaryBufferDetails");
      bufferInput.open("/home/dtn2/ns-allinone-3.22/ns-3.22/examples/DTN_SF_UDP/stationaryBufferDetails");
      if (bufferInput.is_open()){
        while (bufferInput >> node_num >> numOfEntries >> entrySize >> secondsIntervalinput){
          if(node_num==i){
            // app->BufferSetup(numOfEntries, entrySize, secondsIntervalinput);
            app->bufferCount=0;
            app->entryLength = entrySize;
            app->secondsInterval = secondsIntervalinput;
            app->bufferLength = numOfEntries;
          // std::cout<<"seconds interval" <<secondsIntervalinput<<"\n";
          }
        }
      }
      else{
        std::cout<<"Unable to open Stationary Buffer Details\n";
      }
      bufferInput.close();


      nodes.Get (i)->AddApplication (app);
      app->SetStartTime (Seconds (0.5 + 0.00001*i));
      app->SetStopTime (Seconds (5000.));
      Ptr<Socket> dst = Socket::CreateSocket (nodes.Get (i), udp_tid);
      char dststring[1024]="";
      sprintf(dststring,"10.0.0.%d",(i + 1));
      InetSocketAddress dstlocaladdr (Ipv4Address(dststring), 50000);
      dst->Bind(dstlocaladdr);
      dst->SetRecvCallback (MakeCallback (&DtnApp::ReceiveBundle, app));
      
      Ptr<Socket> source = Socket::CreateSocket (nodes.Get (i), udp_tid);
      InetSocketAddress remote (Ipv4Address ("255.255.255.255"), 80);
      source->SetAllowBroadcast (true);
      source->Connect (remote);
      std::cout<< "node "<< i <<" getnode "<< dst->GetNode()<< " dst-> "<< dst <<" "<< dststring<< " source-> " <<source<<"\n";

      app->GenerateData(1);

      Ptr<Socket> recvSink = Socket::CreateSocket (nodes.Get (i), udp_tid);
      InetSocketAddress local (Ipv4Address::GetAny (), 80);
      recvSink->Bind (local);
      recvSink->SetRecvCallback (MakeCallback (&DtnApp::ReceiveHello, app));
    }
    else{
      Ptr<Mobile> app;
      app = CreateObject<Mobile> ();  
      app->MobileSetup (nodes.Get (i));

      nodes.Get (i)->AddApplication (app);
      app->SetStartTime (Seconds (0.5 + 0.00001*i));
      app->SetStopTime (Seconds (5000.));
      Ptr<Socket> dst = Socket::CreateSocket (nodes.Get (i), udp_tid);
      char dststring[1024]="";
      sprintf(dststring,"10.0.0.%d",(i + 1));
      InetSocketAddress dstlocaladdr (Ipv4Address(dststring), 50000);
      dst->Bind(dstlocaladdr);
      dst->SetRecvCallback (MakeCallback (&DtnApp::ReceiveBundle, app));
      
      Ptr<Socket> source = Socket::CreateSocket (nodes.Get (i), udp_tid);
      InetSocketAddress remote (Ipv4Address ("255.255.255.255"), 80);
      source->SetAllowBroadcast (true);
      source->Connect (remote);
      std::cout<< "node "<< i <<" getnode "<< dst->GetNode()<< " dst-> "<< dst <<" "<< dststring<< " source-> " <<source<<"\n";
      
      std::cout<<"node "<<i<<" is not stationary 0 = stationary value\n";
      app->SendHello (source, duration, Seconds (0.1 + 0.00085*i), 1);
    
      Ptr<Socket> recvSink = Socket::CreateSocket (nodes.Get (i), udp_tid);
      InetSocketAddress local (Ipv4Address::GetAny (), 80);
      recvSink->Bind (local);
      recvSink->SetRecvCallback (MakeCallback (&DtnApp::ReceiveHello, app));

    }
  }
}

void DtnExample::PopulateArpCache () { 
  Ptr<ArpCache> arp = CreateObject<ArpCache> (); 
  arp->SetAliveTimeout (Seconds(3600 * 24 * 365)); 
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) { 
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> (); 
    NS_ASSERT(ip !=0); 
    ObjectVectorValue interfaces; 
    ip->GetAttribute("InterfaceList", interfaces); 
    for(uint32_t j = 0; j != ip->GetNInterfaces (); j ++) {
      Ptr<Ipv4Interface> ipIface = ip->GetInterface (j);
      NS_ASSERT(ipIface != 0); 
      Ptr<NetDevice> device = ipIface->GetDevice(); 
      NS_ASSERT(device != 0); 
      Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ()); 
      for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++) { 
        Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal(); 
        if(ipAddr == Ipv4Address::GetLoopback()) 
          continue; 
        ArpCache::Entry * entry = arp->Add(ipAddr); 
        entry->MarkWaitReply(0); 
        entry->MarkAlive(addr); 
      } 
    } 
  } 
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) { 
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> (); 
    NS_ASSERT(ip !=0); 
    ObjectVectorValue interfaces; 
    ip->GetAttribute("InterfaceList", interfaces);
    for(uint32_t j = 0; j != ip->GetNInterfaces (); j ++) {
      Ptr<Ipv4Interface> ipIface = ip->GetInterface (j);
      ipIface->SetAttribute("ArpCache", PointerValue(arp)); 
    } 
  } 
}
