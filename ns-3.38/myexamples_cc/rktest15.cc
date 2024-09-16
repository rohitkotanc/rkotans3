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
#include <functional>
using namespace ns3;
void AdjustNetworkAttributes(Ptr<PointToPointNetDevice> device, Ptr<PointToPointChannel> channel, double currentDelay, double currentDataRate, double delayStep, double dataRateStep, Time endTime)
{
    currentDelay += delayStep;
    channel->SetAttribute("Delay", TimeValue(MilliSeconds(currentDelay)));
    currentDataRate -= dataRateStep;
    std::string dataRateString = std::to_string(currentDataRate) + "Mbps";
    device->SetAttribute("DataRate", StringValue(dataRateString));
    std::cout << "At time " << Simulator::Now().GetSeconds() << " seconds, delay is "
              << currentDelay << " ms and data rate is "
              << currentDataRate << " Mbps." << std::endl;
    if (Simulator::Now() < endTime && currentDataRate > 0.0)
    {
        Simulator::Schedule(Seconds(1.0), std::bind(&AdjustNetworkAttributes, device, channel, currentDelay, currentDataRate, delayStep, dataRateStep, endTime));
    }
}
void WriteServerThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}
int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "rktest15.json";
    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);
    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");
    NodeContainer nodes{2u};
    auto positions = CreateObject<ListPositionAllocator>();
    positions->Add(Vector3D{5.0, 5.0, 0.0});
    positions->Add(Vector3D{15.0, 5.0, 0.0});
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.SetPositionAllocator(positions);
    mobility.Install(nodes);
    Ptr<ConstantVelocityMobilityModel> mobility0 = nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
    mobility0->SetVelocity(Vector(0.0, 2.0, 0.0));
    Ptr<ConstantVelocityMobilityModel> mobility1 = nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
    mobility1->SetVelocity(Vector(0.0, -2.0, 0.0));
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));
    auto netDevices = pointToPoint.Install(nodes);
    Ptr<PointToPointNetDevice> device = DynamicCast<PointToPointNetDevice>(netDevices.Get(0));
    Ptr<PointToPointChannel> channel = DynamicCast<PointToPointChannel>(device->GetChannel());
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
    echoClient.SetAttribute("MaxPackets", UintegerValue(10000u));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientApp = echoClient.Install(nodes.Get(0u));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));
    double initialDelay = 0.0;
    double initialDataRate = 100.0;
    double delayStep = 1.0;
    double dataRateStep = 5.0;
    Simulator::Schedule(Seconds(2.0), std::bind(&AdjustNetworkAttributes, device, channel, initialDelay, initialDataRate, delayStep, dataRateStep, duration - Seconds(2.0)));
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
    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
}
