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
#include <fstream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("NRSimpleScenario_01");


uint32_t numSinks;
ApplicationContainer sinks;
std::vector<int> lastTotalRx;
std::ofstream throughputFile; 

int
main(int argc, char* argv[])
{
    std::string scenario = "UMa"; // scenario
    double frequency = 28e9;      // central frequency
    double bandwidth = 100e6;     // bandwidth
    double mobility = false;      // whether to enable mobility
    double simTime = 3;           // in second
    double speed = 1;             // in m/s for walking UT.
    bool logging = false; // whether to enable logging from the simulation, another option is by
                         // exporting the NS_LOG environment variable
    double hBS;          // base station antenna height in meters
    double hUT;          // user antenna height in meters
    double txPower = 40; // txPower
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

    // create base stations and mobile terminals
    NodeContainer gnbNodes;
    NodeContainer ueNodes;
    gnbNodes.Create(2);
    ueNodes.Create(2);

    // position the base stations
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    gnbPositionAlloc->Add(Vector(0.0, 0.0, hBS));
    gnbPositionAlloc->Add(Vector(0.0, 80.0, hBS));
    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobility.SetPositionAllocator(gnbPositionAlloc);
    gnbMobility.Install(gnbNodes);

    // position the mobile terminals and enable the mobility
    MobilityHelper uemobility;
    uemobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    uemobility.Install(ueNodes);

    if (mobility)
    {
        ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(
            Vector(90, 15, hUT)); // (x, y, z) in m
        ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(0, speed, 0)); // move UE1 along the y axis

        ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(
            Vector(30, 50.0, hUT)); // (x, y, z) in m
        ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(-speed, 0, 0)); // move UE2 along the x axis
    }
    else
    {
        ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(90, 15, hUT));
        ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));

        ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(30, 50.0, hUT));
        ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));
    }

    /*
     * Create NR simulation helpers
     */
    Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(nrEpcHelper);

    /*
     * Spectrum configuration. We create a single operational band and configure the scenario.
     */
    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // in this example we have a single band, and that band is
                                    // composed of a single component carrier

    /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
     * a single BWP per CC and a single BWP in CC.
     *
     * Hence, the configured spectrum is:
     *
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

    // install nr net devices
    NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(ueNodes, allBwps);

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    nrHelper->GetGnbPhy(gnbNetDev.Get(0), 0)->SetTxPower(txPower);
    nrHelper->GetGnbPhy(gnbNetDev.Get(1), 0)->SetTxPower(txPower);

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
    uint16_t dlPort = 1234;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(nrEpcHelper->GetUeDefaultGatewayAddress(), 1);

        UdpServerHelper dlPacketSinkHelper(dlPort);
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MicroSeconds(100)));
        //dlClient.SetAttribute("MaxPackets", UintegerValue(10));
        dlClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF)); // continue sending
        dlClient.SetAttribute("PacketSize", UintegerValue(1500));
        
        clientApps.Add(dlClient.Install(remoteHost));
    }

    // attach UEs to the closest gNB
    nrHelper->AttachToClosestGnb(ueNetDev, gnbNetDev);

    // start server and client apps
    serverApps.Start(Seconds(0.4));
    clientApps.Start(Seconds(0.4));
    serverApps.Stop(Seconds(simTime));
    clientApps.Stop(Seconds(simTime - 0.2));

    // enable the traces provided by the nr module
    nrHelper->EnableTraces();



    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    return EXIT_SUCCESS;
}

