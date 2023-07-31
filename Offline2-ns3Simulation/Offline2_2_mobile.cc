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

// Default Network Topology
//      
//      Wifi 10.1.1.0(S)                   Wifi 10.1.3.0(M)
//  s1                  10.1.2.0                        r1
//  s2              Ap               AP                 r2
//  s3              n0 ------------- n1                 r3
//  s4                 Point-to-Point                   r4
//  s5                 10 Mbps, 2ms                     r5
//  ....                                               ....
//


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Offline2");

void
CourseChange(std::string context, Ptr<const MobilityModel> model)
{
    Vector position = model->GetPosition();
    NS_LOG_UNCOND(context <<
    " x = " << position.x << ", y = " << position.y);
}

Ptr<const RandomWalk2dMobilityModel> testModel;
void
SeePositionChange()
{
    Vector position = testModel->GetPosition();
    NS_LOG_UNCOND("For receiver 0" <<
    " x = " << position.x << ", y = " << position.y);
    Simulator::Schedule(Seconds(1), &SeePositionChange);
}

uint128_t totalBytesReceived = 0;
uint128_t totalPacketsTransmitted = 0;
uint128_t totalPacketsReceived = 0;
uint32_t packetSize;

double_t packetDeliveryRatio = 0.0;
double_t networkThroughput = 0.0;

// Trace sources
void 
PacketReceived (Ptr< const Packet > packet, const Address &address)
{   
    totalBytesReceived += packet->GetSize();
    networkThroughput = ((totalBytesReceived/Simulator::Now().GetSeconds())*8)/1e6;   // in Mbps

    totalPacketsReceived += packet->GetSize()/packetSize;    // sometimes, this gives more than 1024
    packetDeliveryRatio = ((double)totalPacketsReceived/(double)totalPacketsTransmitted)*100;
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
    int speed = 5;   // in m/s
    int coverageMutliplier = 1;  // in multiple of Tx_Range

    CommandLine cmd(__FILE__);
    cmd.AddValue("nNodes", "Number of sender-receivers", nNodes);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("nPacketsPerSecond", "Number of packets per second", nPacketsPerSecond);
    cmd.AddValue("speed", "Speed of sender-receivers", speed);
    cmd.AddValue("coverageMutliplier", "Coverage area multiplier", coverageMutliplier);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    int nSenders;
    int nReceivers;
    nSenders = nReceivers = nNodes / 2;

    if(nNodes > (2*nFlows)) {
        nFlows = nNodes / 2;
    }


    // setting up accessPoints and in between p2p connection
    NodeContainer apNodes;
    apNodes.Create(2);    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(apNodes);

    
    // Sender network Topology setup
    NodeContainer senderNodes;
    senderNodes.Create(nSenders);
    NodeContainer senderApNode = apNodes.Get(0);
    
    YansWifiChannelHelper senderChannel = YansWifiChannelHelper::Default();
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

    MobilityHelper senderMobility;
    senderMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(1.0),
                                  "DeltaY",
                                  DoubleValue(1.0),
                                  "GridWidth",
                                  UintegerValue(4),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    std::ostringstream modelSpeed;
    modelSpeed << "ns3::ConstantRandomVariable[Constant=" << speed << "]";

    senderMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)),
                              "Speed",
                              StringValue(modelSpeed.str()));
    senderMobility.Install(senderNodes);

    senderMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
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
    
    MobilityHelper receiverMobility;
    receiverMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(1.0),
                                  "DeltaY",
                                  DoubleValue(1.0),
                                  "GridWidth",
                                  UintegerValue(4),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    modelSpeed << "ns3::ConstantRandomVariable[Constant=" << speed << "]";

    receiverMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)),
                              "Speed",
                              StringValue(modelSpeed.str()));
    receiverMobility.Install(receiverNodes);

    receiverMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    receiverMobility.Install(receiverApNode);

    // for showing mobility
    testModel = receiverNodes.Get(0)->GetObject<RandomWalk2dMobilityModel>();

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
    address.Assign(receiverApDevices);


    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));

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
        senderApp.Start(Seconds(1.0));
    }
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    if(debug) {
        Simulator::Schedule(Seconds(1), &SeePositionChange);
    }
    Simulator::Stop(Seconds(10.0));


    Simulator::Run();
    Simulator::Destroy();

    std::cout << "\nNetwork Throughput: " << networkThroughput << " Mbit/s Packet Delivery Ratio: " << packetDeliveryRatio << std::endl;

    return 0;
}
