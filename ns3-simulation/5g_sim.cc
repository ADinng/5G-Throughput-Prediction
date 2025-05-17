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

#include <iomanip>
#include <ios>
#include <map>
#include <string>
#include <vector>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("NRSimpleScenario");



int main(int argc, char *argv[])

{
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    nrHelper->SetEpcHelper(epcHelper);

    // Create gNB and UE
    NodeContainer gNbNodes;
    gNbNodes.Create(1);
    NodeContainer ueNodes;
    ueNodes.Create(1);

    // Install protocol stack
    InternetStackHelper internet;
    internet.Install(ueNodes);

    // Configure basic mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(gNbNodes);
    mobility.Install(ueNodes);

    // Configure NR frequency band and bandwidth
    double frequency = 28e9;      // central frequency
    double bandwidth = 100e6;     // bandwidth
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

    // Configure UDP downlink application (RemoteHost -> UE)
    uint16_t dlPort = 1234;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        // Install UDP server on UE
        UdpServerHelper dlPacketSinkHelper(dlPort);
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

        // Install UDP client on RemoteHost
        UdpClientHelper dlClientHelper(ueIpIface.GetAddress(u), dlPort);
        dlClientHelper.SetAttribute("Interval", TimeValue(MicroSeconds(1000)));
        dlClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
        dlClientHelper.SetAttribute("PacketSize", UintegerValue(1500));
        clientApps.Add(dlClientHelper.Install(remoteHost));
    }

    // Start and stop applications
    serverApps.Start(Seconds(0.4));
    clientApps.Start(Seconds(0.4));
    serverApps.Stop(Seconds(10.0));
    clientApps.Stop(Seconds(9.8));

    // Enable NR module traces (PHY, MAC, RLC, PDCP, etc.)
    nrHelper->EnableTraces();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Print UDP server received packets for verification
    Ptr<UdpServer> serverApp = serverApps.Get(0)->GetObject<UdpServer>();
    uint64_t receivedPackets = serverApp->GetReceived();
    std::cout << "UE 0 received UDP packets: " << receivedPackets << std::endl;
    
    Simulator::Destroy();
    return 0;
}
