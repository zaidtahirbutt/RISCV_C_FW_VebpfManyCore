////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	ethproto.h
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To encapsulate common functions associated with the ethernet
//		protocol portion of the network stack.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2019, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
////////////////////////////////////////////////////////////////////////////////
//
//
#ifndef	ETHPROTO_H
#define	ETHPROTO_H

#include <stdint.h>
#include "pkt.h"

typedef	uint64_t ETHERNET_MAC;

extern	unsigned eth_headersize();
extern	unsigned eth_headersize_v2();
extern	NET_PACKET *new_ethpkt(unsigned ln);
extern	NET_PACKET *new_ethpkt_v2(unsigned ln);
extern	void	tx_ethpkt(NET_PACKET *pkt, unsigned ethtype, ETHERNET_MAC mac);
extern	void	tx_ethpkt_v2(NET_PACKET *pkt, unsigned ethtype, ETHERNET_MAC src_mac, ETHERNET_MAC dst_mac, void *tx_pkt_current_start_mem_address);
extern	void	rx_ethpkt(NET_PACKET *pkt);
extern	void	rx_ethpkt_v2(NET_PACKET *pkt);
extern	void	dump_ethpkt(NET_PACKET *pkt);
extern	void	ethpkt_mac(NET_PACKET *pkt, ETHERNET_MAC *mac);
extern	void	ethpkt_mac_v2(NET_PACKET *pkt, ETHERNET_MAC *mac);
extern	unsigned ethpkt_ethtype(NET_PACKET *pkt);
extern	unsigned ethpkt_ethtype_v2(NET_PACKET *pkt);

#endif
