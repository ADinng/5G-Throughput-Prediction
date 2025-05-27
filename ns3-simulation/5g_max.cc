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
#include "ns3/string.h"
#include "ns3/callback.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("NRSimpleScenario_03");

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
    //Simulator::Schedule(MilliSeconds(inter), &CalculateThroughput, inter);
    Simulator::Schedule(Seconds(1.0), &CalculateThroughput, inter);
}



int
main(int argc, char* argv[])
{
    uint32_t nGnbSitesX = 1;        
    uint32_t nGnbSites = 2* nGnbSitesX;
    double interSiteDistance = 200;  
    //double areaMarginFactor = 0.5;    
    double ueDensity = 0.0001;  
    uint32_t numBearersPerUe = 1;  

    std::string scenario = "UMa"; // scenario
    double frequency = 28e9;      // central frequency
    double bandwidth = 100e6;     // bandwidth
    //double mobility = false;      // whether to enable mobility
    double simTime = 5;           // in second
    //double speed = 1;             // in m/s for walking UT.
    bool logging = false; // whether to enable logging from the simulation, another option is by
                         // exporting the NS_LOG environment variable
    double hBS;          // base station antenna height in meters
    double hUT;          // user antenna height in meters
    double txPower = 40; // txPower

    /// Minimum speed value of macro UE with random waypoint model [m/s].
    uint16_t outdoorUeMinSpeed = 15.0;
    /// Maximum speed value of macro UE with random waypoint model [m/s].
    uint16_t outdoorUeMaxSpeed = 20.0;
    // set mobility model: steady, gauss
    std::string mobilityType = "steady";


    enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa;

    // enable logging
    if (logging)
    {
        // LogComponentEnable ("ThreeGppSpectrumPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ThreeGppChannelModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ChannelConditionModel", LOG_LEVEL_ALL);
        LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
        // LogComponentEnable ("NrRlcUm", LOG_LEVEL_LOGIC);
        // LogComponentEnable ("NrPdcp", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_ALL);

    }

    /*
     * Default values for the simulation. We are progressively removing all
     * the instances of SetDefault, but we need it for legacy code (LTE)
     */
    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));

    // set mobile device and base station antenna heights in meters, according to the chosen
    // scenario
    if (scenario == "RMa")
    {
        hBS = 35;
        hUT = 1.5;
        scenarioEnum = BandwidthPartInfo::RMa;
    }
    else if (scenario == "UMa")
    {
        hBS = 25;
        hUT = 1.5;
        scenarioEnum = BandwidthPartInfo::UMa;
    }
    else if (scenario == "UMi-StreetCanyon")
    {
        hBS = 10;
        hUT = 1.5;
        scenarioEnum = BandwidthPartInfo::UMi_StreetCanyon;
    }
    else if (scenario == "InH-OfficeMixed")
    {
        hBS = 3;
        hUT = 1;
        scenarioEnum = BandwidthPartInfo::InH_OfficeMixed;
    }
    else if (scenario == "InH-OfficeOpen")
    {
        hBS = 3;
        hUT = 1;
        scenarioEnum = BandwidthPartInfo::InH_OfficeOpen;
    }
    else
    {
        NS_ABORT_MSG("Scenario not supported. Choose among 'RMa', 'UMa', 'UMi-StreetCanyon', "
                     "'InH-OfficeMixed', and 'InH-OfficeOpen'.");
    }

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

    // //position the base stations
    // Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    // gnbPositionAlloc->Add(Vector(0.0, 0.0, hBS));
    // gnbPositionAlloc->Add(Vector(0.0, 200.0, hBS));

    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    for (uint32_t i = 0; i < nGnbSites; ++i)
    {
        uint32_t x = i % nGnbSitesX;
        uint32_t y = i / nGnbSitesX;
        double posX = x * interSiteDistance;
        double posY = y * interSiteDistance;
        gnbPositionAlloc->Add(Vector(posX, posY, hBS));
        NS_LOG_UNCOND("gNB " << i << " at position (" << posX << ", " << posY << ", " << hBS << ")");
    }

    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");


    gnbMobility.SetPositionAllocator(gnbPositionAlloc);
    gnbMobility.Install(gnbNodes);

    // // position the mobile terminals and enable the mobility
    // MobilityHelper uemobility;
    // uemobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    // uemobility.Install(ueNodes);

    // if (mobility)
    // {
    //     ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(
    //         Vector(90, 15, hUT)); // (x, y, z) in m
    //     ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
    //         Vector(0, speed, 0)); // move UE1 along the y axis

    //     ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(
    //         Vector(30, 50.0, hUT)); // (x, y, z) in m
    //     ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
    //         Vector(-speed, 0, 0)); // move UE2 along the x axis
    // }
    // else
    // {
    //     ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(90, 15, hUT));
    //     ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));

    //     ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(30, 50.0, hUT));
    //     ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));
    // }


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

    // Create NR simulation helpers
    Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(nrEpcHelper);

    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // have a single band, and that band is composed of a single component carrier

    /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
     * a single BWP per CC and a single BWP in CC.
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

    // Configure scheduler
    nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());

    // Antennas for the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Antennas for the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));


    NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(ueNodes, allBwps);

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    for (uint32_t i = 0; i < gnbNodes.GetN(); ++i)
    {
        nrHelper->GetGnbPhy(gnbNetDev.Get(i), 0)->SetTxPower(txPower);
    }

    // When all the configuration is done, explicitly call UpdateConfig ()
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
    Ptr<Node> pgw = nrEpcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
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
    internet.Install(ueNodes);

    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));

    // assign IP address to UEs, and install UDP downlink applications
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    uint16_t dlPort = 10000;  
    // uint16_t ulPort = 20000;  

    std::cout << "Configuring and starting applications..." << std::endl;
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(1));

    numSinks = ueNodes.GetN();
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(nrEpcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ++dlPort;

            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
            sinks.Add(serverApps.Get(u));
            lastTotalRx.push_back(0);

            UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
            dlClient.SetAttribute("Interval", TimeValue(MicroSeconds(1000))); 
            dlClient.SetAttribute("MaxPackets", UintegerValue(1000));
            // dlClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF)); // continue sending
            dlClient.SetAttribute("PacketSize", UintegerValue(1500)); 
            
            clientApps.Add(dlClient.Install(remoteHost));

            std::cout << "UE " << u << " configured with IP " 
                      << ueIpIface.GetAddress(u) << " and port " << dlPort << std::endl;
            

        }
    }


    std::cout << "Attaching UEs to gNBs..." << std::endl;
    nrHelper->AttachToClosestGnb(ueNetDev, gnbNetDev);
    std::cout << "UEs attached successfully" << std::endl;
    

    std::cout << "Starting server applications..." << std::endl;
    serverApps.Start(Seconds(0.4));
    std::cout << "Starting client applications..." << std::endl;
    clientApps.Start(Seconds(0.4));
    
    serverApps.Stop(Seconds(simTime));
    clientApps.Stop(Seconds(simTime - 0.2));


    nrHelper->EnableTraces();

    Simulator::Schedule (Seconds (0.4), &CalculateThroughput,100);
    Simulator::Stop(Seconds(simTime));

    std::cout << "Starting simulation..." << std::endl;
    Simulator::Run();
    std::cout << "Simulation finished." << std::endl;

    nrHelper = nullptr;
    Simulator::Destroy();
    return 0;
}