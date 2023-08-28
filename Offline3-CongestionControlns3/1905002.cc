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
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/flow-monitor-module.h"

// Default Network Topology
//      
//       10.1.1.0                           10.1.3.0
//         (p2p)           10.1.2.0            (p2p)                
//  s0                                                 r0
//                  bn0 ------------- bn1                 
//  s1                Point-to-Point                   r1



using namespace ns3;

#ifndef TUTORIAL_APP_H
#define TUTORIAL_APP_H


namespace ns3
{

class Application;

/**
 * Tutorial - a simple Application sending packets.
 */
class TestApp : public Application
{
  public:
    TestApp();
    ~TestApp() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Setup the socket.
     * \param socket The socket.
     * \param address The destination address.
     * \param packetSize The packet size to transmit.
     * \param nPackets The number of packets to transmit.
     * \param dataRate the data rate to use.
     */
    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               uint32_t nPackets,
               DataRate dataRate);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /// Schedule a new transmission.
    void ScheduleTx();
    /// Send a packet.
    void SendPacket();

    Ptr<Socket> m_socket;   //!< The transmission socket.
    Address m_peer;         //!< The destination address.
    uint32_t m_packetSize;  //!< The packet size.
    uint32_t m_nPackets;    //!< The number of packets to send.
    DataRate m_dataRate;    //!< The data rate to use.
    EventId m_sendEvent;    //!< Send event.
    bool m_running;         //!< True if the application is running.
    uint32_t m_packetsSent; //!< The number of packets sent.
};

} // namespace ns3

#endif /* TUTORIAL_APP_H */

using namespace ns3;

TestApp::TestApp()
    : m_socket(nullptr),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

TestApp::~TestApp()
{
    m_socket = nullptr;
}

/* static */
TypeId
TestApp::GetTypeId()
{
    static TypeId tid = TypeId("TestApp")
                            .SetParent<Application>()
                            .SetGroupName("Tutorial")
                            .AddConstructor<TestApp>();
    return tid;
}

void
TestApp::Setup(Ptr<Socket> socket,
                   Address address,
                   uint32_t packetSize,
                   uint32_t nPackets,
                   DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void
TestApp::StartApplication()
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void
TestApp::StopApplication()
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
TestApp::SendPacket()
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    // if (++m_packetsSent < m_nPackets) --- removed this line coz we want to send packets continuously
    // {
        ScheduleTx();
    // }
}

void
TestApp::ScheduleTx()
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &TestApp::SendPacket, this);
    }
}


NS_LOG_COMPONENT_DEFINE("Offline3-Task1");

// trace sources
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  // NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << " " << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;
}


int
main(int argc, char* argv[])
{
    bool verbose = false;
    // bool debug = false;

    uint32_t packetSize = 1024;          // in bytes

    int bottleneckDelay = 100;   // in ms
    double simulatorRunningTime = 10.0; // in seconds
    std::string sendingRate = "1Gbps";

    int bottleneckDataRate = 50; // in Mbps
    int errorExp = 6;            // in exponent of 10
    std::string congestionControl = "ns3::TcpAdaptiveReno";

    CommandLine cmd(__FILE__);
    cmd.AddValue("bRate", "BottleNeckDataRate", bottleneckDataRate);
    cmd.AddValue("errorExp", "10^[-errorExp]", errorExp);
    cmd.AddValue("congAlg", "Congestion Control Algorithm", congestionControl);   

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));

    // setting up bottleneck nodes
    PointToPointHelper pointToPointBottleneck;
    std::ostringstream dataRate;
    dataRate << bottleneckDataRate << "Mbps";
    pointToPointBottleneck.SetDeviceAttribute("DataRate", StringValue(dataRate.str()));
    pointToPointBottleneck.SetChannelAttribute("Delay", StringValue("100ms"));

    //  setting up sender-receiver nodes 
    uint64_t bandwidth_delay_product = (bottleneckDataRate * bottleneckDelay * ((1024*1024) / (8*packetSize))) / 1000;  // in #packets, dividing by 100 as we are using ms
    PointToPointHelper p2pSR;
    p2pSR.SetDeviceAttribute ("DataRate", StringValue(sendingRate));
    p2pSR.SetChannelAttribute ("Delay", StringValue("1ms"));
    p2pSR.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string (bandwidth_delay_product) + "p"));
    
    // using dubmbell topology
    PointToPointDumbbellHelper dumbbell(2, p2pSR, 2, p2pSR, pointToPointBottleneck);

    // set error model
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(pow(10, -errorExp)));
    dumbbell.m_routerDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    dumbbell.m_routerDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    // set congestion control
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(congestionControl));
    InternetStackHelper stackFlow1;
    stackFlow1.Install(dumbbell.GetLeft(0));
    stackFlow1.Install(dumbbell.GetRight(0));

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    InternetStackHelper stackFlow2;
    stackFlow2.Install(dumbbell.GetLeft(1));
    stackFlow2.Install(dumbbell.GetRight(1));

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    InternetStackHelper stackBottleNeck;
    stackBottleNeck.Install(dumbbell.GetLeft());
    stackBottleNeck.Install(dumbbell.GetRight());


    // set Ip Addresses
    dumbbell.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"), /* left*/ Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),  /*right*/ 
                        Ipv4AddressHelper("10.3.1.0", "255.255.255.0")); // bottleneck


     // install flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll ();

    uint16_t port = 9;
    for(int i=0;i<2; i++)
    {
      
      // setting sink apps
      Address sinkAddress(InetSocketAddress (dumbbell.GetRightIpv4Address (i), port));
      PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(), port));

      ApplicationContainer sinkApps = packetSinkHelper.Install(dumbbell.GetRight (i));
      sinkApps.Start(Seconds(0));
      sinkApps.Stop(Seconds(simulatorRunningTime));


      // setting sender apps  
      Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(dumbbell.GetLeft(i), TcpSocketFactory::GetTypeId());

      Ptr<TestApp> app = CreateObject<TestApp>();
      app->Setup(ns3TcpSocket, sinkAddress, packetSize, 0, DataRate(sendingRate));
      dumbbell.GetLeft(i)->AddApplication(app);

      app->SetStartTime(Seconds(0.1));
      app->SetStopTime(Seconds(simulatorRunningTime));

      std::ostringstream oss;
      oss << "scratch/offline3/cwnd" << i <<  ".dat";
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream(oss.str());
      ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback(&CwndChange, stream));
    }                           
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    

    Simulator::Stop(Seconds(simulatorRunningTime));
    Simulator::Run();

    double totalBytesReceived = 0;

    // flow1  0(S) ---------->  0(R)
    // flow2  0(S) <----------  0(R)
    // flow3  1(S) ---------->  1(R)    
    // flow4  1(S) <----------  1(R)

    double flow1Throughput = 0.0;
    double flow2Throughput = 0.0;
    double flow3Throughput = 0.0;
    double flow4Throughput = 0.0;

    // for jain's calculation
    double totalThroughput = 0.0;
    double totalThroughputSquare = 0.0;
    int count = 0;

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

        // if source address is left node of flow1, update flow1TotalBytes
        if(t.sourceAddress == dumbbell.GetLeftIpv4Address(0))
        {   
            //std::cout << "Updating flow1TotalBytes" << std::endl;
            flow1Throughput = iter->second.rxBytes * 8.0 / iter->second.timeLastRxPacket.GetSeconds() / 1024;
            totalThroughput += flow1Throughput;
            totalThroughputSquare += flow1Throughput * flow1Throughput;
        }
        else if(t.sourceAddress == dumbbell.GetRightIpv4Address(0))
        {
            //std::cout << "Updating flow2TotalBytes" << std::endl;
            flow2Throughput = iter->second.rxBytes * 8.0 / iter->second.timeLastRxPacket.GetSeconds() / 1024;
            totalThroughput += flow2Throughput;
            totalThroughputSquare += flow2Throughput * flow2Throughput;
        }
        else if(t.sourceAddress == dumbbell.GetLeftIpv4Address(1))
        {
            //std::cout << "Updating flow3TotalBytes" << std::endl;
            flow3Throughput = iter->second.rxBytes * 8.0 / iter->second.timeLastRxPacket.GetSeconds() / 1024;
            totalThroughput += flow3Throughput;
            totalThroughputSquare += flow3Throughput * flow3Throughput;

        }
        
        else if(t.sourceAddress == dumbbell.GetRightIpv4Address(1))
        {
            //std::cout << "Updating flow4TotalBytes" << std::endl;
            flow4Throughput = iter->second.rxBytes * 8.0 / iter->second.timeLastRxPacket.GetSeconds() / 1024;
            totalThroughput += flow4Throughput;
            totalThroughputSquare += flow4Throughput * flow4Throughput;
        }

        // std::cout << "Flow " << iter->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")"
        //         << "  Throughput: " << iter->second.rxBytes * 8.0 / iter->second.timeLastRxPacket.GetSeconds() / 1024 << " Kbps"
        //         << std::endl;

        totalBytesReceived += iter->second.rxBytes;

        count++;
        
    }

    // std::cout << "Average Throughput: " << (totalBytesReceived / (4.0 * 1024 * simulatorRunningTime)) * 8.0 << " Kbps" << std::endl;

    double pair1Throughput = (flow1Throughput + flow2Throughput);
    double pair2Throughput = (flow3Throughput + flow4Throughput);

    // std::cout << "Pair1 Throughput: " << pair1Throughput << " Kbps" << std::endl;
    // std::cout << "Pair2 Throughput: " << pair2Throughput << " Kbps" << std::endl;

    double jainsIndex = (totalThroughput * totalThroughput) / (count * totalThroughputSquare);

    std::cout << pair1Throughput << "," << pair2Throughput << "," << jainsIndex << std::endl;

    Simulator::Destroy();

    return 0;
}
