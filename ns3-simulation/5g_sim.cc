#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

#include <iomanip>
#include <ios>
#include <map>
#include <string>
#include <vector>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NRSimpleScenario");


// Callback for UE connection established
void 
NotifyConnectionEstablishedUe (std::string context, 
                                uint64_t imsi, 
                                uint16_t cellId, 
                                uint16_t rnti)
{
    std::cout << Simulator::Now().GetSeconds() << " " << context
              << " UE IMSI " << imsi
              << ": connected to CellId " << cellId
              << " with RNTI " << rnti 
              << std::endl;
}


// Callback for UE connection released
void 
NotifyConnectionReleasedUe (std::string context, 
                            uint64_t imsi, 
                            uint16_t cellId, 
                            uint16_t rnti)
{
    std::cout << Simulator::Now().GetSeconds() << " " << context
              << " UE IMSI " << imsi
              << ": released from CellId " << cellId
              << " with RNTI " << rnti 
              << std::endl;
}

// Callback for handover start at gNB
void 
NotifyHandoverStartGnb (std::string context, 
                        uint64_t imsi, 
                        uint16_t cellId, 
                        uint16_t rnti, 
                        uint16_t targetCellId)
{
    std::cout << Simulator::Now().GetSeconds() << " " << context
              << " gNB CellId " << cellId
              << ": start handover of UE with IMSI " << imsi
              << " RNTI " << rnti
              << " to CellId " << targetCellId 
              << std::endl;
}

// Callback for handover end at gNB
void 
NotifyHandoverEndOkGnb (std::string context, 
                        uint64_t imsi, 
                        uint16_t cellId, 
                        uint16_t rnti)
{
    std::cout << Simulator::Now().GetSeconds() << " " << context
              << " gNB CellId " << cellId
              << ": completed handover of UE with IMSI " << imsi
              << " RNTI " << rnti 
              << std::endl;
}


// Callback for RSRP and SINR measurements
void RsrpSinrTrace(std::string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr, bool isServingCell, uint8_t componentCarrierId)
{
    std::cout << "[RSRP/SINR] " << context << " CellId=" << cellId << " RNTI=" << rnti
              << " RSRP=" << rsrp << " SINR=" << sinr << " ServingCell=" << isServingCell
              << " CCId=" << (int)componentCarrierId << std::endl;
}

// Callback for CQI measurements
void CqiTrace(std::string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr, bool isServingCell, uint8_t componentCarrierId)
{
    std::cout << "[CQI] " << context << " CellId=" << cellId << " RNTI=" << rnti
              << " RSRP=" << rsrp << " SINR=" << sinr << " ServingCell=" << isServingCell
              << " CCId=" << (int)componentCarrierId << std::endl;
}

// Callback for PRB utilization
void PrbUtilTrace(std::string context, uint64_t imsi, uint64_t size)
{
    std::cout << "[PRB Util] " << context << " IMSI=" << imsi << " Size=" << size << std::endl;
}

// Add QoS-related trace callback
//void BufferStatusTrace(std::string context, uint32_t bufferSize)
//{
//    std::cout << "[Buffer Status] " << context 
//              << " Buffer Size: " << bufferSize << " bytes" << std::endl;
//}


// holds number of sinks
uint32_t numSinks;
// pointer to the sinks
ApplicationContainer sinks;
// holds last amount of received bytes for all the clients
std::vector<int> lastTotalRx;

void CalculateThroughput (uint16_t inter)
    {
      Time now = Simulator::Now (); /* Return the simulator's virtual time. */

      for (uint32_t u = 0; u < numSinks; ++u) {

      Ptr<PacketSink> sink = StaticCast<PacketSink> (sinks.Get (u));
      uint32_t idd = ((sinks.Get(u))->GetNode())->GetId();
      double curRx = sink->GetTotalRx();
      double cur = ((curRx - lastTotalRx[u]) * 8.0) / ((double)inter/1000);
      //double avg = (curRx * 8.0) / now.GetSeconds ();     /* Convert Application RX Packets to MBits. */
      std::cout <<"Time: "<< now.GetSeconds () <<" UE "<<idd<< " Current Throughput: " << cur << " bit/s" << std::endl;
      //std::cout <<"Time: "<< now.GetSeconds () <<" UE "<<u<<" Average Throughput: " << avg << " bit/s" << std::endl;
      lastTotalRx[u] = curRx;
      }
      Simulator::Schedule (MilliSeconds (inter), &CalculateThroughput,inter);
    }




int main(int argc, char *argv[])

{
    
      // this scenario, but do this before processing command line
      // arguments, so that the user is allowed to override these settings
    Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (1)));
    Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (100000000));
    Config::SetDefault ("ns3::UdpClient::PacketSize", UintegerValue (1024));
    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(10 * 1024));
    Config::SetDefault("ns3::NrRlcAm::MaxTxBufferSize", UintegerValue(10 * 1024));

    // Number of gNBs along X axis (for grid layout)
    uint32_t numGnbSitesX = 1;

    // Total number of gNBs (base stations)
    uint32_t numGnbSites = 1;

    // Distance between two nearby gNBs (meters)
    double interSiteDistance = 500.0;

    // Area margin factor for simulation area size
    double areaMarginFactor = 0.5;

    // UE density per square meter
    double ueDensity = 0.0002;

    // gNB transmit power in dBm
    double gnbTxPowerDbm = 46.0;

    // Carrier frequency in Hz (NR does not use EARFCN)
    double frequency = 3.5e9; // 3.5 GHz

    // System bandwidth in Hz (1 RB = 180 kHz, e.g., 50 RBs = 9 MHz)
    uint16_t bandwidthRbs = 50;
    double bandwidth = bandwidthRbs * 180000; // 9 MHz

    // Simulation time in seconds
    double simTime = 1000.0;

    // Enable EPC (core network)
    bool epc = true;

    // Enable downlink data flows when EPC is used
    bool epcDl = true;

    // Select UDP or TCP application traffic
    bool useUdp = false; // true for UDP, false for TCP

    // Select OnOff application for TCP (if useUdp == false)
    bool onOffApp = false;

    // Number of QoS flows (bearers) per UE
    uint16_t numQosFlows = 1;

    /// Minimum speed value of macro UE with random waypoint model [m/s].
    double outdoorUeMinSpeed = 15.0;
    /// Maximum speed value of macro UE with random waypoint model [m/s].
    double outdoorUeMaxSpeed = 20.0;

    // Mobility model type for UEs: "steady", "randomwalk", "constant", etc.
    std::string mobilityType = "steady";
    
    // set number of sectors per enb
    std::string multiSector = "false";
   
    /// SRS Periodicity (has to be at least greater than the number of UEs per eNB)
    uint16_t srsPeriodicity = 160; // 80



    // Calculate simulation area and gNB grid (migrated from LTE Box logic)
    Box ueBox;
    double ueZ = 1.5;
    if (numGnbSites > 0)
    {
        uint32_t currentSite = numGnbSites - 1;
        uint32_t biRowIndex = (currentSite / (numGnbSitesX + numGnbSitesX + 1));
        uint32_t biRowRemainder = currentSite % (numGnbSitesX + numGnbSitesX + 1);
        uint32_t rowIndex = biRowIndex * 2 + 1;
        if (biRowRemainder >= numGnbSitesX)
        {
            ++rowIndex;
        }
        uint32_t numGnbSitesY = rowIndex;
        NS_LOG_UNCOND("numGnbSitesY = " << numGnbSitesY);

        ueBox = Box(-areaMarginFactor * interSiteDistance,
                    (numGnbSitesX + areaMarginFactor) * interSiteDistance,
                    -areaMarginFactor * interSiteDistance,
                    (numGnbSitesY - 1) * interSiteDistance * sqrt(0.75) +
                        areaMarginFactor * interSiteDistance,
                    ueZ,
                    ueZ);
    }
    
    // Calculate total simulation area for UEs
    double ueAreaSize = (ueBox.xMax - ueBox.xMin) * (ueBox.yMax - ueBox.yMin);

    // Calculate number of UEs based on area and density
    uint32_t numUe = static_cast<uint32_t>(round(ueAreaSize * ueDensity));
    NS_LOG_UNCOND("numUe = " << numUe << " (density=" << ueDensity << ")");


    NodeContainer gNbNodes;
    if (multiSector == "true") {
        gNbNodes.Create(3 * numGnbSites); 
    } else {
        gNbNodes.Create(1 * numGnbSites); 
    }

    NodeContainer ueNodes;
    ueNodes.Create(numUe);

    // Configure basic mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();

    // Configure gNB antenna array (e.g., 8x8 elements, dual-polarized)
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("IsDualPolarized", BooleanValue(true));

    // Configure UE antenna array (e.g., 2x2 elements, single-polarized)
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("IsDualPolarized", BooleanValue(false));

    // Set gNB and UE transmit power (in dBm)
    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(30.0)); // gNB: 30 dBm
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(23.0));  // UE: 23 dBm

    // Configure SRS for NR using Config::SetDefault
    Config::SetDefault("ns3::NrGnbRrc::SrsPeriodicity", UintegerValue(srsPeriodicity));

    Ptr<NrPointToPointEpcHelper> epcHelper;
    if (epc)
    {
        NS_LOG_UNCOND("Enabling EPC for 5G NR");
        epcHelper = CreateObject<NrPointToPointEpcHelper>();
        nrHelper->SetEpcHelper(epcHelper);
    }


    // Install protocol stack
    InternetStackHelper internet;
    internet.Install(ueNodes);


    // Set gNB positions (e.g., two gNBs 200m apart)
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    gnbPositionAlloc->Add(Vector(0.0, 0.0, 10.0));    // gNB 0 at (0,0,10)
    gnbPositionAlloc->Add(Vector(50.0, 0.0, 10.0));   // gNB 1 at (50,0,10)
    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobility.SetPositionAllocator(gnbPositionAlloc);
    gnbMobility.Install(gNbNodes);

    // Set UE initial positions and mobility (RandomWalk2d)
    MobilityHelper ueMobility;
    ueMobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=20.0]"));
    ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(0, 50, 0, 20)),
        "Distance", DoubleValue(10.0),
        "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"));
    ueMobility.Install(ueNodes);

    // Configure NR frequency band and bandwidth
    CcBwpCreator ccBwpCreator;
    CcBwpCreator::SimpleOperationBandConf bandConf(frequency, bandwidth, 1, BandwidthPartInfo::UMa);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

  
    nrHelper->InitializeOperationBand(&band);
    BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps({band});
    nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());

    // Install devices
    NetDeviceContainer gNbDevs = nrHelper->InstallGnbDevice(gNbNodes, allBwps);
    NetDeviceContainer ueDevs = nrHelper->InstallUeDevice(ueNodes, allBwps);

    // Configure IP
    epcHelper->AssignUeIpv4Address(ueDevs);
    nrHelper->AttachToClosestGnb(ueDevs, gNbDevs);

    for (auto it = gNbDevs.Begin(); it != gNbDevs.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }
    for (auto it = ueDevs.Begin(); it != ueDevs.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }

    NS_LOG_UNCOND("5G NR minimal scenario started!");

    // Create RemoteHost and install protocol stack
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internetRemote;
    internetRemote.Install(remoteHostContainer);

    // Configure point-to-point link between RemoteHost and PGW
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;

    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Install IP stack on UEs and assign IP addresses
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    // Set default gateway for UEs
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Configure UDP downlink applications with different QoS flows
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        for (uint32_t q = 0; q < numQosFlows; ++q)
        {
            // Configure different ports for each QoS flow
            uint16_t dlPort = 1234 + q;
            
            // Install UDP server on UE
            UdpServerHelper dlPacketSinkHelper(dlPort);
            serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

            // Configure UDP client parameters
            UdpClientHelper dlClientHelper(ueIpIface.GetAddress(u), dlPort);
            
            // Set different parameters based on QoS class
            if (q == 0)  // High priority traffic
            {
                dlClientHelper.SetAttribute("Interval", TimeValue(MicroSeconds(100)));  // Shorter interval
                dlClientHelper.SetAttribute("PacketSize", UintegerValue(1000));         // Smaller packets
            }
            else  // Low priority traffic
            {
                dlClientHelper.SetAttribute("Interval", TimeValue(MicroSeconds(1000))); // Longer interval
                dlClientHelper.SetAttribute("PacketSize", UintegerValue(1500));         // Larger packets
            }
            
            dlClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            clientApps.Add(dlClientHelper.Install(remoteHost));
        }
    }

    // Start and stop applications
    serverApps.Start(Seconds(0.4));
    clientApps.Start(Seconds(0.4));
    serverApps.Stop(Seconds(10.0));
    clientApps.Stop(Seconds(9.8));

    // Enable NR module traces (PHY, MAC, RLC, PDCP, etc.)
    nrHelper->EnableTraces();

    // Register callback for UE RRC connection established event
    Config::Connect("/NodeList/*/DeviceList/*/NrUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/NrGnbRrc/NotifyConnectionRelease",
                    MakeCallback(&NotifyConnectionReleasedUe));

    // Register callback for gNB RRC handover start event
    Config::Connect("/NodeList/*/DeviceList/*/NrGnbRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartGnb));
    // Register callback for gNB RRC handover end event
    Config::Connect("/NodeList/*/DeviceList/*/NrGnbRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkGnb));

    // Register PHY layer measurement and PRB utilization traces
    Config::Connect(
       "/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUePhy/ReportUeMeasurements",
       MakeCallback(&RsrpSinrTrace)
    );
    Config::Connect(
       "/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUePhy/ReportUeMeasurements",
       MakeCallback(&CqiTrace)
    );
    Config::Connect(
       "/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUePhy/ReportUplinkTbSize",
       MakeCallback(&PrbUtilTrace)
    );

    // Register buffer status trace for UE MAC
    //Config::Connect(
    //    "/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUeMac/ReportBufferStatus",
    //    MakeCallback(&BufferStatusTrace)
    //);

    // Enable FlowMonitor for all nodes
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Print statistics for each QoS flow
    std::ofstream udpStats("udp_stats.csv");
    udpStats << "UE,QoSFlow,ReceivedPackets\n";
    for (uint32_t u = 0; u < numUe; ++u)
    {
        for (uint32_t q = 0; q < numQosFlows; ++q)
        {
            Ptr<UdpServer> serverApp = serverApps.Get(u * numQosFlows + q)->GetObject<UdpServer>();
            uint64_t receivedPackets = serverApp->GetReceived();
            udpStats << u << "," << q << "," << receivedPackets << "\n";
        }
    }
    udpStats.close();

    // Print FlowMonitor statistics for each flow
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    std::ofstream flowStats("flow_stats.csv");
    flowStats << "FlowId,Src,Dest,TxPackets,RxPackets,LostPackets,ThroughputMbps,MeanDelaySec\n";
    for (const auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        double throughput = (flow.second.rxBytes * 8.0) /
            (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) / 1e6;
        double meanDelay = (flow.second.rxPackets > 0) ? (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) : 0.0;
        flowStats << flow.first << "," << t.sourceAddress << "," << t.destinationAddress << ","
                  << flow.second.txPackets << "," << flow.second.rxPackets << "," << flow.second.lostPackets << ","
                  << throughput << "," << meanDelay << "\n";
    }
    flowStats.close();

    Simulator::Destroy();
    return 0;
}
