#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/netsimulyzer-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/netanim-module.h>
#include <iostream>
using namespace ns3;
void AdjustPacketInterval(UdpEchoClientHelper& client, double& currentInterval, double intervalStep, Time endTime)
{
    currentInterval += intervalStep;
    if (Simulator::Now() < endTime)
    {
        client.SetAttribute("Interval", TimeValue(Seconds(currentInterval)));
        Simulator::Schedule(Seconds(1.0), &AdjustPacketInterval, std::ref(client), std::ref(currentInterval), intervalStep, endTime);
    }
}
void DropConnectivityAndPauseClient(Ptr<NetDevice> clientDevice, Ptr<NetDevice> serverDevice)
{
    Ptr<PointToPointNetDevice> clientP2PDevice = DynamicCast<PointToPointNetDevice>(clientDevice);
    Ptr<PointToPointNetDevice> serverP2PDevice = DynamicCast<PointToPointNetDevice>(serverDevice);
    clientP2PDevice->SetAttribute("DataRate", StringValue("1bps"));
    serverP2PDevice->SetAttribute("DataRate", StringValue("1bps"));
    std::cout << "Connectivity dropped at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
}
void RestoreConnectivityAndResumeClient(Ptr<NetDevice> clientDevice, Ptr<NetDevice> serverDevice)
{
    Ptr<PointToPointNetDevice> clientP2PDevice = DynamicCast<PointToPointNetDevice>(clientDevice);
    Ptr<PointToPointNetDevice> serverP2PDevice = DynamicCast<PointToPointNetDevice>(serverDevice);
    clientP2PDevice->SetAttribute("DataRate", StringValue("100Mbps"));
    serverP2PDevice->SetAttribute("DataRate", StringValue("100Mbps"));
    std::cout << "Connectivity restored at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
}
void WriteThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}
int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "three-car-constant-connectivity.json";
    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");
    NodeContainer nodes;
    nodes.Create(3);
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector3D(25.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(5.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(0.0, -10.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);
    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -2.5, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -2.5, 0.0));
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
    NetDeviceContainer netDevices = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
    InternetStackHelper stack;
    stack.Install(nodes.Get(0));
    stack.Install(nodes.Get(1));
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(netDevices);
    const uint16_t echoPort = 9;
    UdpEchoServerHelper echoServer(echoPort);
    ApplicationContainer serverApp = echoServer.Install(nodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), echoPort);
    double initialInterval = 0.1;
    double intervalStep = 0.05;
    echoClient.SetAttribute("MaxPackets", UintegerValue(10000));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(initialInterval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp = echoClient.Install(nodes.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));
    Simulator::Schedule(Seconds(2.0), &AdjustPacketInterval, std::ref(echoClient), std::ref(initialInterval), intervalStep, duration - Seconds(2.0));
    Simulator::Schedule(Seconds(6.0), &DropConnectivityAndPauseClient, netDevices.Get(0), netDevices.Get(1));
    Simulator::Schedule(Seconds(7.0), &RestoreConnectivityAndResumeClient, netDevices.Get(0), netDevices.Get(1));
    Ptr<netsimulyzer::Orchestrator> orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));
    netsimulyzer::NodeConfigurationHelper nodeHelper(orchestrator);
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::BLUE));
    nodeHelper.Install(nodes.Get(0));
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::RED));
    nodeHelper.Install(nodes.Get(1));
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::GREEN));
    nodeHelper.Install(nodes.Get(2));
    Ptr<netsimulyzer::ThroughputSink> clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (TX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientApp.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&WriteThroughput, clientThroughput));
    Ptr<netsimulyzer::ThroughputSink> serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, serverThroughput));
    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
