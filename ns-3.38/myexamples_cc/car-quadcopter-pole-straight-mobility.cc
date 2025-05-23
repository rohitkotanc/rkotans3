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

void WriteServerThroughput(Ptr<netsimulyzer::ThroughputSink> sink,
                           Ptr<const Packet> packet,
                           const Address&, const Address&)
{
    sink->AddPacket(packet);
}

void DropConnectivity(Ptr<RateErrorModel> clientErrorModel, Ptr<RateErrorModel> serverErrorModel)
{
    std::cout << " Connectivity permanently dropped at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
    clientErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
    serverErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
}

int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "car-quadcopter-pole-straight-mobility.json";
    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName",
                 "The name of the file to write the NetSimulyzer trace info",
                 outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");

    NodeContainer nodes{3u};
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(-10.0),
                                  "MinY", DoubleValue(10.0),
                                  "DeltaX", DoubleValue(20.0),
                                  "DeltaY", DoubleValue(0.0),
                                  "GridWidth", UintegerValue(2),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);
    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(3.0, 0.0, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));
    nodes.Get(2)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(-3.0, 0.0, 0.0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
    auto netDevices = pointToPoint.Install(nodes.Get(0), nodes.Get(2));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto interfaces = address.Assign(netDevices);

    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};
    auto serverApp = echoServer.Install(nodes.Get(0u));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(0.1));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(0u), echoPort);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10'000u));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientApp = echoClient.Install(nodes.Get(2u));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(0.1));

    Ptr<RateErrorModel> clientErrorModel = CreateObject<RateErrorModel>();
    Ptr<RateErrorModel> serverErrorModel = CreateObject<RateErrorModel>();

    netDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(clientErrorModel));
    netDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(serverErrorModel));

    std::cout << " Scheduling connectivity drop at 3.0s" << std::endl;
    Simulator::Schedule(Seconds(3.0), &DropConnectivity, clientErrorModel, serverErrorModel);

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

    auto clientThroughput =
        CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (RX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientThroughput->SetAttribute("Interval", TimeValue(Seconds(1.0)));
    clientThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    clientThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    clientApp.Get(0u)->TraceConnectWithoutContext(
        "Rx",
        MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughput));

    auto serverThroughput =
        CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    serverThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    serverApp.Get(0u)->TraceConnectWithoutContext(
        "RxWithAddresses",
        MakeBoundCallback(&WriteServerThroughput, serverThroughput));

    auto collection = CreateObject<netsimulyzer::SeriesCollection>(orchestrator);
    collection->SetAttribute("Name", StringValue("Client and Server Throughput (RX)"));
    collection->SetAttribute("HideAddedSeries", BooleanValue(false));
    collection->Add(clientThroughput->GetSeries());
    collection->Add(serverThroughput->GetSeries());

    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
}


