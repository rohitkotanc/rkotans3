#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/buildings-module.h>
#include <ns3/netsimulyzer-module.h>

using namespace ns3;

void WriteThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}

void DropConnectivity(Ptr<RateErrorModel> car1ClientErrorModel, Ptr<RateErrorModel> car2ClientErrorModel, Ptr<RateErrorModel> serverErrorModel)
{
    car1ClientErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
    car2ClientErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
    serverErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
    std::cout << "Connectivity dropped at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
}

void RestoreConnectivity(Ptr<RateErrorModel> car1ClientErrorModel, Ptr<RateErrorModel> car2ClientErrorModel, Ptr<RateErrorModel> serverErrorModel)
{
    car1ClientErrorModel->SetAttribute("ErrorRate", DoubleValue(0.0));
    car2ClientErrorModel->SetAttribute("ErrorRate", DoubleValue(0.0));
    serverErrorModel->SetAttribute("ErrorRate", DoubleValue(0.0));
    std::cout << "Connectivity restored at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
}

int main(int argc, char* argv[])
{
    double durationUser = 30.0;
    std::string outputFileName = "two-car-uav-traffic-connectivity.json";

    CommandLine cmd;
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);

    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");

    NodeContainer nodes;
    nodes.Create(4);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector3D(55.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(-25.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(15.0, -245.0, 0.0));
    positionAlloc->Add(Vector3D(15.0, 5.0, 50.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -5.0, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -5.0, 0.0));
    nodes.Get(2)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 5.0, 0.0));
    nodes.Get(3)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -5.0, 0.0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));

    auto car1Link = pointToPoint.Install(nodes.Get(0), nodes.Get(3));
    auto car2Link = pointToPoint.Install(nodes.Get(1), nodes.Get(3));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto car1Interfaces = address.Assign(car1Link);

    address.SetBase("10.1.2.0", "255.255.255.0");
    auto car2Interfaces = address.Assign(car2Link);

    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};
    auto serverApp = echoServer.Install(nodes.Get(3));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClientCar1(car1Interfaces.GetAddress(1), echoPort);
    echoClientCar1.SetAttribute("MaxPackets", UintegerValue(10000u));
    echoClientCar1.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClientCar1.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientAppCar1 = echoClientCar1.Install(nodes.Get(0));
    clientAppCar1.Start(Seconds(1.0));
    clientAppCar1.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClientCar2(car2Interfaces.GetAddress(1), echoPort);
    echoClientCar2.SetAttribute("MaxPackets", UintegerValue(10000u));
    echoClientCar2.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClientCar2.SetAttribute("PacketSize", UintegerValue(1024u));
    auto clientAppCar2 = echoClientCar2.Install(nodes.Get(1));
    clientAppCar2.Start(Seconds(1.0));
    clientAppCar2.Stop(duration - Seconds(1.0));

    Ptr<netsimulyzer::Orchestrator> orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));


Ptr<RateErrorModel> car1ClientErrorModel = CreateObject<RateErrorModel>();
Ptr<RateErrorModel> car2ClientErrorModel = CreateObject<RateErrorModel>();
Ptr<RateErrorModel> serverErrorModel = CreateObject<RateErrorModel>();


car1Link.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(car1ClientErrorModel));
car2Link.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(car2ClientErrorModel));
car1Link.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(serverErrorModel));
car2Link.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(serverErrorModel));


Simulator::Schedule(Seconds(5.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(6.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(17.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(18.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);



    netsimulyzer::NodeConfigurationHelper nodeHelper(orchestrator);
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::BLUE));
nodeHelper.Install(nodes.Get(0));

nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::RED));
nodeHelper.Install(nodes.Get(1));

nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::GREEN));
nodeHelper.Install(nodes.Get(2));

nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::YELLOW));
nodeHelper.Install(nodes.Get(3));



    Ptr<Building> building1 = CreateObject<Building>();
    building1->SetBoundaries(Box(15.0, 20.0, -25.0, -20.0, 0.0, 50.0));
    building1->SetBuildingType(Building::Residential);
    building1->SetExtWallsType(Building::ConcreteWithWindows);
    building1->SetNFloors(3);
    building1->SetNRoomsX(2);
    building1->SetNRoomsY(2);

    Ptr<Building> building2 = CreateObject<Building>();
    building2->SetBoundaries(Box(15.0, 20.0, -85.0, -80.0, 0.0, 50.0));
    building2->SetBuildingType(Building::Commercial);
    building2->SetExtWallsType(Building::ConcreteWithWindows);
    building2->SetNFloors(8);
    building2->SetNRoomsX(1);
    building2->SetNRoomsY(1);

    BuildingContainer buildingContainer;
    buildingContainer.Add(building1);
    buildingContainer.Add(building2);

    netsimulyzer::BuildingConfigurationHelper buildingHelper(orchestrator);
    buildingHelper.Install(buildingContainer);


    Ptr<netsimulyzer::ThroughputSink> car1ClientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Car1 Client Throughput (RX)");
    Ptr<netsimulyzer::ThroughputSink> car2ClientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Car2 Client Throughput (RX)");

    if (clientAppCar1.GetN() > 0) {
        clientAppCar1.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, car1ClientThroughput));
    }
    if (clientAppCar2.GetN() > 0) {
        clientAppCar2.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, car2ClientThroughput));
    }

    Ptr<netsimulyzer::ThroughputSink> serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, serverThroughput));

    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}



