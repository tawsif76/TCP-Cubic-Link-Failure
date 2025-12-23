#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/string.h"
#include "ns3/ndnSIM/utils/topology/annotated-topology-reader.hpp"
#include "tcp-consumer-app.h"
#include "tcp-producer-app.h"
#include "ns3/ipv4-header.h"
#include <fstream>
#include <vector>
#include <map>

NS_LOG_COMPONENT_DEFINE("TcpBaseline");

namespace ns3 {


std::ofstream g_rawDelayLog;   
std::ofstream g_hopLog;     
std::ofstream g_cwndLog;

uint32_t g_totalTx = 0;        
uint32_t g_totalRx = 0;        
uint32_t g_intervalTx = 0;
uint32_t g_intervalRx = 0;


void TxTrace(Ptr<const Packet> p) {
    g_intervalTx++;
    g_totalTx++;
}

void OnPacketArrival(uint32_t nodeId, Time delay) {
    g_intervalRx++;
    g_totalRx++;
    
    g_rawDelayLog << Simulator::Now().GetSeconds() << "," 
                  << nodeId << "," 
                  << delay.GetSeconds() << "\n";
}


void Ipv4RxTrace(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface) {
    Ipv4Header ipHeader;
    packet->PeekHeader(ipHeader);
    uint8_t ttl = ipHeader.GetTtl();
    
    if (ttl < 64) {
        uint32_t hops = 64 - ttl;
    
        g_hopLog << Simulator::Now().GetSeconds() << "," << hops << "\n";
    }
}


void DownInterface(Ptr<Node> node, uint32_t ifIndex) {
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
  NS_LOG_UNCOND("[CUT] Bringing DOWN interface " << ifIndex
                  << " of " << Names::FindName(node) << " at " << Simulator::Now().GetSeconds());
  ipv4->SetDown(ifIndex);
}

std::vector<uint32_t> GetGatewayInterfacesToCut(Ptr<Node> producerNode, AnnotatedTopologyReader &reader) {
    std::string prodName = Names::FindName(producerNode);
    std::vector<uint32_t> ifaceList;

    Ptr<Ipv4> ipv4 = producerNode->GetObject<Ipv4>();

    for (auto it = reader.LinksBegin(); it != reader.LinksEnd(); ++it) {
        Ptr<Node> a = it->GetFromNode();
        Ptr<Node> b = it->GetToNode();

        std::string nameA = Names::FindName(a);
        std::string nameB = Names::FindName(b);

        bool isGwBb =
            (nameA.rfind("gw-", 0) == 0 && nameB == prodName) ||
            (nameB.rfind("gw-", 0) == 0 && nameA == prodName);

        if (isGwBb) {
            Ptr<Node> gw = (nameA.rfind("gw-", 0) == 0) ? a : b;
            Ptr<Node> bb = producerNode;

            for (uint32_t i = 0; i < bb->GetNDevices(); i++) {
                Ptr<NetDevice> bbDev = bb->GetDevice(i);
                Ptr<Channel> channel = bbDev->GetChannel();
                if (channel == nullptr) continue;

                for (std::size_t j = 0; j < channel->GetNDevices(); ++j) {
                    Ptr<NetDevice> remoteDev = channel->GetDevice(j);
                    if (remoteDev->GetNode() == gw) {
                        int32_t interfaceIndex = ipv4->GetInterfaceForDevice(bbDev);
                        if (interfaceIndex != -1) {
                             ifaceList.push_back(static_cast<uint32_t>(interfaceIndex));
                        }
                        goto next_link;
                    }
                }
            }
        }
        next_link:;
    }
    return ifaceList;
}


void RxTrace(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet) {
    // Format: Time,PacketBytes
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << packet->GetSize() << std::endl;
}

int main (int argc, char* argv[]) {
  std::string topoFile = "scratch/TCP-Fibre-Cut/4755.r0-clients3-conv-annotated.txt";
  double stopTime = 20.0;
  bool fiberCut = true;

  double cutTime = 12.0;
  double percentClients = 0.05; 

  LogComponentEnable("TcpConsumerApp", LOG_LEVEL_INFO);
  LogComponentEnable("TcpProducerApp", LOG_LEVEL_INFO);

  CommandLine cmd;
  cmd.AddValue("topo", "Input Rocketfuel annotated topology file", topoFile);
  cmd.AddValue("stop", "Simulation stop time", stopTime);
  cmd.AddValue("cut", "Enable fiber cut event", fiberCut);
  cmd.AddValue("cutTime", "Time to cut a backbone link", cutTime);
  cmd.AddValue("percentClients", "Fraction of clients to activate", percentClients);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::PfifoFastQueueDisc::MaxSize", StringValue("10p"));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));

  AnnotatedTopologyReader reader("", 1.0);
  reader.SetFileName(topoFile);
  NodeContainer nodes = reader.Read();

  std::vector<Ptr<Node>> clientNodes;
  std::vector<Ptr<Node>> producerCan;

  for (auto it = nodes.Begin(); it != nodes.End(); ++it) {
    Ptr<Node> node = *it;
    std::string name = Names::FindName(node);
    if (name.find("leaf-") == 0) {
      clientNodes.push_back(node);
    } else if (name.find("bb-") == 0) {
      producerCan.push_back(node);
    }
  }

  std::map<Ptr<Node>, uint32_t> degree;
  for (auto it = reader.LinksBegin(); it != reader.LinksEnd(); ++it) {
    degree[it->GetFromNode()]++;
    degree[it->GetToNode()]++;
  }
  std::sort(producerCan.begin(), producerCan.end(),
    [&](Ptr<Node> A, Ptr<Node> B) {
      return degree[A] > degree[B];
    });

  Ptr<Node> producerNode = producerCan[0];
  std::cerr << "Prod Name " << Names::FindName(producerNode) << "\n";


  InternetStackHelper stack;
  stack.InstallAll();

  Ipv4AddressHelper ipv4;
  reader.AssignIpv4Addresses("10.0.0.0");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();



  Ptr<TcpProducerApp> producer = CreateObject<TcpProducerApp>();
  producer->Setup(1024); 
  producerNode->AddApplication(producer);
  producer->SetStartTime(Seconds(0.1));
  producer->SetStopTime(Seconds(stopTime));


  producer->TraceConnectWithoutContext("InterestDelay", 
                                       MakeBoundCallback(&OnPacketArrival, producerNode->GetId()));

  
  std::string prefix = fiberCut ? "CUT-" : "";
  AsciiTraceHelper asciiTraceHelper;

  Ptr<OutputStreamWrapper> throughputStream = asciiTraceHelper.CreateFileStream("scratch/TCP-Fibre-Cut/"+prefix+"tcp-throughput.csv");
  *throughputStream->GetStream() << "Time,Bytes" << std::endl;

  uint32_t totalContents = 10000;
  double alpha = 0.8;
  
  int activeClients = 0;
  int totalPossibleClients = clientNodes.size();
  int limitClients = totalPossibleClients * percentClients;

  int jitter = 0;
  for (Ptr<Node> client : clientNodes) {

    if (activeClients >= limitClients) break;
    activeClients++;

    Ptr<TcpConsumerApp> consumer = CreateObject<TcpConsumerApp>();

    Ptr<Ipv4> prodIP = producerNode->GetObject<Ipv4>();
    Ipv4InterfaceAddress iaddr = prodIP->GetAddress(1, 0);
    Ipv4Address ipAddr = iaddr.GetLocal();

    InetSocketAddress remote(ipAddr, 9000);
    consumer->Setup(Address(remote), totalContents, alpha);

    client->AddApplication(consumer);
    
    consumer->SetStartTime(Seconds(1.0 + (jitter * 0.01)));
    ++jitter;
    consumer->SetStopTime(Seconds(stopTime));
    Ptr<Ipv4> ipv4 = client->GetObject<Ipv4>();
    ipv4->TraceConnectWithoutContext("Rx", MakeCallback(&Ipv4RxTrace));

    consumer->TraceConnectWithoutContext("Tx", MakeCallback(&TxTrace));
    consumer->TraceConnectWithoutContext("LastDelay", 
                                        MakeBoundCallback(&OnPacketArrival, client->GetId()));

    for (uint32_t i = 0; i < client->GetNDevices(); ++i) {
        Ptr<NetDevice> dev = client->GetDevice(i);

        if (dev->GetInstanceTypeId() == PointToPointNetDevice::GetTypeId()) {
            dev->TraceConnectWithoutContext("MacRx", MakeBoundCallback(&RxTrace, throughputStream));
        }
    }
  }

  std::cout << "Active Clients: " << activeClients << " out of " << totalPossibleClients << std::endl;

 
  if (fiberCut) {
    std::vector<uint32_t> gwIfaces = GetGatewayInterfacesToCut(producerNode, reader);
    uint32_t cuts = std::max<uint32_t>(0, (int32_t)gwIfaces.size());

    for (uint32_t k = 0; k < cuts; k++) {
        uint32_t iface = gwIfaces[k];
        Simulator::Schedule(Seconds(cutTime), &DownInterface, producerNode, iface);
    }
    Simulator::Schedule(Seconds(cutTime + 0.01), &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
  }


    
    g_rawDelayLog.open("scratch/TCP-Fibre-Cut/" + prefix + "tcp-all-delays.csv");
    g_rawDelayLog << "Time,NodeId,Delay\n";

    g_hopLog.open("scratch/TCP-Fibre-Cut/" + prefix + "tcp-hop-count.csv");
    g_hopLog << "Time,Hops\n";

    g_cwndLog.open("scratch/TCP-Fibre-Cut/" + prefix + "cwnd-trace.csv");
    g_cwndLog << "Time,NodeId,CWND\n";


  Simulator::Stop(Seconds(stopTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} 

int main(int argc, char* argv[]) {
  return ns3::main(argc, argv);
}