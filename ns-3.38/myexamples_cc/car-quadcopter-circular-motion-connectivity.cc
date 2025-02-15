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
    std::string outputFileName = "car-quadcopter-circular-motion-connectivity.json";

    CommandLine cmd;
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");

    NodeContainer nodes;
    nodes.Create(3);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(nodes);

    double radius = 20.0;
    int numWaypoints = 72;

    Ptr<WaypointMobilityModel> quadcopterMobility = nodes.Get(0)->GetObject<WaypointMobilityModel>();
    for (int i = 0; i <= numWaypoints; ++i)
    {
        double angle = -2 * M_PI * i / numWaypoints;
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        double time = i * (duration.GetSeconds() / numWaypoints);
        quadcopterMobility->AddWaypoint(Waypoint(Seconds(time), Vector(x, y, 10.0)));
    }

    Ptr<WaypointMobilityModel> carMobility = nodes.Get(2)->GetObject<WaypointMobilityModel>();
    for (int i = 0; i <= numWaypoints; ++i)
    {
        double angle = -2 * M_PI * i / numWaypoints;
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        double time = i * (duration.GetSeconds() / numWaypoints);
        carMobility->AddWaypoint(Waypoint(Seconds(time), Vector(x, y, 0.0)));
    }

    Ptr<MobilityModel> serverMobility = nodes.Get(1)->GetObject<MobilityModel>();
    serverMobility->SetPosition(Vector(0.0, 0.0, 0.0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));

    auto quadLink = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
    auto carLink = pointToPoint.Install(nodes.Get(2), nodes.Get(1));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto quadInterfaces = address.Assign(quadLink);

    address.SetBase("10.1.2.0", "255.255.255.0");
    auto carInterfaces = address.Assign(carLink);

    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};
    auto serverApp = echoServer.Install(nodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClientQuad(quadInterfaces.GetAddress(1), echoPort);
    echoClientQuad.SetAttribute("MaxPackets", UintegerValue(10000u));
    echoClientQuad.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClientQuad.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientAppQuad = echoClientQuad.Install(nodes.Get(0));
    clientAppQuad.Start(Seconds(1.0));
    clientAppQuad.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClientCar(carInterfaces.GetAddress(1), echoPort);
    echoClientCar.SetAttribute("MaxPackets", UintegerValue(10000u));
    echoClientCar.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClientCar.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientAppCar = echoClientCar.Install(nodes.Get(2));
    clientAppCar.Start(Seconds(1.0));
    clientAppCar.Stop(duration - Seconds(1.0));

    auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));

    auto clientThroughputQuad = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Quadcopter Client Throughput (TX)");
    clientThroughputQuad->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientAppQuad.Get(0)->TraceConnectWithoutContext("Tx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughputQuad));

    auto clientThroughputCar = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Car Client Throughput (TX)");
    clientThroughputCar->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    clientAppCar.Get(0)->TraceConnectWithoutContext("Tx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughputCar));

    auto serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteServerThroughput, serverThroughput));

    netsimulyzer::NodeConfigurationHelper nodeHelper{orchestrator};

    nodeHelper.Set("Model", netsimulyzer::models::QUADCOPTER_UAV_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::BLUE});
    nodeHelper.Install(nodes.Get(0));

    nodeHelper.Set("Model", netsimulyzer::models::CELL_TOWER_POLE_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::GREEN});
    nodeHelper.Install(nodes.Get(1));

    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>{netsimulyzer::RED});
    nodeHelper.Install(nodes.Get(2));

    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}


