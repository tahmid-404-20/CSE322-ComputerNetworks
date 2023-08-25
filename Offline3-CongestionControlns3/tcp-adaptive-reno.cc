/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
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
 *
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>,
 *          Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 *          Greeshma Umapathi
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "tcp-adaptive-reno.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE("TcpAdaptiveReno");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TcpAdaptiveReno);

TypeId
TcpAdaptiveReno::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpAdaptiveReno")
            .SetParent<TcpWestwoodPlus>()
            .SetGroupName("Internet")
            .AddConstructor<TcpAdaptiveReno>();
    return tid;
}

TcpAdaptiveReno::TcpAdaptiveReno()
    : TcpWestwoodPlus(),
    //   m_currentBW(0),
    //   m_lastSampleBW(0),
    //   m_lastBW(0),
    //   m_ackedSegments(0),
    //   m_IsCount(false),
    //   m_lastAck(0),
      m_minRTT(Time(0)),
      m_currentRTT(Time(0)),
      m_lastCongRTT(Time(0)),
      m_congRTT(Time(0)),
      m_Wprobe(0),
      m_Wbase(0)
{
    NS_LOG_FUNCTION(this);
}

TcpAdaptiveReno::TcpAdaptiveReno(const TcpAdaptiveReno& sock)
    : TcpWestwoodPlus(sock),
    //   m_currentBW(sock.m_currentBW),
    //   m_lastSampleBW(sock.m_lastSampleBW),
    //   m_lastBW(sock.m_lastBW),
    //   m_fType(sock.m_fType),
    //   m_IsCount(sock.m_IsCount)
    m_minRTT(sock.m_minRTT),
    m_currentRTT(sock.m_currentRTT),
    m_lastCongRTT(sock.m_lastCongRTT),
    m_congRTT(sock.m_congRTT),
    m_Wprobe(sock.m_Wprobe),
    m_Wbase(sock.m_Wbase)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Invoked the copy constructor");
}

TcpAdaptiveReno::~TcpAdaptiveReno()
{
}

void
TcpAdaptiveReno::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << packetsAcked << rtt);

    if (rtt.IsZero())
    {
        NS_LOG_WARN("RTT measured is zero!");
        return;
    }

    m_currentRTT = rtt;

    if(m_minRTT.IsZero() || m_minRTT > rtt)
        m_minRTT = rtt;

    m_ackedSegments += packetsAcked;
    TcpWestwoodPlus::EstimateBW(rtt, tcb);
}

void
TcpAdaptiveReno::UpdateCongRTT(void) {
    // jth loss is currentRTT since after 3 times ACK, it is considered a drop
    if (m_lastCongRTT.IsZero()) {   // first packet dropped
        m_congRTT = m_currentRTT;
    } else {
        double a = 0.85;
        m_congRTT = Seconds(a * m_lastCongRTT.GetSeconds() + (1 - a) * m_currentRTT.GetSeconds());
    }

    m_lastCongRTT = m_congRTT;
}

double
TcpAdaptiveReno::EstimateCongestionLevel() {
    // std::cout << "EstimateCongestionLevel  ---  minRTT: " << m_minRTT << " currentRTT: " << m_currentRTT << " congRTT: " << m_congRTT << std::endl;
    return std::min(((m_currentRTT.GetSeconds() - m_minRTT.GetSeconds()) / (m_congRTT.GetSeconds() - m_minRTT.GetSeconds())), 1.0);   
}

double
TcpAdaptiveReno::EstimateIncWnd(Ptr<TcpSocketState> tcb) {
    double M = 1000.0;
    double B = m_currentBWDouble / 8.0;   // MSS in bytes, so keeping all in Bytes    
    double Winc_max = B * static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / M; 

    double alpha = 10.0;
    double beta = 2 * Winc_max * ((1 / alpha) - ((1 / alpha + 1) / std::exp(alpha)));
    double gamma = 1 - (2 * Winc_max * ((1 / alpha) - ((1 / alpha + 1.0/2.0) / std::exp(alpha))));

    double c = EstimateCongestionLevel();

    // std::cout << "Congestion avoidance  --  Congestion Level: " << c << " cWnd: " << tcb->m_cWnd.Get() << std::endl;

    return ( Winc_max / std::exp(c * alpha) + c * beta + gamma);  
}

void
TcpAdaptiveReno::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked > 0)
    {
        double adder =
            static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();  // 1 MSS/W
        adder = std::max(1.0, adder);
        
        if(m_Wbase == 0) {   // first time, as Wbase follows TCP-Reno
            m_Wbase = tcb->m_cWnd.Get();
        }

        m_Wbase += static_cast<uint32_t>(adder);
        double Winc = EstimateIncWnd(tcb);
        double probe = static_cast<double>(m_Wprobe) + Winc / static_cast<double>(tcb->m_cWnd.Get());
        m_Wprobe = static_cast<uint32_t>(std::max(probe, 0.0));

        tcb->m_cWnd = m_Wbase + m_Wprobe;
        // NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                    //  << tcb->m_ssThresh);
    }
}

uint32_t
TcpAdaptiveReno::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight [[maybe_unused]])
{   
    UpdateCongRTT();
    double c = EstimateCongestionLevel();

    // std::cout << "GetSsThresh  --- Congestion Level: " << c << " cWnd: " << tcb->m_cWnd.Get();

    m_Wbase = static_cast<uint32_t>((static_cast<double>(tcb->m_cWnd.Get())) / (1 + c));
    m_Wprobe = 0;
    uint32_t ssThresh = m_Wbase;
    // std::cout << " SsThresh: " << ssThresh << std::endl;
    NS_LOG_LOGIC("CurrentBW: " << m_currentBW << " minRtt: " << tcb->m_minRtt
                               << " ssThresh: " << ssThresh);

    return std::max(2 * tcb->m_segmentSize, ssThresh);
}

Ptr<TcpCongestionOps>
TcpAdaptiveReno::Fork()
{
    return CreateObject<TcpAdaptiveReno>(*this);
}

} // namespace ns3
