/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
*  Copyright (c) 2011 UPB
*
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
*
* Author: Radu Lupu <rlupu@elcom.pub.ro>
*/


#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "dhcp-server.h"
#include "dhcp-header.h"
#include <ns3/ipv4.h>

namespace ns3 {
  
  NS_LOG_COMPONENT_DEFINE ("DhcpServer");
  NS_OBJECT_ENSURE_REGISTERED (DhcpServer);
  
  
  TypeId
  DhcpServer::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::DhcpServer")
    .SetParent<Application> ()
    .AddConstructor<DhcpServer> ()
    .AddAttribute ("Port",
    "Port on which we listen for incoming packets.",
      UintegerValue (67),
    MakeUintegerAccessor (&DhcpServer::m_port),
    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PoolAddresses",
    "Pool of addresses to provide on request.",
    Ipv4AddressValue (),
    MakeIpv4AddressAccessor (&DhcpServer::m_poolAddress),
    MakeIpv4AddressChecker ())
    .AddAttribute ("PoolMask",
    "Mask of the pool of addresses.",
    Ipv4MaskValue (),
    MakeIpv4MaskAccessor (&DhcpServer::m_poolMask),
    MakeIpv4MaskChecker ())
    .AddAttribute ("Option_1",
    "Here of type IPv4 address. (e.g. gw addr or other server addr)",
    Ipv4AddressValue (),
    MakeIpv4AddressAccessor (&DhcpServer::m_option1),
    MakeIpv4AddressChecker ())
    ;
    return tid;
  }
  
  DhcpServer::DhcpServer ()
  {
    NS_LOG_FUNCTION (this);
    m_IDhost = 1;
    m_state = WORKING;
  }
  
  DhcpServer::~DhcpServer ()
  {
    NS_LOG_FUNCTION (this);
  }
  
  void
  DhcpServer::DoDispose (void)
  {
    NS_LOG_FUNCTION (this);
    Application::DoDispose ();
  }
  
  void DhcpServer::StartApplication(void){
    NS_LOG_FUNCTION (this);
    
    if (m_socket == 0)
    {
      uint32_t addrIndex;
      int32_t ifIndex;
      
      //add the DHCP local address to the leased addresses list, if it is defined!
      Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4> ();
      ifIndex = ipv4->GetInterfaceForPrefix(m_poolAddress, m_poolMask);
      if(ifIndex >= 0){
        for(addrIndex = 0; addrIndex < ipv4->GetNAddresses(ifIndex); addrIndex++){
          if((ipv4->GetAddress(ifIndex, addrIndex).GetLocal().Get() & m_poolMask.Get()) == m_poolAddress.Get()){
            uint32_t id = ipv4->GetAddress(ifIndex, addrIndex).GetLocal().Get() - m_poolAddress.Get();
            m_leasedAddresses.push_back(std::make_pair (id, 0xff));//set infinite GRANTED_LEASED_TIME for my address
            break;
          }
        }
      }
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), m_port);
      m_socket->SetAllowBroadcast (true);
      m_socket->Bind (local);
    }
    m_socket->SetRecvCallback(MakeCallback(&DhcpServer::NetHandler, this));
    m_expiredEvent = Simulator::Schedule (Seconds (LEASE_TIMEOUT), &DhcpServer::TimerHandler, this);
  }
  
  
  void DhcpServer::StopApplication (){
    NS_LOG_FUNCTION (this);
    
    if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
    Simulator::Remove(m_expiredEvent);
  }
  
  void DhcpServer::NetHandler (Ptr<Socket> socket){
    NS_LOG_FUNCTION (this << socket);
    RunEfsm();
  }
  
  void DhcpServer::TimerHandler(){
    //setup no of timer out events and release of unsolicited addresses from the list!
    int a=1;
    std::list<std::pair<uint32_t, uint8_t> >::iterator i;
    for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++, a++){
      //update the addr state
      if(i->second <= 1){
        m_leasedAddresses.erase(i--);    //release this addr
        NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Address removed!"<< a);
        continue;
      }
      if(i->second != 0xff) i->second--;
    }
    NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Address leased state expired!");
    m_expiredEvent = Simulator::Schedule (Seconds (LEASE_TIMEOUT), &DhcpServer::TimerHandler, this);
  }
  
  
  void DhcpServer::RunEfsm(void){
    DhcpHeader header, new_header;
    Address from;
    Ptr<Packet> packet = 0;
    Ipv4Address address;
    Mac48Address source;
    
    switch(m_state){
      case WORKING:
      uint32_t addr;
      packet = Create<Packet> (DHCP_HEADER_LENGTH);    //header (no payload added)
      packet = m_socket->RecvFrom (from);
      packet->RemoveHeader(header);
      source = header.GetChaddr();
      
      if(header.GetType() == DHCPDISCOVER){
        NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace RX: DHCP DISCOVER from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
        " source port: " <<  InetSocketAddress::ConvertFrom (from).GetPort ());
        
        //figure out a new address to be leased
        for(;;){//if no free addr found, stay here for_ever
          std::list<std::pair<uint32_t, uint8_t> >::const_iterator i;
          for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++){
            //check wheather the addr is busy
            if(i->first == m_IDhost){
              m_IDhost = m_IDhost % (uint32_t)(pow(2, 32 - m_poolMask.GetPrefixLength())-1) + 1;
              break;
            }
          }
          if(i == m_leasedAddresses.end ()){
            //free addr found => add to the list and set the lifetime
            addr = m_poolAddress.Get() + m_IDhost;
            m_leasedAddresses.push_back(std::make_pair (m_IDhost, GRANTED_LEASE_TIME));
            address.Set(addr);
            
            m_IDhost = m_IDhost % (uint32_t)(pow(2, 32 - m_poolMask.GetPrefixLength())-1) + 1;
            break;
          }
        }
        //            packet->RemoveAllPacketTags ();
        //            packet->RemoveAllByteTags ();
        packet = Create<Packet> (DHCP_HEADER_LENGTH);
        new_header.SetType (DHCPOFFER);
        new_header.SetChaddr(source);
        new_header.SetYiaddr(address);
        new_header.SetOption_1(m_option1);
        packet->AddHeader (new_header);
        
        if ((m_socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("255.255.255.255"), InetSocketAddress::ConvertFrom (from).GetPort ()))) >= 0){
          NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCP OFFER");
        }else{
          NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Error while sending DHCP OFFER");
        }
      }
      
      if(header.GetType() == DHCPREQ){
        uint32_t addr;
        Ipv4Address address;
        
        if (packet->GetSize () > 0){
          address=header.GetOption_1();
          addr=address.Get();
          
          NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace RX: DHCP REQUEST from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
          " source port: " <<  InetSocketAddress::ConvertFrom (from).GetPort () << " refreshed addr is =" << address);
          
          source = header.GetChaddr();
          std::list<std::pair<uint32_t, uint8_t> >::iterator i;
          for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++){
            //update the lifetime of this addr
            if((m_poolAddress.Get() + i->first) == addr){
              (i->second)++;
              packet = Create<Packet> (DHCP_HEADER_LENGTH);
              new_header.SetType (DHCPACK);
              new_header.SetChaddr(source);
              new_header.SetYiaddr(address);
              packet->AddHeader (new_header);
              m_peer= InetSocketAddress::ConvertFrom (from).GetIpv4 ();
              NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCP ACK SENT"<<source);
              if(m_peer!=address)
              m_socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("255.255.255.255"), InetSocketAddress::ConvertFrom (from).GetPort ()));
              else
                m_socket->SendTo(packet, 0, InetSocketAddress(m_peer, InetSocketAddress::ConvertFrom (from).GetPort ()));
              break;
            }
          }
          if(i == m_leasedAddresses.end ()){
            NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "This IP addr does not exists or released!");
            packet = Create<Packet> (DHCP_HEADER_LENGTH);
            new_header.SetType (DHCPNACK);
            new_header.SetChaddr(source);
            new_header.SetYiaddr(address);
            
            packet->AddHeader (new_header);
            NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCP NACK SENT");
            m_socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("255.255.255.255"), InetSocketAddress::ConvertFrom (from).GetPort ()));
          }
        }
      }
    }
  }
  
  
} // Namespace ns3
