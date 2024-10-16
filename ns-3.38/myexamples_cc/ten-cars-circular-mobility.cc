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
void WriteServerThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}
int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "ten-cars-circular-mobility.json";
    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");
    NodeContainer nodes{2u};
    auto positions = CreateObject<ListPositionAllocator>();
    positions->Add(Vector3D{-1.0, 5.0, 0.0});
    positions->Add(Vector3D{1.0, 5.0, 0.0});
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positions);
    mobility.Install(nodes);
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
    echoClient.SetAttribute("MaxPackets", UintegerValue(10'000u));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientApp = echoClient.Install(nodes.Get(0u));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));
    auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(false));
    netsimulyzer::NodeConfigurationHelper nodeHelper{orchestrator};
    nodeHelper.Set("Model", netsimulyzer::models::SMARTPHONE_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::BLUE});
    nodeHelper.Install(nodes.Get(0u));
    nodeHelper.Set("Model", netsimulyzer::models::SERVER_VALUE);
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
