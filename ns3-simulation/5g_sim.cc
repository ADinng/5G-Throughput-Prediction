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
#include "ns3/antenna-module.h"
#include "ns3/eps-bearer.h"
#include "ns3/epc-tft.h"

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

static void
CourseChange(std::string foo, Ptr<const MobilityModel> mobility)
{
    Time now = Simulator::Now();
    Vector pos = mobility->GetPosition(); // Get position
    std::cout << "Time: " << now.GetSeconds() << " " << foo << " POS: x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
}

void
ReportUeMeasurementsCallback(std::string path, uint16_t rnti, uint16_t cellId,
                            double rsrp, double rsrq, bool servingCell, uint8_t componentCarrierId)
{
    if (servingCell == true)
        std::cout << Simulator::Now().GetSeconds() << " "
                  << " RNTI " << rnti
                  << ", CellId: " << cellId
                  << ", Serving Cell: " << servingCell
                  << ", RSRP: " << rsrp
                  << ", RSRQ: " << rsrq
                  << std::endl;
}



// holds number of sinks
uint32_t numSinks;
// pointer to the sinks
ApplicationContainer sinks;
// holds last amount of received bytes for all the clients
std::vector<int> lastTotalRx;

void CalculateThroughput(uint16_t inter)
{
    Time now = Simulator::Now(); /* Return the simulator's virtual time. */

    for (uint32_t u = 0; u < numSinks; ++u)
    {
        Ptr<PacketSink> sink = StaticCast<PacketSink>(sinks.Get(u));
        uint32_t idd = ((sinks.Get(u))->GetNode())->GetId();
        double curRx = sink->GetTotalRx();
        double cur = ((curRx - lastTotalRx[u]) * 8.0) / ((double)inter / 1000);
        // double avg = (curRx * 8.0) / now.GetSeconds();     /* Convert Application RX Packets to MBits. */
        std::cout << "Time: " << now.GetSeconds() << " UE " << idd << " Current Throughput: " << cur << " bit/s" << std::endl;
        // std::cout << "Time: " << now.GetSeconds() << " UE " << u << " Average Throughput: " << avg << " bit/s" << std::endl;
        lastTotalRx[u] = curRx;
    }

    Simulator::Schedule(MilliSeconds(inter), &CalculateThroughput, inter);
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

    // Number of gNB sites (cells)
    uint32_t numGnbSites = 1; // Number of sites

    // Grid layout for sites (hexagonal grid)
    uint32_t numGnbSitesX = 1; // Number of sites along X axis
    uint32_t numGnbSitesY = 1; // Number of sites along Y axis
    double interSiteDistance = 500.0; // Distance between sites (meters)
    double gnbHeight = 10.0; // gNB height
    // Area margin factor for simulation area size
    double areaMarginFactor = 0.5;
    // UE density per square meter
    double ueDensity = 0.0002;
    // gNB transmit power in dBm
    //double gnbTxPowerDbm = 46.0;
    // Carrier frequency in Hz (NR does not use EARFCN)
    double frequency = 3.5e9; // 3.5 GHz
    double bandwidth = 10e6;


    // Bandwidth configuration
    // uint16_t bandwidthRbs = 273;  // 50 MHz
    // double bandwidth = bandwidthRbs * 180000; // 49.14 MHz
    // // Configure RBG size for 108MHz bandwidth
    // Config::SetDefault("ns3::NrGnbMac::NumRbPerRbg", UintegerValue(8));

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
    uint32_t numSectorsPerSite = (multiSector == "true") ? 3 : 1;
    /// SRS Periodicity (has to be at least greater than the number of UEs per eNB)
    uint16_t srsPeriodicity = 160; // 80
    // Configure SRS for NR using Config::SetDefault
    Config::SetDefault("ns3::NrGnbRrc::SrsPeriodicity", UintegerValue(srsPeriodicity));

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
    
    // // Force UE area to 100m x 100m centered at (0,0) for debugging
    // ueBox = Box(-50, 50, -50, 50, ueZ, ueZ);

    // Calculate total simulation area for UEs
    double ueAreaSize = (ueBox.xMax - ueBox.xMin) * (ueBox.yMax - ueBox.yMin);

    // Calculate number of UEs based on area and density
    uint32_t numUe = static_cast<uint32_t>(round(ueAreaSize * ueDensity));
    NS_LOG_UNCOND("numUe = " << numUe << " (density=" << ueDensity << ")");


    NodeContainer gNbNodes;
    gNbNodes.Create(numGnbSites * numSectorsPerSite);
    // if (multiSector == "true") {
    //     gNbNodes.Create(3 * numGnbSites); 
    // } else {
    //     gNbNodes.Create(1 * numGnbSites); 
    // }

    NodeContainer ueNodes;
    ueNodes.Create(numUe);

    // MobilityHelper mobility;
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

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

    // Prepare position allocator and sector orientations
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    std::vector<double> sectorOrientations;
    for (uint32_t s = 0; s < numSectorsPerSite; ++s) {
        sectorOrientations.push_back(360.0 * s / numSectorsPerSite);
    }

    // Place each site and its sectors
    for (uint32_t i = 0; i < numGnbSitesX; ++i) {
        for (uint32_t j = 0; j < numGnbSitesY; ++j) {
            // Hexagonal grid coordinates
            double x = interSiteDistance * (i + 0.5 * (j % 2));
            double y = interSiteDistance * j * sqrt(3.0) / 2.0;
            for (uint32_t sector = 0; sector < numSectorsPerSite; ++sector) {
                gnbPositionAlloc->Add(Vector(x, y, gnbHeight));
            }
        }
    }



    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobility.SetPositionAllocator(gnbPositionAlloc);
    gnbMobility.Install(gNbNodes);
    BuildingsHelper::Install(gNbNodes);


    // // Configure antenna model and parameters for gNBs
    // if (multiSector == "true") {
    //     // Multi-sector configuration (3 sectors)
    //     nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(8));
    //     nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    //     nrHelper->SetGnbAntennaAttribute("IsDualPolarized", BooleanValue(true));
    //     nrHelper->SetGnbAntennaAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));
    //     nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(gnbTxPowerDbm));
    // }
    // else if (multiSector == "false") {
    //     // Single sector configuration (isotropic)
    //     NS_LOG_UNCOND("Setting isotropic antenna");
    //     nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(1));
    //     nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(1));
    //     nrHelper->SetGnbAntennaAttribute("IsDualPolarized", BooleanValue(false));
    //     nrHelper->SetGnbAntennaAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));
    //     nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(10.0));
    // }

    // Configure handover algorithm for 5G NR
    nrHelper->SetHandoverAlgorithmType("ns3::NrA3RsrpHandoverAlgorithm");
    nrHelper->SetHandoverAlgorithmAttribute("Hysteresis", DoubleValue(3.0));
    nrHelper->SetHandoverAlgorithmAttribute("TimeToTrigger", TimeValue(MilliSeconds(256)));



    // Set UE initial positions and mobility based on mobilityType
    MobilityHelper ueMobility;
    ueMobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=20.0]"));

    if (outdoorUeMaxSpeed != 0.0)
    {
        if (mobilityType == "steady")
        {
            ueMobility.SetMobilityModel("ns3::SteadyStateRandomWaypointMobilityModel");
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue(ueBox.xMin));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue(ueBox.yMin));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue(ueBox.xMax));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue(ueBox.yMax));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue(ueZ));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue(outdoorUeMaxSpeed));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue(outdoorUeMinSpeed));
        }
        else if (mobilityType == "gauss")
        {
            ueMobility.SetMobilityModel("ns3::GaussMarkovMobilityModel");
            Config::SetDefault("ns3::GaussMarkovMobilityModel::Bounds", BoxValue(ueBox));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::TimeStep", TimeValue(Seconds(1.0)));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::Alpha", DoubleValue(0.85));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::MeanVelocity", StringValue("ns3::UniformRandomVariable[Min=" + std::to_string(outdoorUeMinSpeed) + "|Max=" + std::to_string(outdoorUeMaxSpeed) + "]"));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::NormalVelocity", StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::NormalDirection", StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::NormalPitch", StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"));
        }
        // Install mobility model and position allocator
        ueMobility.Install(ueNodes);
    }
    else
    {
        // Static random position allocation using simulation area box
        Ptr<RandomBoxPositionAllocator> positionAlloc = CreateObject<RandomBoxPositionAllocator>();
        positionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=" + std::to_string(ueBox.xMin) + "|Max=" + std::to_string(ueBox.xMax) + "]"));
        positionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=" + std::to_string(ueBox.yMin) + "|Max=" + std::to_string(ueBox.yMax) + "]"));
        positionAlloc->SetAttribute("Z", StringValue("ns3::UniformRandomVariable[Min=" + std::to_string(ueBox.zMin) + "|Max=" + std::to_string(ueBox.zMax) + "]"));
        ueMobility.SetPositionAllocator(positionAlloc);
        ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        ueMobility.Install(ueNodes);
    }

    BuildingsHelper::Install(ueNodes);

    // Prepare allBwps before any device installation
    CcBwpCreator ccBwpCreator;
    CcBwpCreator::SimpleOperationBandConf bandConf(frequency, bandwidth, 1, BandwidthPartInfo::UMa);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
    nrHelper->InitializeOperationBand(&band);
    BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps({band});

    // Create global device containers
    NetDeviceContainer gNbDevs;
    NetDeviceContainer ueDevs;

    // Install gNB devices in batches, each batch with a different orientation
    for (uint32_t sector = 0; sector < numSectorsPerSite; ++sector) {
        NodeContainer sectorNodes;
        for (uint32_t site = 0; site < numGnbSites; ++site) {
            sectorNodes.Add(gNbNodes.Get(site * numSectorsPerSite + sector));
        }
        double orientation = sectorOrientations[sector];
        nrHelper->SetGnbAntennaAttribute("BearingAngle", DoubleValue(orientation * M_PI / 180.0));
        NetDeviceContainer sectorDevs = nrHelper->InstallGnbDevice(sectorNodes, allBwps);
        gNbDevs.Add(sectorDevs);
    }


    // Install all UE devices at once
    ueDevs = nrHelper->InstallUeDevice(ueNodes, allBwps);

    // Configure scheduler
    nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());
    nrHelper->SetSchedulerAttribute("FixedMcsDl", BooleanValue(true));
    nrHelper->SetSchedulerAttribute("FixedMcsUl", BooleanValue(true));
    nrHelper->SetSchedulerAttribute("StartingMcsDl", UintegerValue(28));
    nrHelper->SetSchedulerAttribute("StartingMcsUl", UintegerValue(28));

    // Configure RBG size
    // nrHelper->SetGnbMacAttribute("NumRbPerRbg", UintegerValue(4));
    // nrHelper->SetGnbMacAttribute("NumHarqProcess", UintegerValue(20));


    // Set antenna orientation for each gNB node before device installation
    for (uint32_t n = 0; n < gNbNodes.GetN(); ++n) {
        double orientation = sectorOrientations[n % numSectorsPerSite];
        nrHelper->SetGnbAntennaAttribute("BearingAngle", DoubleValue(orientation * M_PI / 180.0));
 
    }
    // NetDeviceContainer gNbDevs = nrHelper->InstallGnbDevice(gNbNodes, allBwps);
    // NetDeviceContainer ueDevs = nrHelper->InstallUeDevice(ueNodes, allBwps);



    Ipv4Address remoteHostAddr;
    NodeContainer ues;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ipv4InterfaceContainer ueIpIfaces;
    Ptr<Node> remoteHost;
    NetDeviceContainer allueDevs;

    if (epc)
    {
        // Create RemoteHost and install protocol stack
        Ptr<Node> pgw = epcHelper->GetPgwNode();
        NodeContainer remoteHostContainer;
        remoteHostContainer.Create(1);
        remoteHost = remoteHostContainer.Get(0);
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
        
        // Get the IP address of the RemoteHost (interface 1) for potential use in applications
        remoteHostAddr = internetIpIfaces.GetAddress(1);
        
        Ipv4StaticRoutingHelper ipv4RoutingHelper;
        Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
        remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

        ues.Add(ueNodes);
        allueDevs.Add(ueDevs);

        // Install the IP stack on the UEs
        internet.Install(ues);
        ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(allueDevs));

        // attachment (needs to be done after IP stack configuration)
        // using initial cell selection
        nrHelper->AttachToClosestGnb(allueDevs, gNbDevs);
    }



    if (epc)
    {
        NS_LOG_LOGIC("setting up applications");

        uint16_t dlPort = 10000;
        uint16_t ulPort = 20000;

        // Randomize start times to avoid simulation artifacts
        Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
        if (useUdp)
        {
            startTimeSeconds->SetAttribute("Min", DoubleValue(0));
            startTimeSeconds->SetAttribute("Max", DoubleValue(500));
        }
        else
        {
            startTimeSeconds->SetAttribute("Min", DoubleValue(0.100));
            startTimeSeconds->SetAttribute("Max", DoubleValue(0.110));
        }
        numSinks = ueNodes.GetN();
        for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
        {
            Ptr<Node> ue = ueNodes.Get(u);
            // Set the default gateway for the UE
            Ptr<Ipv4StaticRouting> ueStaticRouting =
                ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

            for (uint32_t b = 0; b < numQosFlows; ++b)
            {
                ++dlPort;
                ++ulPort;

                ApplicationContainer clientApps;
                ApplicationContainer serverApps;

                if (useUdp)
                {
                    if (epcDl)
                    {
                        NS_LOG_LOGIC("installing UDP DL app for UE " << u);
                        UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
                        clientApps.Add(dlClientHelper.Install(remoteHost));
                        PacketSinkHelper dlPacketSinkHelper(
                            "ns3::UdpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                        serverApps.Add(dlPacketSinkHelper.Install(ue));
                        sinks.Add(serverApps);
                        lastTotalRx.push_back(0);
                    }
                }
                else // use TCP
                {
                    if (epcDl)
                    {
                        if (onOffApp == false) {
                            NS_LOG_LOGIC("installing TCP DL app for UE " << u);
                            BulkSendHelper dlClientHelper(
                                "ns3::TcpSocketFactory",
                                InetSocketAddress(ueIpIfaces.GetAddress(u), dlPort));
                            dlClientHelper.SetAttribute("MaxBytes", UintegerValue(0));
                            clientApps.Add(dlClientHelper.Install(remoteHost));
                            PacketSinkHelper dlPacketSinkHelper(
                                "ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                            serverApps.Add(dlPacketSinkHelper.Install(ue));
                            sinks.Add(serverApps);
                            lastTotalRx.push_back(0);
                        }
                        else {
                            Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("15Mb/s"));
                            NS_LOG_LOGIC("installing TCP DL ON OFF app for UE " << u);
                            OnOffHelper dlClientHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(u), dlPort));
                            dlClientHelper.SetAttribute("OnTime", StringValue("ns3::UniformRandomVariable[Min=15.0|Max=25.0]"));
                            dlClientHelper.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=15.0|Max=25.0]"));
                            dlClientHelper.SetAttribute("MaxBytes", UintegerValue(0));
                            clientApps.Add(dlClientHelper.Install(remoteHost));
                            PacketSinkHelper dlPacketSinkHelper(
                                "ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                            serverApps.Add(dlPacketSinkHelper.Install(ue));
                            sinks.Add(serverApps);
                            lastTotalRx.push_back(0);
                        }
                    }
                } // end if (useUdp)

                // Activate dedicated EPS bearer for this flow (if supported)
                Ptr<NrEpcTft> tft = Create<NrEpcTft>();
                if (epcDl)
                {
                    NrEpcTft::PacketFilter dlpf;
                    dlpf.localPortStart = dlPort;
                    dlpf.localPortEnd = dlPort;
                    tft->Add(dlpf);
                }
                if (epcDl)
                {
                    NrEpsBearer bearer(NrEpsBearer::NGBR_VIDEO_TCP_DEFAULT);
                    nrHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);
                }
                // NrEpsBearer bearer(NrEpsBearer::NR_QCI_NON_GBR_DEFAULT);
                // nrHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);

                Time startTime = Seconds(startTimeSeconds->GetValue());
                std::cout << Simulator::Now().GetSeconds() << " UE " << u << " startTime: " << startTime << std::endl;

                serverApps.Start(startTime);
                clientApps.Start(startTime);
            } // end for b
        }
    }


    Simulator::Stop(Seconds(simTime));
    
    // Enable NR module traces (PHY, MAC, RLC, PDCP, etc.)
    nrHelper->EnableTraces();

    // Register callback for UE RRC connection established event
    Config::Connect("/NodeList/*/DeviceList/*/NrUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/NrGnbRrc/NotifyConnectionRelease",
                    MakeCallback(&NotifyConnectionReleasedUe));

    // Register callback for gNB RRC handover start event
    // Config::Connect("/NodeList/*/DeviceList/*/NrGnbRrc/HandoverStart",
    //                 MakeCallback(&NotifyHandoverStartGnb));
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

        // mobility callbacks
    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback(&CourseChange));

    // UE measurements callback
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/ReportUeMeasurements",
                   MakeBoundCallback(&ReportUeMeasurementsCallback));

    // Schedule throughput calculation
    Simulator::Schedule(Seconds(0.4), &CalculateThroughput, 100);
    

    // Register buffer status trace for UE MAC
    //Config::Connect(
    //    "/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUeMac/ReportBufferStatus",
    //    MakeCallback(&BufferStatusTrace)
    //);

    // // Enable FlowMonitor for all nodes
    // Ptr<FlowMonitor> flowMonitor;
    // FlowMonitorHelper flowHelper;
    // flowMonitor = flowHelper.InstallAll();

    Simulator::Run();

    // Print statistics for each QoS flow
    // std::ofstream udpStats("udp_stats.csv");
    // udpStats << "UE,QoSFlow,ReceivedPackets\n";
    // for (uint32_t u = 0; u < numUe; ++u)
    // {
    //     for (uint32_t q = 0; q < numQosFlows; ++q)
    //     {
    //         Ptr<UdpServer> serverApp = serverApps.Get(u * numQosFlows + q)->GetObject<UdpServer>();
    //         uint64_t receivedPackets = serverApp->GetReceived();
    //         udpStats << u << "," << q << "," << receivedPackets << "\n";
    //     }
    // }
    // udpStats.close();

    // // Print FlowMonitor statistics for each flow
    // flowMonitor->CheckForLostPackets();
    // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    // std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    // std::ofstream flowStats("flow_stats.csv");
    // flowStats << "FlowId,Src,Dest,TxPackets,RxPackets,LostPackets,ThroughputMbps,MeanDelaySec\n";
    // for (const auto& flow : stats)
    // {
    //     Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
    //     double throughput = (flow.second.rxBytes * 8.0) /
    //         (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) / 1e6;
    //     double meanDelay = (flow.second.rxPackets > 0) ? (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) : 0.0;
    //     flowStats << flow.first << "," << t.sourceAddress << "," << t.destinationAddress << ","
    //               << flow.second.txPackets << "," << flow.second.rxPackets << "," << flow.second.lostPackets << ","
    //               << throughput << "," << meanDelay << "\n";
    // }
    // flowStats.close();
    nrHelper = nullptr;
    Simulator::Destroy();
    return 0;
}
