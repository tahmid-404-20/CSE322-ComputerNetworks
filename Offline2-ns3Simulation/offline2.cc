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

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nCsma = 3;
    uint32_t nWifi = 3;
    bool tracing = true;

    bool debug = true;

    uint32_t packetSize = 1024;

    uint32_t nNodes = 20;
    uint32_t nFlows = 10;
    uint32_t nPacketsPerSecond = 100;
    uint32_t speed = 5;   // in m/s
    uint32_t coverageMutliplier = 1;  // in multiple of Tx_Range

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

    uint32_t nSenders;
    uint32_t nReceivers;
    nSenders = nReceivers = nNodes / 2;


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
    // modification of coverage area
    uint32_t Tx_RangeDefault = 5;
    uint32_t coverageArea = Tx_RangeDefault * coverageMutliplier;
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

    double senderYMax = 0.8 * coverageArea;
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

    std::ostringstream modelSpeed;
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

    for(int i=0; i < nSenders; i++) {
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(receiverNodes.Get(i));
        // sink = StaticCast<PacketSink>(sinkApp.Get(0));

        OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(receiverInterfaces.GetAddress(0), port)));
        onoff.SetConstantRate(DataRate(packetSize * 8 * nPacketsPerSecond)); // Set the total data rate in bps
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        ApplicationContainer senderApp = onoff.Install(senderNodes.Get(i));

        sinkApp.Start(Seconds(0.0));
        senderApp.Start(Seconds(1.0));
    }
    


    // Application
    // UdpEchoServerHelper echoServer(9);

    // for(int i=0;i < nSenders; i++) {
    //     ApplicationContainer serverApps = echoServer.Install(receiverNodes.Get(i));
    //     serverApps.Start(Seconds(1.0));
    //     serverApps.Stop(Seconds(10.0));

    //     UdpEchoClientHelper echoClient(receiverInterfaces.GetAddress(i), 9);
    //     echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    //     echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    //     echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    //     ApplicationContainer clientApps = echoClient.Install(senderNodes.Get(i));
    //     clientApps.Start(Seconds(2.0));
    //     clientApps.Stop(Seconds(10.0));

    // }
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // for(int i=0;i<nReceivers;i++) {
    //     std::ostringstream oss;
    //     oss << "/NodeList/" << receiverNodes.Get(i)->GetId() << "/$ns3::MobilityModel/$ns3::ConstantVelocityMobilityModel/CourseChange";
    //     Config::Connect(oss.str(), MakeCallback(&CourseChange)); // connects trace source to trace sink
    // }
    






    // NodeContainer wifiStaNodes;
    // wifiStaNodes.Create(nWifi);
    // NodeContainer wifiApNode = p2pNodes.Get(0);

    // // Physical Layer
    // // YANS model - Yet Another Network Simulator
    // YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    // YansWifiPhyHelper phy;
    // phy.Set("TxPowerStart", DoubleValue(20.0)); // Set the initial transmission power in dBm
    // phy.Set("TxPowerEnd", DoubleValue(20.0));   // Set the final transmission power in dBm
    // phy.Set("TxGain", DoubleValue(0.0));        // Set the transmission gain in dB
    // phy.Set("RxGain", DoubleValue(0.0));        // Set the reception gain in dB


    // // Set the desired coverage area (transmission range)
    // double coverageArea = 100.0; // Set your desired coverage area in meters
    // phy.Set("TxRange", DoubleValue(coverageArea));
    // phy.SetChannel(channel.Create()); // share the same wireless medium 

    // // Data Link Layer
    // // SSid used to set the "ssid" Attribute in the mac layer implementation
    // // The (SSID - Service Set IDentifier) is the network name used to logically 
    // // identify the wireless network. 
    // // Each network will have a single SSID that identifies the network, 
    // // and this name will be used by clients to connect to the network.
    // WifiMacHelper mac;
    // Ssid ssid = Ssid("ns-3-ssid"); // creates an 802.11 service set identifier (SSID) 

    // WifiHelper wifi;

    // // ActiveProbing false -  probe requests will not be sent by MACs created by this
    // // helper, and stations will listen for AP beacons.
    // NetDeviceContainer staDevices;
    // mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    // staDevices = wifi.Install(phy, mac, wifiStaNodes);

    // NetDeviceContainer apDevices;
    // mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    // apDevices = wifi.Install(phy, mac, wifiApNode);

    // MobilityHelper mobility;

    // mobility.SetPositionAllocator("ns3::GridPositionAllocator",
    //                               "MinX",
    //                               DoubleValue(0.0),
    //                               "MinY",
    //                               DoubleValue(0.0),
    //                               "DeltaX",
    //                               DoubleValue(5.0),
    //                               "DeltaY",
    //                               DoubleValue(10.0),
    //                               "GridWidth",
    //                               UintegerValue(4),
    //                               "LayoutType",
    //                               StringValue("RowFirst"));

    // mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    //                           "Bounds",
    //                           RectangleValue(Rectangle(-50, 50, -50, 50)));
    // mobility.Install(wifiStaNodes);

    // // To get a mobility model and print their position
    // // Ptr<MobilityModel> mob = wifiStaNodes.Get(2)->GetObject<MobilityModel>();
    // // std::cout << mob->GetPosition().x << " " << mob->GetPosition().y << std::endl;

    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    // mobility.Install(wifiApNode);

    // // Ptr<MobilityModel> mob2 = wifiApNode.Get(0)->GetObject<MobilityModel>();
    // // std::cout << mob2->GetPosition().x << " " << mob2->GetPosition().y << std::endl;

    // InternetStackHelper stack;
    // stack.Install(csmaNodes);
    // stack.Install(wifiApNode);
    // stack.Install(wifiStaNodes);

    // Ipv4AddressHelper address;

    // address.SetBase("10.1.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer p2pInterfaces;
    // p2pInterfaces = address.Assign(p2pDevices);

    // address.SetBase("10.1.2.0", "255.255.255.0");
    // Ipv4InterfaceContainer csmaInterfaces;
    // csmaInterfaces = address.Assign(csmaDevices);

    // address.SetBase("10.1.3.0", "255.255.255.0");
    // address.Assign(staDevices);
    // address.Assign(apDevices);

    // UdpEchoServerHelper echoServer(9);

    // ApplicationContainer serverApps = echoServer.Install(csmaNodes.Get(nCsma));
    // serverApps.Start(Seconds(1.0));
    // serverApps.Stop(Seconds(10.0));

    // UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(nCsma), 9);
    // echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    // echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    // echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // ApplicationContainer clientApps = echoClient.Install(wifiStaNodes.Get(nWifi - 1));
    // clientApps.Start(Seconds(2.0));
    // clientApps.Stop(Seconds(10.0));

    // Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Schedule(Seconds(1), &SeePositionChange);
    Simulator::Stop(Seconds(10.0));

    // if (tracing)
    // {
    //     phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    //     pointToPoint.EnablePcapAll("scratch/third/third");
    //     phy.EnablePcap("scratch/third/third", apDevices.Get(0));
    //     phy.EnablePcap("scratch/third/third", staDevices.Get(1));
    //     csma.EnablePcap("scratch/third/third", csmaDevices.Get(0), true);
    // }

    // // // global NodeList - contains all nodes
    // std::ostringstream oss;
    // oss << "/NodeList/" << wifiStaNodes.Get(nWifi - 1)->GetId()
    // << "/$ns3::MobilityModel/CourseChange";
    // Config::Connect(oss.str(), MakeCallback(&CourseChange)); // connects trace source to trace sink


    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
