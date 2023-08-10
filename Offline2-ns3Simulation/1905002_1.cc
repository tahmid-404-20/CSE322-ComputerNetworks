/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//      
//      Wifi 10.1.1.0(S)                   Wifi 10.1.3.0(M)
//  s1                  10.1.2.0                        r1
//  s2              Ap               AP                 r2
//  s3              n0 ------------- n1                 r3
//  s4                 Point-to-Point                   r4
//  s5                 17 Mbps, 2ms                     r5
//  ....                                               ....
//


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Offline2-Static");

uint128_t totalBytesReceived = 0;
uint128_t totalPacketsTransmitted = 0;
uint128_t totalPacketsReceived = 0;
uint32_t packetSize;

double_t networkThroughput = 0.0;

// Trace sources
void 
PacketReceived (Ptr< const Packet > packet, const Address &address)
{   
    totalBytesReceived += packet->GetSize();
    networkThroughput = ((totalBytesReceived/Simulator::Now().GetSeconds())*8)/1e6;   // in Mbps

    totalPacketsReceived += packet->GetSize()/packetSize;    // sometimes, this gives more than 1024
}

void 
PacketTransmitted (Ptr< const Packet > packet)
{   
    totalPacketsTransmitted++;
}


int
main(int argc, char* argv[])
{
    bool verbose = false;
    bool debug = false;

    packetSize = 1024;

    int nNodes = 20;
    int nFlows = 10;
    int nPacketsPerSecond = 100;
    int coverageMultiplier = 2;  // in multiple of Tx_Range

    CommandLine cmd(__FILE__);
    cmd.AddValue("nNodes", "Number of sender-receivers", nNodes);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("nPacketsPerSecond", "Number of packets per second", nPacketsPerSecond);
    cmd.AddValue("coverageMultiplier", "Coverage area multiplier", coverageMultiplier);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    int nSenders;
    int nReceivers;
    nSenders = nReceivers = nNodes / 2;

    // setting up accessPoints and in between p2p connection
    NodeContainer apNodes;
    apNodes.Create(2);    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("17Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(apNodes);

    
    // Sender network Topology setup
    NodeContainer senderNodes;
    senderNodes.Create(nSenders);
    NodeContainer senderApNode = apNodes.Get(0);
    // modification of coverage area
    uint32_t Tx_RangeDefault = 5;
    uint32_t coverageArea = Tx_RangeDefault * coverageMultiplier;
    Config::SetDefault("ns3::RangePropagationLossModel::MaxRange", DoubleValue(coverageArea));

    YansWifiChannelHelper senderChannel = YansWifiChannelHelper::Default();
    senderChannel.AddPropagationLoss("ns3::RangePropagationLossModel");

    YansWifiPhyHelper senderPhy;
    senderPhy.SetChannel(senderChannel.Create());

    WifiMacHelper senderMac;
    Ssid senderSsid = Ssid("Senders-Wifi");
    WifiHelper senderWifi;

    NetDeviceContainer senderDevices;
    senderMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(senderSsid), "ActiveProbing", BooleanValue(false));
    senderDevices = senderWifi.Install(senderPhy, senderMac, senderNodes);

    NetDeviceContainer senderApDevices;
    senderMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(senderSsid));
    senderApDevices = senderWifi.Install(senderPhy, senderMac, senderApNode);

//  *
//  *
//  *
//    ----- SenderAp   
//  *
//  *
//  *

    double senderYMax = 0.96*coverageArea;   
    double senderDeltaY = 2.0*senderYMax / (nSenders-1);
    double senderApX = 1.0;

    Ptr<ListPositionAllocator> senderPositionAlloc = CreateObject<ListPositionAllocator>();    
    double senderY = -senderYMax;
    for(int i=0; i< nSenders; i++) {
        senderPositionAlloc->Add(Vector(0.0, senderY, 0.0));
        senderY += senderDeltaY;
    }
    senderPositionAlloc->Add(Vector(senderApX, 0.0, 0.0));  // senderAp


    // sender mobility model
    MobilityHelper senderMobility;
    senderMobility.SetPositionAllocator(senderPositionAlloc);
    senderMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    senderMobility.Install(senderNodes);
    senderMobility.Install(senderApNode);
    
    if(debug) {
        std::cout << "Sender Positions" << std::endl;
        for(int i=0;i<nSenders;i++) {
            Ptr<MobilityModel> mob = senderNodes.Get(i)->GetObject<MobilityModel>();
            std::cout << mob->GetPosition().x << " " << mob->GetPosition().y << std::endl;
        }

        std::cout << "Sender Ap node position" << std::endl;
        Ptr<MobilityModel> mob2 = senderApNode.Get(0)->GetObject<MobilityModel>();
        std::cout << mob2->GetPosition().x << " " << mob2->GetPosition().y << std::endl;
    }
    


    // Receiver network Topology setup
    NodeContainer receiverNodes;
    receiverNodes.Create(nReceivers);
    NodeContainer receiverApNode = apNodes.Get(1);

    YansWifiChannelHelper receiverChannel = YansWifiChannelHelper::Default();
    receiverChannel.AddPropagationLoss("ns3::RangePropagationLossModel");
    YansWifiPhyHelper receiverPhy;
    receiverPhy.SetChannel(receiverChannel.Create());

    WifiMacHelper receiverMac;
    Ssid receiverSsid = Ssid("Receivers-Wifi");
    WifiHelper receiverWifi;

    NetDeviceContainer receiverDevices;
    receiverMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(receiverSsid), "ActiveProbing", BooleanValue(false));
    receiverDevices = receiverWifi.Install(receiverPhy, receiverMac, receiverNodes);

    NetDeviceContainer receiverApDevices;
    receiverMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(receiverSsid));
    receiverApDevices = receiverWifi.Install(receiverPhy, receiverMac, receiverApNode);
    
//                  *
//                  *
//                  *
//  ReceiverAp -----   
//                  *
//                  *
//                  *

    double receiverYMax = 0.96*coverageArea; 
    double receiverDeltaY = 2.0*receiverYMax / (nReceivers-1);
    double receiverApX = 7.0;

    Ptr<ListPositionAllocator> receiverPositionAlloc = CreateObject<ListPositionAllocator>();    
    double receiverY = -receiverYMax;
    for(int i=0; i< nReceivers; i++) {
        receiverPositionAlloc->Add(Vector(8.0, receiverY, 0.0));
        receiverY += receiverDeltaY;
    }
    receiverPositionAlloc->Add(Vector(receiverApX, 0.0, 0.0));  // receiverAp

    // receiver mobility model
    MobilityHelper receiverMobility;
    receiverMobility.SetPositionAllocator(receiverPositionAlloc);
    receiverMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    receiverMobility.Install(receiverNodes);
    receiverMobility.Install(receiverApNode);

    if(debug) {
        std::cout << "Receiver Positions" << std::endl;
        for(int i=0;i<nSenders;i++) {
            Ptr<MobilityModel> mob = receiverNodes.Get(i)->GetObject<MobilityModel>();
            std::cout << mob->GetPosition().x << " " << mob->GetPosition().y << std::endl;
        }

        std::cout << "Receiver Ap node position" << std::endl;
        Ptr<MobilityModel> mob2 = receiverApNode.Get(0)->GetObject<MobilityModel>();
        std::cout << mob2->GetPosition().x << " " << mob2->GetPosition().y << std::endl;
    }

    // Internet Stack
    InternetStackHelper stack;
    stack.Install(senderNodes);
    stack.Install(receiverNodes);
    stack.Install(apNodes);

    // IP Address
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer senderInterfaces;
    senderInterfaces = address.Assign(senderDevices);
    address.Assign(senderApDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer receiverInterfaces;
    receiverInterfaces = address.Assign(receiverDevices);
    Ipv4InterfaceContainer receiverApInterfaces;
    receiverApInterfaces = address.Assign(receiverApDevices);


    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));

    if(debug) std::cout << "Creating App" << std::endl;
    uint16_t port = 9;
    uint32_t portAddition = 0;
    uint32_t index = 0;

    for(int i=0; i < nFlows; i++) {
        // for installing multiple application in a node
        if(i && i%nReceivers == 0) {
            portAddition++;
        }

        index = i%nReceivers;
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), port + portAddition));
        ApplicationContainer sinkApp = sinkHelper.Install(receiverNodes.Get(index));
        
        Ptr<PacketSink> sink = StaticCast<PacketSink>(sinkApp.Get(0));
        sink->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceived));


        OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(receiverInterfaces.GetAddress(0), port + portAddition)));
        onoff.SetConstantRate(DataRate(packetSize * 8 * nPacketsPerSecond)); // Set the total data rate in bps
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        ApplicationContainer senderApp = onoff.Install(senderNodes.Get(index));
        
        Ptr<OnOffApplication> onoffApp = StaticCast<OnOffApplication>(senderApp.Get(0));
        onoffApp->TraceConnectWithoutContext("Tx", MakeCallback(&PacketTransmitted));

        sinkApp.Start(Seconds(0.0));
        senderApp.Start(Seconds(0.1));
    }

    if(debug) std::cout << "Done creating" << std::endl;

    if(debug) std::cout << "Receiver 22-0 ip: " << receiverInterfaces.GetAddress(0) << std::endl;
    if(debug) std::cout << "Receiver 26-0 ip: " << receiverInterfaces.GetAddress(4) << std::endl;
    if(debug) std::cout << "Receiver Ap ip: " << receiverApInterfaces.GetAddress(0) << std::endl;

    if (verbose)
    {
        receiverPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        receiverPhy.EnablePcap("scratch/offline_static", receiverApDevices.Get(0));
        receiverPhy.EnablePcap("scratch/offline_static", receiverDevices.Get(0));
        receiverPhy.EnablePcap("scratch/offline_static", receiverDevices.Get(4));
        senderPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        senderPhy.EnablePcap("scratch/offline_static", senderApDevices.Get(0));
        senderPhy.EnablePcap("scratch/offline_static", senderDevices.Get(0));
        senderPhy.EnablePcap("scratch/offline_static", senderDevices.Get(4));
    }

    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(10.0));

    AnimationInterface anim("scratch/static.xml");
    anim.SetMaxPktsPerTraceFile(50000000);
    
    Simulator::Run();
    Simulator::Destroy();

    // std::cout << "Total Packets Transmitted: " << (double)totalPacketsTransmitted  << "total received " << (double)totalPacketsReceived << std::endl;

    // std::cout << "\nNetwork Throughput: " << networkThroughput << " Mbit/s Packet Delivery Ratio: " <<
    // ((double)totalPacketsReceived/(double)totalPacketsTransmitted)*100 << std::endl;


    std::cout << networkThroughput << "," << 
    ((double)totalPacketsReceived/(double)totalPacketsTransmitted)*100;

    return 0;
}
