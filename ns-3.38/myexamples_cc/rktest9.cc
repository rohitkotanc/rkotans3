#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/netsimulyzer-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <cmath>
#include <string>
#include "ns3/netsimulyzer-module.h"
#include "ns3/netanim-module.h"
using namespace ns3;
void WriteServerThroughput(Ptr<netsimulyzer::ThroughputSink> sink,
                           Ptr<const Packet> packet,
                           const Address&,
                           const Address&)
{
    sink->AddPacket(packet);
}
int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "rktest9.json";
    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");
    NodeContainer nodes{4u};
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(nodes);
    double radius = 20.0;
    double speedCar = 5.0;
    double speedQuad = 12.0;
    int numWaypoints = 36;
    Ptr<WaypointMobilityModel> quadcopterMobility = nodes.Get(0)->GetObject<WaypointMobilityModel>();
    Ptr<WaypointMobilityModel> carMobility = nodes.Get(2)->GetObject<WaypointMobilityModel>();
    for (int i = 0; i < numWaypoints; ++i)
    {
        double angle = 2 * M_PI * i / numWaypoints;
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        double timeCar = i * (2 * M_PI * radius / speedCar) / numWaypoints;
        double timeQuad = i * (2 * M_PI * radius / speedQuad) / numWaypoints;
        quadcopterMobility->AddWaypoint(Waypoint(Seconds(timeQuad), Vector(x, y, 40.0)));
        carMobility->AddWaypoint(Waypoint(Seconds(timeCar), Vector(x, y, 0.0)));
    }
nodes.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));
nodes.Get(3)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
    auto netDevices1 = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
    auto netDevices2 = pointToPoint.Install(nodes.Get(2), nodes.Get(1));
    auto netDevices3 = pointToPoint.Install(nodes.Get(0), nodes.Get(3));
    auto netDevices4 = pointToPoint.Install(nodes.Get(2), nodes.Get(3));
    auto netDevices5 = pointToPoint.Install(nodes.Get(1), nodes.Get(3));
    auto netDevices6 = pointToPoint.Install(nodes.Get(3), nodes.Get(1));
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto interfaces1 = address.Assign(netDevices1);
    address.SetBase("10.1.2.0", "255.255.255.0");
    auto interfaces2 = address.Assign(netDevices2);
    address.SetBase("10.1.3.0", "255.255.255.0");
    auto interfaces3 = address.Assign(netDevices3);
    address.SetBase("10.1.4.0", "255.255.255.0");
    auto interfaces4 = address.Assign(netDevices4);
    address.SetBase("10.1.5.0", "255.255.255.0");
    auto interfaces5 = address.Assign(netDevices5);
    address.SetBase("10.1.6.0", "255.255.255.0");
    auto interfaces6 = address.Assign(netDevices6);
    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};
    auto serverApp = echoServer.Install(nodes.Get(1u));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));
    UdpEchoClientHelper echoClient(interfaces6.GetAddress(1u), echoPort);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10'000u));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientApp = echoClient.Install(nodes.Get(0u));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));
    auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));
    netsimulyzer::NodeConfigurationHelper nodeHelper{orchestrator};
    nodeHelper.Set("Model", netsimulyzer::models::QUADCOPTER_UAV_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::BLUE});
    nodeHelper.Install(nodes.Get(0u));
    nodeHelper.Set("Model", netsimulyzer::models::CELL_TOWER_POLE_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::GREEN});
    nodeHelper.Install(nodes.Get(1u));
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::RED});
    nodeHelper.Install(nodes.Get(2u));
    nodeHelper.Set("Model", netsimulyzer::models::CELL_TOWER_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::BLACK});
    nodeHelper.Install(nodes.Get(3u));
    auto clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (TX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientThroughput->SetAttribute("Interval", TimeValue(Seconds(1.0)));
    clientThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    clientThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    clientApp.Get(0u)->TraceConnectWithoutContext("Tx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughput));
    auto serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (TX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    serverThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    serverApp.Get(0u)->TraceConnectWithoutContext("TxWithAddresses", MakeBoundCallback(&WriteServerThroughput, serverThroughput));
    auto collection = CreateObject<netsimulyzer::SeriesCollection>(orchestrator);
    collection->SetAttribute("Name", StringValue("Client and Server Throughput (TX)"));
    collection->SetAttribute("HideAddedSeries", BooleanValue(false));
    StringValue name;
    serverThroughput->GetSeries()->GetXAxis()->GetAttribute("Name", name);
    collection->GetXAxis()->SetAttribute("Name", name);
    serverThroughput->GetSeries()->GetYAxis()->GetAttribute("Name", name);
    collection->GetYAxis()->SetAttribute("Name", name);
    collection->Add(clientThroughput->GetSeries());
    collection->Add(serverThroughput->GetSeries());
    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
}
