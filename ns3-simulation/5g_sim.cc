#include "ns3/applications-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/antenna-module.h>
#include <ns3/buildings-helper.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/ipv4-flow-classifier.h>
#include <ns3/nr-stats-calculator.h>
#include <cmath>  
#include <fstream>
#include <map>
#include "ns3/string.h"
#include "ns3/callback.h"
#include "ns3/hexagonal-grid-scenario-helper.h"
#include "ns3/realistic-beamforming-algorithm.h"


using namespace ns3;
NS_LOG_COMPONENT_DEFINE("NRSimpleScenario");


std::map<uint16_t, uint32_t> g_rntiToUeIndex;

// Callback for UE connection established
void 
NotifyConnectionEstablishedUe (std::string context, 
                                uint64_t imsi, 
                                uint16_t cellId, 
                                uint16_t rnti)
{
    uint32_t ueIndex = imsi -1;
    g_rntiToUeIndex[rnti] = ueIndex;
    std::cout << Simulator::Now().GetSeconds() << " " << context
              << " UE IMSI " << imsi
              << ": connected to CellId " << cellId
              << " with RNTI " << rnti 
              << std::endl;
    
    //std::cout << Simulator::Now().GetSeconds() << " "<< "UE: " << ueIndex << " imsi: " << imsi << " rnti: " << rnti <<std::endl;
    
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

void RrcStateChangeCallback(uint64_t imsi, uint16_t cellId, uint16_t rnti, NrUeRrc::State oldState, NrUeRrc::State newState)
{
    std::cout << "UE RRC state changed: IMSI=" << imsi 
              << ", CellId=" << cellId 
              << ", RNTI=" << rnti 
              << ", OldState=" << oldState 
              << ", NewState=" << newState << std::endl;
}

uint32_t numSinks;
ApplicationContainer sinks;
std::vector<int> lastTotalRx;
std::ofstream throughputFile; 

void CalculateThroughput(uint16_t inter)
{
    Time now = Simulator::Now();
    for (uint32_t u = 0; u < numSinks; ++u)
    {
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




static void
CourseChange(std::string foo, Ptr<const MobilityModel> mobility)
{
    Time now = Simulator::Now();
    Vector pos = mobility->GetPosition(); // Get position
    std::cout << "Time: " << now.GetSeconds() << " " << foo << " POS: x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
}


void CqiTrace(std::string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr, bool isServingCell, uint8_t componentCarrierId)
{
    std::cout << "[CQI] " << context << " CellId=" << cellId << " RNTI=" << rnti
              << " RSRP=" << rsrp << " SINR=" << sinr << " ServingCell=" << isServingCell
              << " CCId=" << (int)componentCarrierId << std::endl;
}


struct UeRssiInfo {
    double rssi_dBm = NaN;
    double time = NaN;
};

std::map<uint32_t, UeRssiInfo> g_ueIndexToRssi;  //ueIndex ->rssi

void
UeRssiPerProcessedChunkTrace(uint32_t ueIndex, double rssidBm)
{
    g_ueIndexToRssi[ueIndex].rssi_dBm=rssidBm;
    //std::cout << Simulator::Now().GetSeconds() << " "<< "UE " << ueIndex << " RSSI (dBm): " << rssidBm << std::endl;
}


void RsrpTracedCallback (uint16_t rnti,
                                           double rsrp,
                                           uint8_t componentCarrierId){

                                           
            std::cout << Simulator::Now().GetSeconds() << " "
            << " RNTI " << rnti

            << ", RSRP: " << rsrp
            << std::endl;
    }
    ;




uint32_t numRbs_global = 533;
extern uint32_t numRbs_global;
void
ReportUeMeasurementsCallback(
                             std::string path,
                             uint16_t rnti,
                             uint16_t cellId,
                             double rsrp,
                             double rsrq,
                             bool servingCell,
                             uint8_t componentCarrierId)
{
    if (servingCell == true)
    {
        uint32_t ueIndex = g_rntiToUeIndex[rnti];
        double rssi_dBm = g_ueIndexToRssi[ueIndex].rssi_dBm;

        if (!std::isnan(rssi_dBm))
        {
            // double rsrp_mW = std::pow(10.0, rsrp / 10.0);
            // double rssi_mW = std::pow(10.0, rssi_dBm / 10.0);
            // double rsrq_linear = (numRbs_global * rsrp_mW) / rssi_mW;
            // double rsrq_dB = 10 * std::log10(rsrq_linear);

            // double rsrq_dB = 10 * std::log10(numRbs_global) + rsrp - rssi_dBm;

            std::cout << Simulator::Now().GetSeconds() << " "
            << " RNTI " << rnti
            << ", CellId: " << cellId
            << ", Serving Cell: " << servingCell
            << ", RSRP: " << rsrp
            << ", RSSI: " << rssi_dBm
            // << ", RSRQ: " << rsrq_dB
            << std::endl;

            // std::cout << "RNTI: " << rnti << ", ueIndex: " << ueIndex << ", RSRP: " << rsrp << ", RSSI: " << rssi_dBm << std::endl;
        }
        // else
        // {
        //     std::cout << Simulator::Now().GetSeconds() << " "
        //     << " RNTI " << rnti
        //     << ", CellId: " << cellId
        //     << ", Serving Cell: " << servingCell
        //     << ", RSRP: " << rsrp
        //     << ", RSRQ: " << rsrq
        //     << std::endl;
        // }

    }
}



int
main(int argc, char* argv[])
{
    uint32_t nGnbSitesX = 1;        
    uint32_t nGnbSites = 1* nGnbSitesX;
    double interSiteDistance =  200; 
    // double ueDensity = 0.0005;  //100-20
    double ueDensity = 0.000125;  //200-20
    // double ueDensity = 0.00025;  //200-40
    // double ueDensity = 0.000375;  //200-60
    // double ueDensity = 0.0005; //200-80
    // double ueDensity = 0.000625; //200-100

    uint32_t numBearersPerUe = 1;  

    std::string scenario = "UMa"; // scenario
    double frequency = 3.5e9;      // central frequency
    double bandwidth = 20e6;     // bandwidth
    double simTime = 100;           // in second
    bool logging = true; // whether to enable logging from the simulation, another option is by
                         // exporting the NS_LOG environment variable
    double hBS=25;          // base station antenna height in meters
    double hUT=1.5;          // user antenna height in meters
    double txPower = 40; // txPower

    // Add application type flags
    bool useUdp = false;  // if true, use UDP; if false, use TCP
    bool onOffApp = true; // if true, use On-Off application; if false, use BulkSend


    /// Minimum speed value of macro UE with random waypoint model [m/s].
    uint16_t outdoorUeMinSpeed = 15.0;
    /// Maximum speed value of macro UE with random waypoint model [m/s].
    uint16_t outdoorUeMaxSpeed = 20.0;
    // set mobility model: steady, gauss
    std::string mobilityType = "steady";

    // // SRS Periodicity (has to be at least greater than the number of UEs per gNB)
    uint16_t srsPeriodicity = 160; // 80
    Config::SetDefault("ns3::NrGnbRrc::SrsPeriodicity", UintegerValue(srsPeriodicity));

    // enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa;
    // BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa;
    BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa_LoS;

    CommandLine cmd(__FILE__);
    cmd.AddValue("ueDensity", "UE density in the simulation area", ueDensity);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue("useUdp", "Use UDP protocol if true, TCP otherwise", useUdp);
    cmd.AddValue("onOffApp", "Use OnOff application if true, Bulk otherwise", onOffApp);
    cmd.Parse(argc, argv);

    // enable logging
    if (logging)
    {
        // LogComponentEnable ("ThreeGppSpectrumPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ThreeGppChannelModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ChannelConditionModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        // LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
        // LogComponentEnable ("NrRlcUm", LOG_LEVEL_LOGIC);
        // LogComponentEnable ("NrPdcp", LOG_LEVEL_INFO);
        // LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
        // LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        // LogComponentEnable("NrMacSchedulerTdmaPF", LOG_LEVEL_INFO);
        // LogComponentEnable("NrGnbMac", LOG_LEVEL_INFO);
        // LogComponentEnable("NrUeMac", LOG_LEVEL_INFO);
        // LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
        //   LogComponentEnable("NrUeRrc", LOG_LEVEL_INFO);
    }

    /*
     * Default values for the simulation. We are progressively removing all
     * the instances of SetDefault, but we need it for legacy code (LTE)
     */

    Config::SetDefault("ns3::UdpClient::Interval", TimeValue (MilliSeconds (1)));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(100000000));
    Config::SetDefault("ns3::UdpClient::PacketSize", UintegerValue(1024));
    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(10 * 1024));
    // Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));

    // calculate the coverage area
    Box ueBox;
    if (nGnbSites > 0)
    {
        uint32_t nGnbSitesY = ceil((double)nGnbSites / nGnbSitesX);
        
        double coverageRadius = interSiteDistance / 2;  
        ueBox = Box(-coverageRadius,
                   (nGnbSitesX * interSiteDistance) + coverageRadius,
                   -coverageRadius,
                   (nGnbSitesY * interSiteDistance) + coverageRadius,
                   hUT,
                   hUT);
    }

    double ueAreaSize = (ueBox.xMax - ueBox.xMin) * (ueBox.yMax - ueBox.yMin);
    uint32_t nUes = round(ueAreaSize * ueDensity);

    NS_LOG_UNCOND("Area size: " << ueAreaSize << " m^2");
    NS_LOG_UNCOND("Number of UEs: " << nUes << " (density=" << ueDensity << ")");
    NS_LOG_UNCOND("Number of gNBs: " << nGnbSites);
    NS_LOG_UNCOND("UE distribution area: x[" << ueBox.xMin << "," << ueBox.xMax 
                  << "], y[" << ueBox.yMin << "," << ueBox.yMax << "]");

    // create base stations and mobile terminals
    NodeContainer gnbNodes;
    NodeContainer ueNodes;
    gnbNodes.Create(nGnbSites);
    ueNodes.Create(nUes);

    //position the base stations
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    gnbPositionAlloc->Add(Vector((ueBox.xMin+ueBox.xMax)/2, (ueBox.yMin+ueBox.yMax)/2, hBS));
    Vector pos = gnbPositionAlloc->GetNext();
    std::cout << "gNB position from allocator: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobility.SetPositionAllocator(gnbPositionAlloc);
    gnbMobility.Install(gnbNodes);
    BuildingsHelper::Install(gnbNodes);

   // Configuring user Mobility
    MobilityHelper ueMobility;
    if (outdoorUeMaxSpeed > 0.0)
    {
        if (mobilityType == "steady")
        {
            ueMobility.SetMobilityModel("ns3::SteadyStateRandomWaypointMobilityModel");
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue(ueBox.xMin));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue(ueBox.yMin));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue(ueBox.xMax));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue(ueBox.yMax));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue(hUT));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue(outdoorUeMaxSpeed));
            Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue(outdoorUeMaxSpeed));
        }
        else if (mobilityType == "gauss")
        {
            ueMobility.SetMobilityModel("ns3::GaussMarkovMobilityModel");
            Config::SetDefault("ns3::GaussMarkovMobilityModel::Bounds", BoxValue(ueBox));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::TimeStep", TimeValue(Seconds(1.0)));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::Alpha", DoubleValue(0.85));
            
            std::string meanVelStr = "ns3::UniformRandomVariable[Min=" + 
                                   std::to_string(outdoorUeMinSpeed) + "|Max=" + 
                                   std::to_string(outdoorUeMaxSpeed) + "]";
            Config::SetDefault("ns3::GaussMarkovMobilityModel::MeanVelocity", StringValue(meanVelStr));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::NormalVelocity", 
                              StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"));
            Config::SetDefault("ns3::GaussMarkovMobilityModel::NormalDirection", 
                              StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"));
        }
        
        Ptr<RandomBoxPositionAllocator> uePositionAlloc = CreateObject<RandomBoxPositionAllocator>();
        ueMobility.SetPositionAllocator(uePositionAlloc);
        ueMobility.Install(ueNodes);
        
        // forcing initialization so we don't have to wait for Nodes to
        // start before positions are assigned (which is needed to
        // output node positions to file and to make AttachToClosestEnb work)
        for (auto it = ueNodes.Begin(); it != ueNodes.End(); ++it)
        {
            (*it)->Initialize();
        }
    }
    else
    {
        // static user deployment
        ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        Ptr<RandomBoxPositionAllocator> uePositionAlloc = CreateObject<RandomBoxPositionAllocator>();
        
        Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable>();
        xVal->SetAttribute("Min", DoubleValue(ueBox.xMin));
        xVal->SetAttribute("Max", DoubleValue(ueBox.xMax));
        uePositionAlloc->SetAttribute("X", PointerValue(xVal));
        
        Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable>();
        yVal->SetAttribute("Min", DoubleValue(ueBox.yMin));
        yVal->SetAttribute("Max", DoubleValue(ueBox.yMax));
        uePositionAlloc->SetAttribute("Y", PointerValue(yVal));
        
        Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable>();
        zVal->SetAttribute("Min", DoubleValue(ueBox.zMin));
        zVal->SetAttribute("Max", DoubleValue(ueBox.zMax));
        uePositionAlloc->SetAttribute("Z", PointerValue(zVal));
        
        ueMobility.SetPositionAllocator(uePositionAlloc);
        ueMobility.Install(ueNodes);
    }

    BuildingsHelper::Install(ueNodes);

    // Create NR simulation helpers
    Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    // Ptr<RealisticBeamformingHelper> realisticBeamformingHelper = CreateObject<RealisticBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    
    // Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(0)));
    // nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(true));
    
    // nrHelper->SetBeamformingHelper(realisticBeamformingHelper);
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(nrEpcHelper);

    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // have a single band, and that band is composed of a single component carrier

    /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates a single BWP per CC and a single BWP in CC.
     *the configured spectrum is:
     * |---------------Band---------------|
     * |---------------CC-----------------|
     * |---------------BWP----------------|
     */
    CcBwpCreator::SimpleOperationBandConf bandConf(frequency,
                                                   bandwidth,
                                                   numCcPerBand,
                                                   scenarioEnum);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
    // Initialize channel and pathloss, plus other things inside band.
    nrHelper->InitializeOperationBand(&band);
    allBwps = CcBwpCreator::GetAllBwps({band});

    // Configure ideal beamforming method
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));


    // RealisticBfManager::TriggerEvent realTriggerEvent{RealisticBfManager::SRS_COUNT};
    // realisticBeamformingHelper->SetBeamformingMethod(RealisticBeamformingAlgorithm::GetTypeId());
    // nrHelper->SetGnbBeamManagerTypeId(RealisticBfManager::GetTypeId());
    // nrHelper->SetGnbBeamManagerAttribute("TriggerEvent", EnumValue(realTriggerEvent));
    // nrHelper->SetGnbBeamManagerAttribute("UpdateDelay", TimeValue(MicroSeconds(0)));
   

    // Configure scheduler
    // nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());
    // nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaPF::GetTypeId());
    // nrHelper->SetSchedulerTypeId(NrMacSchedulerOfdmaPF::GetTypeId());
    // Config::SetDefault("ns3::NrMacSchedulerTdmaPF::FairnessIndex", DoubleValue(0.2));

    // nrEpcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));

    // // Error Model: UE and GNB with same spectrum error model.
    // std::string errorModel = "ns3::NrEesmIrT2";
    // nrHelper->SetUlErrorModel(errorModel);
    // nrHelper->SetDlErrorModel(errorModel);

    // // Both DL and UL AMC will have the same model behind.
    // nrHelper->SetGnbDlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));
    // nrHelper->SetGnbUlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));

    // Noise figure for the gNB
    nrHelper->SetGnbPhyAttribute("NoiseFigure", DoubleValue(5));
    // Noise figure for the UE
    nrHelper->SetUePhyAttribute("NoiseFigure", DoubleValue(7));


    Config::SetDefault("ns3::NrGnbRrc::EpsBearerToRlcMapping",
                       EnumValue(useUdp ? NrGnbRrc::RLC_UM_ALWAYS : NrGnbRrc::RLC_AM_ALWAYS));


    // Antennas for the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Antennas for the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<ThreeGppAntennaModel>()));

    nrHelper->SetGnbAntennaAttribute("AntennaHorizontalSpacing", DoubleValue(0.5));
    nrHelper->SetGnbAntennaAttribute("AntennaVerticalSpacing", DoubleValue(0.8));
    nrHelper->SetGnbAntennaAttribute("DowntiltAngle", DoubleValue(3 * M_PI / 180.0));
                           

    // nrHelper->SetGnbPhyAttribute("Numerology", UintegerValue(1));

    NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(ueNodes, allBwps);

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    for (uint32_t i = 0; i < gnbNodes.GetN(); ++i)
    {
        nrHelper->GetGnbPhy(gnbNetDev.Get(i), 0)->SetTxPower(txPower);
    }

    for (auto it = gnbNetDev.Begin(); it != gnbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = ueNetDev.Begin(); it != ueNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }




    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ipv4Address remoteHostAddr;
    NodeContainer ues;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ipv4InterfaceContainer ueIpIface;
    Ptr<Node> remoteHost;
    NetDeviceContainer ueDevs;

    NS_LOG_LOGIC("setting up internet and remote host");
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    Ptr<Node> pgw = nrEpcHelper->GetPgwNode();
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // Ipv4StaticRoutingHelper ipv4RoutingHelper;
    // in this container, interface 0 is the pgw, 1 is the remoteHost
    remoteHostAddr = internetIpIfaces.GetAddress(1);


    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    ues.Add(ueNodes);
    ueDevs.Add(ueNetDev);

    // internet.Install(ueNodes);
    internet.Install(ues);

    // Ipv4InterfaceContainer ueIpIface;
    // ueIpIface = nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));
    ueIpIface = nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    std::cout << "Attaching UEs to gNBs..." << std::endl;
    // nrHelper->AttachToClosestGnb(ueNetDev, gnbNetDev);

    for (uint32_t i=0; i < ueDevs.GetN(); i++)
    {
        nrHelper->AttachToGnb(ueDevs.Get(i),gnbNetDev.Get(0));
    }
    std::cout << "UEs attached successfully" << std::endl;



    // assign IP address to UEs, and install UDP downlink applications
    // ApplicationContainer clientApps;
    // ApplicationContainer serverApps;
    NS_LOG_LOGIC("setting up applications");
    uint16_t dlPort = 10000;  
    uint16_t ulPort = 20000;  

    std::cout << "Configuring and starting applications..." << std::endl;
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();

    if (useUdp)
    {
        startTimeSeconds->SetAttribute("Min", DoubleValue(0));
        startTimeSeconds->SetAttribute("Max", DoubleValue(1));
        startTimeSeconds->SetStream(randomStream++);
    }
    else
    {
        startTimeSeconds->SetAttribute("Min", DoubleValue(0.100));
        startTimeSeconds->SetAttribute("Max", DoubleValue(0.500));
        // startTimeSeconds->SetAttribute("Max", DoubleValue(0.110));
        startTimeSeconds->SetStream(randomStream++);
    }

    // numSinks = ueNodes.GetN();
    numSinks = ues.GetN();
    // for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    for (uint32_t u = 0; u < ues.GetN(); ++u)
    {
        // Ptr<Node> ueNode = ueNodes.Get(u);
        Ptr<Node> ue = ues.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(nrEpcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ++dlPort;
            ++ulPort;

            ApplicationContainer clientApps;
            ApplicationContainer serverApps;

            if (useUdp)
            {
                // NS_LOG_LOGIC("installing UDP DL app for UE " << u);
                UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
                clientApps.Add(dlClient.Install(remoteHost));
                PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                serverApps.Add(dlPacketSinkHelper.Install(ue));
                sinks.Add(serverApps);
                lastTotalRx.push_back(0);  
            }
            else
            {
                if(onOffApp == false){
                    NS_LOG_LOGIC("installing TCP DL app for UE " << u);
                    // NS_LOG_UNCOND("installing TCP DL app for UE " << u);
                    BulkSendHelper dlClient("ns3::TcpSocketFactory", InetSocketAddress(ueIpIface.GetAddress(u), dlPort));
                    dlClient.SetAttribute("MaxBytes", UintegerValue(0));
                    clientApps.Add(dlClient.Install(remoteHost));
                    Ptr<BulkSendApplication> bulkSend = clientApps.Get(0)->GetObject<BulkSendApplication>();
                    PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                    serverApps.Add(dlPacketSinkHelper.Install(ue));
                    sinks.Add(serverApps);
                    lastTotalRx.push_back(0);
                }
                else{
                    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("15Mb/s"));
                    NS_LOG_LOGIC("installing TCP DL ON OFF app for UE " << u);
                    // NS_LOG_UNCOND("installing TCP DL ON OFF app for UE " << u);
                    OnOffHelper dlClient("ns3::TcpSocketFactory", InetSocketAddress(ueIpIface.GetAddress(u), dlPort));
                    dlClient.SetAttribute("OnTime", StringValue("ns3::UniformRandomVariable[Min=20.0|Max=25.0]"));
                    dlClient.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=10.0|Max=15.0]"));
                    // dlClient.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=15.0|Max=25.0]"));
                    dlClient.SetAttribute("MaxBytes", UintegerValue(0));
                    clientApps.Add(dlClient.Install(remoteHost));
                    PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                    serverApps.Add(dlPacketSinkHelper.Install(ue));
                    sinks.Add(serverApps);
                    lastTotalRx.push_back(0);
                }
            }
            
            std::cout << "UE " << u << " configured with IP " 
                      << ueIpIface.GetAddress(u) << " and port " << dlPort << std::endl;
            
            // The filter for the low-latency traffic
            Ptr<NrEpcTft> tft = Create<NrEpcTft>();
            NrEpcTft::PacketFilter dlpf;
            dlpf.localPortStart = dlPort;
            dlpf.localPortEnd = dlPort;
            dlpf.direction = NrEpcTft::DOWNLINK;
            tft->Add(dlpf);

            // The bearer that will carry low latency traffic
            NrEpsBearer bearer(NrEpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            nrHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);
            // NS_LOG_UNCOND("Bearer activated for UE " << u << " with IP " << ueIpIface.GetAddress(u));
    
            Time startTime = Seconds(startTimeSeconds->GetValue());
            std::cout<< Simulator::Now ().GetSeconds () << " " << " UE IMSI " << u << "startTime: "<<startTime << std::endl;
            serverApps.Start(startTime);
            clientApps.Start(startTime);

        }
    }

    
    Simulator::Stop(Seconds(simTime));

    // serverApps.Stop(Seconds(simTime));
    // clientApps.Stop(Seconds(simTime - 0.2));

    nrHelper->EnableTraces();


    // Config::Connect("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUePhy/ReportUeMeasurements",
    //                 MakeCallback(&CqiTrace)
    // );


    // Register callback for UE RRC connection established event
    Config::Connect("/NodeList/*/DeviceList/*/NrUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::NrGnbNetDevice/NrGnbRrc/NotifyConnectionRelease",
                    MakeCallback(&NotifyConnectionReleasedUe));

    // mobility callbacks
    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback(&CourseChange));
    
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::NrNetDevice/$ns3::NrUeNetDevice/"
                "ComponentCarrierMapUe/*/NrUePhy/ReportUeMeasurements",
                MakeBoundCallback(&ReportUeMeasurementsCallback));




    Simulator::Schedule (Seconds (0.4), &CalculateThroughput,100);

    for (uint32_t i = 0; i < ueNetDev.GetN(); i++)
    {   Ptr<NrSpectrumPhy> ueSpectrumPhy =
            DynamicCast<NrUeNetDevice>(ueNetDev.Get(i))->GetPhy(0)->GetSpectrumPhy();
        Ptr<NrInterference> ueInterference = ueSpectrumPhy->GetNrInterference();

        ueInterference->TraceConnectWithoutContext(
            "RssiPerProcessedChunk",
            MakeBoundCallback(&UeRssiPerProcessedChunkTrace, i));
    }

    std::cout << "Starting simulation..." << std::endl;
    Simulator::Run();
    std::cout << "Simulation finished." << std::endl;

    Ptr<NrUeNetDevice> nrUeDev = DynamicCast<NrUeNetDevice>(ueNetDev.Get(0));
    Ptr<NrPhy> uePhy = nrUeDev->GetPhy(0);
    numRbs_global = uePhy->GetRbNum();
    std::cout << "numRbs = " << uePhy->GetRbNum() << std::endl;

    std::cout << "m_channelBandwidth = " << uePhy->GetChannelBandwidth() << std::endl;


    std::cout << "m_subcarrierSpacing = " << uePhy->GetSubcarrierSpacing() << std::endl;

    nrHelper = nullptr;
    Simulator::Destroy();

    return 0;
}