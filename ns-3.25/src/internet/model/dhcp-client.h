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
 *
 */

#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"

#define DHCPDISCOVER            0
#define DHCPOFFER               1
#define DHCPREQ                 2
#define DHCPACK                 3
#define DHCPNACK                4

#define IDLEE                   0
#define TESTIDLE                5
#define WAIT_OFFER              1
#define REFRESH_LEASE           2
#define WAIT_ACK                9


#define REFRESH_TIMEOUT          7
#define RTRS_TIMEOUT             1

#define DHCP_IPC_PORT           6868

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup dhcpclientserver
 * \class DhcpClient
 * \brief A Dhcp client. It learns DHCP server and IP gw addresses from IP header. 
 * In other words DHCP server must be located on the network gw node.
 *
 */
class DhcpClient : public Application
{
public:
  static TypeId
  GetTypeId (void);

  DhcpClient();

  virtual ~DhcpClient();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Ipv4Address ip, uint16_t port);
  Ipv4Address GetOption_1(void);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void NetHandler(Ptr<Socket> socket);
  void IPCHandler(Ptr<Socket> socket);
  void LinkStateHandler(void);
  void RtrsHandler(void);

  void RunEfsm(void);

  uint8_t m_state;

  Ptr<Socket> m_socket;
  Ptr<Socket> m_ipc_socket;
  Ipv4Address m_peerAddress;
  Ipv4Address m_myAddress;
  Ipv4Address m_option1;
  uint16_t m_peerPort;
  EventId m_refreshEvent;
  EventId m_rtrsEvent;

};

} // namespace ns3

#endif /* DHCP_CLIENT_H */
