/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
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
 *
 */

#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include <ns3/traced-value.h>

#define DHCPDISCOVER            0
#define DHCPOFFER               1
#define DHCPREQ                 2
#define DHCPACK                 3
#define DHCPNACK                4

#define WORKING                 0


#define LEASE_TIMEOUT           30
#define GRANTED_LEASE_TIME      3

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup dhcpclientserver DhcpClientServer
 */

/**
 * \ingroup dhcpclientserver
 * \class DhcpServer
 * \brief A Dhcp server. 
 */
class DhcpServer : public Application
{
public:
  static TypeId GetTypeId (void);
  DhcpServer();
  virtual ~DhcpServer();

  void RunEfsm(void);
  void NetHandler(Ptr<Socket> socket);
  void TimerHandler(void);

friend inline std::ostream & operator<< (std::ostream& os, Ipv4Address &address)
{
  address.Print (os);
  return os;
}
  
protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  Ptr<Socket> m_socket;
  Address m_local;
  uint16_t m_port;

  uint8_t m_state;
  uint8_t m_share;
  
  Ipv4Address m_poolAddress;
  Ipv4Mask m_poolMask;
  Ipv4Address m_option1;
  Ipv4Address m_peer;
  std::list<std::pair<uint32_t, uint8_t> > m_leasedAddresses; //and their status (cache memory)
  uint32_t m_IDhost;
  
  EventId m_expiredEvent;
};

} // namespace ns3

#endif /* DHCP_SERVER_H */
