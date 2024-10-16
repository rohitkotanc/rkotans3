#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/netsimulyzer-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <string>
#include "ns3/netsimulyzer-module.h"
#include "ns3/netanim-module.h"
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
void WriteServerThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}
int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "two-car-constant-connectivity-through-distance.json";
    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");
    NodeContainer nodes{2u};
    auto positions = CreateObject<ListPositionAllocator>();
    positions->Add(Vector3D{10.0, 5.0, 0.0});
    positions->Add(Vector3D{5.0, 5.0, 0.0});
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.SetPositionAllocator(positions);
    mobility.Install(nodes);
Ptr<ConstantVelocityMobilityModel> mobility0 = nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
mobility0->SetVelocity(Vector(0.0, -5.0, 0.0));
Ptr<ConstantVelocityMobilityModel> mobility1 = nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
mobility1->SetVelocity(Vector(0.0, -5.0, 0.0));
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
    auto netDevices = pointToPoint.Install(nodes);
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto interfaces = address.Assign(netDevices);
    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};
    auto serverApp = echoServer.Install(nodes.Get(1u));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1u), echoPort);
    double initialInterval = 0.1;
    double intervalStep = 0.05;
    double currentInterval = initialInterval;
    echoClient.SetAttribute("MaxPackets", UintegerValue(10000u));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(currentInterval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientApp = echoClient.Install(nodes.Get(0u));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));
    Simulator::Schedule(Seconds(2.0), &AdjustPacketInterval, std::ref(echoClient), std::ref(currentInterval), intervalStep, duration - Seconds(2.0));
    auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));
    netsimulyzer::NodeConfigurationHelper nodeHelper{orchestrator};
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::BLUE});
    nodeHelper.Install(nodes.Get(0u));
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::RED});
    nodeHelper.Install(nodes.Get(1u));
    auto clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (TX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientThroughput->SetAttribute("Interval", TimeValue(Seconds(1.0)));
    clientThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    clientThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    clientApp.Get(0u)->TraceConnectWithoutContext("Tx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughput));
    auto serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverThroughput->SetAttribute("Interval", TimeValue(Seconds(1.0)));
    serverThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    serverThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    serverApp.Get(0u)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteServerThroughput, serverThroughput));
    auto collection = CreateObject<netsimulyzer::SeriesCollection>(orchestrator);
    collection->SetAttribute("Name", StringValue("Client and Server Throughput (TX and RX)"));
    collection->SetAttribute("HideAddedSeries", BooleanValue(false));
    StringValue name;
    serverThroughput->GetSeries()->GetXAxis()->GetAttribute("Name", name);
    collection->GetXAxis()->SetAttribute("Name", name);
    serverThroughput->GetSeries()->GetYAxis()->GetAttribute("Name", name);
    collection->GetYAxis()->SetAttribute("Name", name);
    collection->Add(clientThroughput->GetSeries());
    collection->Add(serverThroughput->GetSeries());
    AnimationInterface anim("throughput-sink-example-netsimulyzer.xml");
    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
}
