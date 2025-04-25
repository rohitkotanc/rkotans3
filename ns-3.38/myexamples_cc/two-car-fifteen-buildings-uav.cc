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
    double durationUser = 95.0;
    std::string outputFileName = "two-car-fifteen-buildings-uav.json";

    CommandLine cmd;
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
    positionAlloc->Add(Vector3D(55.0, 1.0, 0.0));
    positionAlloc->Add(Vector3D(-25.0, 1.0, 0.0));
    positionAlloc->Add(Vector3D(15.0, 1.0, 50.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -1.0, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -1.0, 0.0));
    nodes.Get(2)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -1.0, 0.0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));

    auto car1Link = pointToPoint.Install(nodes.Get(0), nodes.Get(2));
    auto car2Link = pointToPoint.Install(nodes.Get(1), nodes.Get(2));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto car1Interfaces = address.Assign(car1Link);

    address.SetBase("10.1.2.0", "255.255.255.0");
    auto car2Interfaces = address.Assign(car2Link);

    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};
    auto serverApp = echoServer.Install(nodes.Get(2));
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


Simulator::Schedule(Seconds(2.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(4.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(6.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(12.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(15.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(18.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(21.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(25.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(31.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(34.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(36.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(39.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(43.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(46.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(49.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(52.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(57.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(59.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(63.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(67.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(71.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(73.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(76.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(78.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(84.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(87.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(91.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(94.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(101.0), &DropConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(110.0), &RestoreConnectivity, car1ClientErrorModel, car2ClientErrorModel, serverErrorModel);




    netsimulyzer::NodeConfigurationHelper nodeHelper(orchestrator);
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::BLUE));
nodeHelper.Install(nodes.Get(0));

nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::RED));
nodeHelper.Install(nodes.Get(1));


nodeHelper.Set("Model", netsimulyzer::models::QUADCOPTER_UAV_VALUE);
nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::YELLOW));
nodeHelper.Install(nodes.Get(2));




     Ptr<Building> building1 = CreateObject<Building>();
         building1->SetBoundaries(Box(15.0, 20.0, -1.0, -3.0, 0.0, 11.0));
    building1->SetBuildingType(Building::Residential);
    building1->SetExtWallsType(Building::Wood);
    building1->SetNFloors(2);
    building1->SetNRoomsX(1);
    building1->SetNRoomsY(2);

    Ptr<Building> building2 = CreateObject<Building>();
    building2->SetBoundaries(Box(15.0, 20.0, -5.0, -11.0, 0.0, 22.0));
    building2->SetBuildingType(Building::Office);
    building2->SetExtWallsType(Building::ConcreteWithWindows);
    building2->SetNFloors(5);
    building2->SetNRoomsX(3);
    building2->SetNRoomsY(3);

    Ptr<Building> building3 = CreateObject<Building>();
    building3->SetBoundaries(Box(15.0, 20.0, -14.0, -17.0, 0.0, 29.0));
    building3->SetBuildingType(Building::Commercial);
    building3->SetExtWallsType(Building::ConcreteWithoutWindows);
    building3->SetNFloors(1);
    building3->SetNRoomsX(2);
    building3->SetNRoomsY(1);

    Ptr<Building> building4 = CreateObject<Building>();
    building4->SetBoundaries(Box(15.0, 20.0, -20.0, -24.0, 0.0, 35.0));
    building4->SetBuildingType(Building::Residential);
    building4->SetExtWallsType(Building::StoneBlocks);
    building4->SetNFloors(3);
    building4->SetNRoomsX(4);
    building4->SetNRoomsY(1);

    Ptr<Building> building5 = CreateObject<Building>();
    building5->SetBoundaries(Box(15.0, 20.0, -30.0, -33.0, 0.0, 46.0));
    building5->SetBuildingType(Building::Office);
    building5->SetExtWallsType(Building::Wood);
    building5->SetNFloors(1);
    building5->SetNRoomsX(3);
    building5->SetNRoomsY(2);

    Ptr<Building> building6 = CreateObject<Building>();
    building6->SetBoundaries(Box(15.0, 20.0, -35.0, -38.0, 0.0, 15.0));
    building6->SetBuildingType(Building::Commercial);
    building6->SetExtWallsType(Building::ConcreteWithWindows);
    building6->SetNFloors(6);
    building6->SetNRoomsX(3);
    building6->SetNRoomsY(3);

    Ptr<Building> building7 = CreateObject<Building>();
    building7->SetBoundaries(Box(15.0, 20.0, -42.0, -45.0, 0.0, 70.0));
    building7->SetBuildingType(Building::Office);
    building7->SetExtWallsType(Building::StoneBlocks);
    building7->SetNFloors(2);
    building7->SetNRoomsX(4);
    building7->SetNRoomsY(2);

    Ptr<Building> building8 = CreateObject<Building>();
    building8->SetBoundaries(Box(15.0, 20.0, -48.0, -51.0, 0.0, 90.0));
    building8->SetBuildingType(Building::Office);
    building8->SetExtWallsType(Building::Wood);
    building8->SetNFloors(7);
    building8->SetNRoomsX(2);
    building8->SetNRoomsY(4);

    Ptr<Building> building9 = CreateObject<Building>();
    building9->SetBoundaries(Box(15.0, 20.0, -56.0, -58.0, 0.0, 95.0));
    building9->SetBuildingType(Building::Commercial);
    building9->SetExtWallsType(Building::ConcreteWithWindows);
    building9->SetNFloors(2);
    building9->SetNRoomsX(2);
    building9->SetNRoomsY(2);

    Ptr<Building> building10 = CreateObject<Building>();
    building10->SetBoundaries(Box(15.0, 20.0, -62.0, -66.0, 0.0, 99.0));
    building10->SetBuildingType(Building::Residential);
    building10->SetExtWallsType(Building::ConcreteWithoutWindows);
    building10->SetNFloors(4);
    building10->SetNRoomsX(3);
    building10->SetNRoomsY(3);

    Ptr<Building> building11 = CreateObject<Building>();
    building11->SetBoundaries(Box(15.0, 20.0, -70.0, -72.0, 0.0, 80.0));
    building11->SetBuildingType(Building::Office);
    building11->SetExtWallsType(Building::StoneBlocks);
    building11->SetNFloors(1);
    building11->SetNRoomsX(1);
    building11->SetNRoomsY(1);

    Ptr<Building> building12 = CreateObject<Building>();
    building12->SetBoundaries(Box(15.0, 20.0, -75.0, -77.0, 0.0, 62.0));
    building12->SetBuildingType(Building::Commercial);
    building12->SetExtWallsType(Building::ConcreteWithoutWindows);
    building12->SetNFloors(5);
    building12->SetNRoomsX(3);
    building12->SetNRoomsY(2);

    Ptr<Building> building13 = CreateObject<Building>();
    building13->SetBoundaries(Box(15.0, 20.0, -83.0, -86.0, 0.0, 55.0));
    building13->SetBuildingType(Building::Residential);
    building13->SetExtWallsType(Building::StoneBlocks);
    building13->SetNFloors(2);
    building13->SetNRoomsX(2);
    building13->SetNRoomsY(3);

    Ptr<Building> building14 = CreateObject<Building>();
    building14->SetBoundaries(Box(15.0, 20.0, -90.0, -93.0, 0.0, 44.0));
    building14->SetBuildingType(Building::Office);
    building14->SetExtWallsType(Building::ConcreteWithWindows);
    building14->SetNFloors(6);
    building14->SetNRoomsX(4);
    building14->SetNRoomsY(4);

    Ptr<Building> building15 = CreateObject<Building>();
    building15->SetBoundaries(Box(15.0, 20.0, -100.0, -109.0, 0.0, 20.0));
    building15->SetBuildingType(Building::Commercial);
    building15->SetExtWallsType(Building::Wood);
    building15->SetNFloors(3);
    building15->SetNRoomsX(1);
    building15->SetNRoomsY(1);



    BuildingContainer buildingContainer;
    buildingContainer.Add(building1);
    buildingContainer.Add(building2);
buildingContainer.Add(building3);
buildingContainer.Add(building4);
buildingContainer.Add(building5);
buildingContainer.Add(building6);
buildingContainer.Add(building7);
buildingContainer.Add(building8);
buildingContainer.Add(building9);
buildingContainer.Add(building10);
buildingContainer.Add(building11);
buildingContainer.Add(building12);
buildingContainer.Add(building13);
buildingContainer.Add(building14);
buildingContainer.Add(building15);

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






