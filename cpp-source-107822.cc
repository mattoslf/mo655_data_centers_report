/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/olsr-helper.h"
#include <vector>
#include <stdint.h>
#include <sstream>
#include <fstream>

using namespace ns3;

int main (int argc, char *argv[])
{
	uint32_t nStas = 5;	// Número de clientes
	uint32_t flowType = 0;	// Tipo do tráfego
	uint32_t simTime = 60.0;	//Tempo de simulação
	uint32_t moveType = 0;	//Tempo de simulação
	uint32_t protocolType = 0;	//Tempo de simulação

	// Flags para a linha de execução
	CommandLine cmd;
	cmd.AddValue ("nStas", "Numero de clientes", nStas);
	cmd.AddValue ("flowType", "Tipo do fluxo, 0 para CBR e 1 para rajadas", flowType);
	cmd.AddValue ("protocolType", "Tipo do protocolo, 0 para UDP e 1 para TCP", protocolType);
	cmd.AddValue ("simTime", "Tempo de simulação", simTime);
	cmd.AddValue ("moveType", "Tipo de movimentação dos clientes, 0 para ficarem parados e 1 para ficarem em movimento", moveType);
	cmd.Parse (argc, argv);

	// Grupos dos nós da rede
	NodeContainer apNodes;
	NodeContainer staNodes;
	NodeContainer staNodesNear;
	NodeContainer staNodesFar;
	NodeContainer csmaSwitch;
	NodeContainer serverNodes;

	// Grupos das insterfaces da rede
	NetDeviceContainer apDevices;
	NetDeviceContainer switchDevices;
	NetDeviceContainer serverDevices;
	NetDeviceContainer staDevices;

	// Grupos das subnets da rede
	Ipv4InterfaceContainer apInterfaces;
	Ipv4InterfaceContainer serverInterfaces;
	Ipv4InterfaceContainer staInterfaces;

	// Pilha dos protocolos e buffers
	InternetStackHelper stack;

	// Construtor do CSMA
	CsmaHelper csma;
	
	// Ips bases das subnets
	Ipv4AddressHelper ipServer, ipSta;
	ipServer.SetBase ("192.168.0.0", "255.255.255.0");
	ipSta.SetBase ("192.168.1.0", "255.255.255.0");

	// Criação dos nós da rede
	apNodes.Create(1);
	staNodes.Create(nStas-2);
	staNodesNear.Create(1);
	staNodesFar.Create(1);
	serverNodes.Create (1);
	csmaSwitch.Create (1);

	// Configuração dos parâmetros do CSMA
	NetDeviceContainer link;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

	// Conecta os APs no switch
	switchDevices = csma.Install (NodeContainer (csmaSwitch, apNodes));
	// Conecta o servidor no switch
	serverDevices = csma.Install(NodeContainer(serverNodes, csmaSwitch));

	// Configuração dos parâmetros da WIFI
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	YansWifiChannelHelper wifiChannel;// = YansWifiChannelHelper::Default ();
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (3.0));
	wifiPhy.SetChannel (wifiChannel.Create ());

	// Construtor das posições e mobilidade dos nós
	MobilityHelper mobility;

	// Primeiro AP no ponto (0,0)
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				"MinX", DoubleValue (50.0),
				"MinY", DoubleValue (50.0),
				"DeltaX", DoubleValue (5.0),
				"DeltaY", DoubleValue (5.0),
				"GridWidth", UintegerValue (1),
				"LayoutType", StringValue ("RowFirst"));
	// Primeiro AP com posição constante
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	// Instala no primeiro AP
	mobility.Install (apNodes.Get(0));

	// Posiciona o servidor e o switch em posições irrelevantes
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				"MinX", DoubleValue (50.0),
				"MinY", DoubleValue (75.0),
				"DeltaX", DoubleValue (25.0),
				"DeltaY", DoubleValue (25.0),
				"GridWidth", UintegerValue (1.0),
				"LayoutType", StringValue ("RowFirst"));
	// Servidor e switch com posição constante
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	// Instala no servidor e no switch
	mobility.Install (NodeContainer(serverNodes, csmaSwitch));

	// Aloca um quadrado de lado 100m e posiciona os clientes em posições aleatórias
	mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
			"X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
			"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));

	// Movimenta os clientes
	if(!moveType) {
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	}
	else {
		mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
				"Mode", StringValue ("Time"),
				"Time", StringValue ("2s"),
				"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
	}

	// Instala no clientes
	mobility.Install (staNodes);

	// Primeiro AP no ponto (0,0)
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				"MinX", DoubleValue (55.0),
				"MinY", DoubleValue (55.0),
				"DeltaX", DoubleValue (5.0),
				"DeltaY", DoubleValue (5.0),
				"GridWidth", UintegerValue (1),
				"LayoutType", StringValue ("RowFirst"));
	// Movimenta os clientes
	if(!moveType) {
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	}
	else {
		mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
				"Mode", StringValue ("Time"),
				"Time", StringValue ("2s"),
				"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
	}
	// Instala no primeiro AP
	mobility.Install (staNodesNear);

		// Primeiro AP no ponto (0,0)
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				"MinX", DoubleValue (1.0),
				"MinY", DoubleValue (1.0),
				"DeltaX", DoubleValue (5.0),
				"DeltaY", DoubleValue (5.0),
				"GridWidth", UintegerValue (1),
				"LayoutType", StringValue ("RowFirst"));
	// Movimenta os clientes
	if(!moveType) {
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	}
	else {
		mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
				"Mode", StringValue ("Time"),
				"Time", StringValue ("2s"),
				"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
	}
	// Instala no primeiro AP
	mobility.Install (staNodesFar);

	// SSID da rede WIFI
	std::ostringstream oss;
	oss << "wifi-default";
	Ssid ssid = Ssid (oss.str ());

	WifiHelper wifi = WifiHelper::Default ();

	// Configura a WIFI dos APs
	wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
	// Instala a WIFI nos APs
	apDevices = wifi.Install (wifiPhy, wifiMac, apNodes);

	// Cria uma Bridge entre o switch e os APs
	Ptr<Node> switchNode = csmaSwitch.Get(0);
	BridgeHelper bridge;
	NetDeviceContainer bridgeDevices;
	bridgeDevices.Add (bridge.Install(apNodes.Get (0), NetDeviceContainer (apDevices.Get (0), switchDevices.Get (1))));

	// Configura a WIFI nos clientes
	wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
	// Instala a WIFI nos clientes
	staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer(staNodes, staNodesNear, staNodesFar));

	// Instala as pilhas e os buffers nos nós
	stack.InstallAll();

	// Atribui IP para o servidor
	serverInterfaces = ipServer.Assign(NetDeviceContainer(serverDevices));
	// Atribui IP para o switch, as bridges e os clientes
	staInterfaces.Add(ipSta.Assign(switchDevices.Get(0)));
	staInterfaces.Add(ipSta.Assign(bridgeDevices));
	staInterfaces.Add(ipSta.Assign(staDevices));

	// Configura a aplicação
	Address dest, sinkdest;
	std::string protocol;
	dest = InetSocketAddress (serverInterfaces.GetAddress(0), 1025);
	sinkdest = InetSocketAddress (Ipv4Address::GetAny (), 1025);
	if(protocolType == 0) {
		std::cout << "Using UDP\n";
		protocol = "ns3::UdpSocketFactory";
	}
	else {
		std::cout << "Using TCP\n";
		protocol = "ns3::TcpSocketFactory";
	}

	// Transforma o nó do servidor em um sink
	PacketSinkHelper sink (protocol, InetSocketAddress (Ipv4Address::GetAny (), 1025));
	ApplicationContainer app = sink.Install (serverNodes);

    app.Start (Seconds (1.5));
    app.Stop (Seconds (simTime));

	// Aplicações dos clientes
	OnOffHelper onoff = OnOffHelper (protocol, dest);
	if(flowType == 0) {	// Tráfego CBR
		// Pacotes de 512 bytes
		onoff.SetAttribute ("PacketSize", UintegerValue (512));
		// Sempre envia pacotes
		onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
		onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000.0]"));
	}
	else {	// Tráfego em rajadas
		// Pacotes de 1500 bytes
		onoff.SetAttribute ("PacketSize", UintegerValue (1500));
		// Envia rajadas entre intervalos de silêncio
		onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
		onoff.SetAttribute ("OnTime",	 StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"));
	}
	
	// Instala a aplicação nos clientes
	ApplicationContainer apps = onoff.Install (NodeContainer(staNodes, staNodesNear, staNodesFar));

	// Tempo de início da aplicação
	apps.Start(Seconds (1.5));
	apps.Stop(Seconds (simTime));

	// Popula as tabelas de roteamento
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Tempo de término da simulação
	Simulator::Stop (Seconds (simTime));

	// FlowMonitor, para colher as informações
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
//	Ptr<FlowMonitor> monitor = flowmon.Install(staNodes);

	// Roda a simulação
	Simulator::Run ();

	flowmon.SerializeToXmlFile ("projeto.stats", true, true);

	// Itera nas informações obtidas da simulação para imprimir os dados requeridos
	monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
			std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
			std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
			std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
			std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
			std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
			std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
			std::cout << "  Delay: " << i->second.delaySum / i->second.rxPackets / 1000 / 1000 << "\n";
		}
	}
	std::cout << "Finished!\n";
	// Destrói a simulação
	Simulator::Destroy ();
}
