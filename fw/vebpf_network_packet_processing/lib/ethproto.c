////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	ethproto.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To encapsulate common functions associated with the ethernet
//		protocol portion of the network stack.
//
// 	The network controller handles some of this for us--it handles the
//	CRC and source MAC address for us.  As a result, instead of sending
//	a 6+6+2 byte header, we only need to handle a 6+2 byte header.
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
// #include <stdio.h>
#include <stdint.h>
#include "ethproto.h"
#include "ipproto.h"
#include "arp.h"
#include "etcnet.h"
//#include "txfns.h"
#include "protoconst.h"
#include "board.h"
// #include "../../../../riscv_subsystem/sw/utils.h" 
extern void printf(char *c, ...);  // including ../../../../riscv_subsystem/sw/utils.h in two files
								  // was giving compilation error. This extern method works.

ETHERNET_MAC	my_mac_addr = DEFAULTMAC;

unsigned eth_headersize() {
	return 8;	// Bytes
}

// this version includes src mac address bytes as well
unsigned eth_headersize_v2() {
	return 14;	// Bytes dst and src mac and 2 ether bytes
}


NET_PACKET	*new_ethpkt(unsigned ln) {  // for ARP new_ethpkt(28)
	NET_PACKET	*pkt;

	pkt = new_pkt(ln+8);  // should be ln+14 for my case
		// for ARP ln = 28, we add + 8 = 36 //total = 40, 32 + 8 from udp packt = 40
		// +8 means the 8 bytes for eth header.. 2 bytes for eth type and 6 bytes for dest mac.. no bytes for src mac for this lib
		// since in zipcpu design the src mac bytes were appended in hardware.. 
		// but for my design I'll have to change this to 8+6 = 14 bytes since I'll be appending the src mac bytes in softcore as well...
		// will be 36 + 6 = 42 when I add the src mac bytes 

	pkt->p_length -= 8;	// should be -=14 for my case	// // plength is -28 now? // nope.. pkt->p_length has already been allocated len by new_pkt()
	pkt->p_user += 8;  // should be +=14 for my case 
	return pkt;
}

// this version includes src mac address bytes as well
NET_PACKET	*new_ethpkt_v2(unsigned ln) {  // for ARP new_ethpkt(28)
	NET_PACKET	*pkt;

	pkt = new_pkt(ln+eth_headersize_v2());  // should be ln+14 for my case
		// for ARP ln = 28, we add + 8 = 36 //total = 40, 32 + 8 from udp packt = 40
		// +8 means the 8 bytes for eth header.. 2 bytes for eth type and 6 bytes for dest mac.. no bytes for src mac for this lib
		// since in zipcpu design the src mac bytes were appended in hardware.. 
		// but for my design I'll have to change this to 8+6 = 14 bytes since I'll be appending the src mac bytes in softcore as well...
		// will be 36 + 6 = 42 when I add the src mac bytes 

	pkt->p_length -= eth_headersize_v2();	// should be -=14 for my case	// // plength is -28 now? // nope.. pkt->p_length has already been allocated len by new_pkt()
	pkt->p_user += eth_headersize_v2();  // should be +=14 for my case 
	return pkt;
}

	// this is for ARP req
		//tx_ethpkt(pkt, ETHERTYPE_ARP, broadcast);  // #define	ETHERTYPE_ARP		0x0806, broadcast=ETHERNET_MAC broadcast = 0x0fffffffffffful;
void	tx_ethpkt(NET_PACKET *pkt, unsigned ethtype, ETHERNET_MAC mac) {
	// Destination MAC 
		//*mac = router_mac_addr; // = #define	DEFAULTMAC	  0xa25345b6fb5eul

	// remove this below, just for testing dest mac value
	// mac = 0xffffffffffffff;  // didnt work

	// now we pull back the p_user pointer (the adderss) 8 addresses even more (what ever the size of those addresses (word size 4 bytes))
	// so that now we can add the ethernet header of 8 bytes (6 bytes of destination mac the router mac address) and 2 bytes of Eth type/length

	pkt->p_user   -= eth_headersize();	// make this 14 // 8;	// Bytes
		// example from send arp function
			// pulling the p_user data pointer back 8 bytes to now write the ethernet packet bytes into it and then send it to the FPGA rtl to send this completed newly formed ethernet IEEE8.something approved packet

	pkt->p_length += eth_headersize();  // make this 14 // // now the packet len is 8 (from ICMP) + 20 (from this IP data) + 8 (for ethernet packets) bytes = 36 bytes 
		// for send arp req its len was 28 for the arp packet, it actually was 36, then the ethpacket func subtracted 8 from it so it could add its size of 8 bytes later here

	//first the ICMP, then IP header is formed or ARP then at the end the ethernet header
	// The IP header length is already added to this. Then in the lower layer here at the eth layer
	// the eth header size is being added to this pkt p_length

	// crc is added in the rtl FPGA hardware at the end of the ethernet packet
	// 0xa25345b6fb5eul
	// tx bram value first 2 words in eth rtl b64553a2,00085efb,
	// whereas I want a25345b6, fb5e0008
	// I think this will be solved by the endian swap in the rtl as in zipversa, but I will recheck for other OSI layer packets
	pkt->p_user[0] = (mac >> 40l) & 0x0ff;  // might need to do only 0xff and not 0x0ff
	pkt->p_user[1] = (mac >> 32l) & 0x0ff;


	pkt->p_user[2] = (mac >> 24) & 0x0ff;
	pkt->p_user[3] = (mac >> 16) & 0x0ff;
	pkt->p_user[4] = (mac >>  8) & 0x0ff;
	pkt->p_user[5] = (mac      ) & 0x0ff;

	// add src mac here 


	pkt->p_user[6] = (ethtype >> 8) & 0x0ff;  // #define	ETHERTYPE_ARP		0x0806
	pkt->p_user[7] = (ethtype     ) & 0x0ff;

	tx_pkt(pkt);  // i think the FPGA will add the extra bits to complete the min length requirement
}

void	tx_ethpkt_v2(NET_PACKET *pkt, unsigned ethtype, ETHERNET_MAC src_mac, ETHERNET_MAC dst_mac, void *tx_pkt_current_start_mem_address) {
	// Destination MAC 
		//*mac = router_mac_addr; // = #define	DEFAULTMAC	  0xa25345b6fb5eul

	// remove this below, just for testing dest mac value
	// mac = 0xffffffffffffff;  // didnt work

	// now we pull back the p_user pointer (the adderss) 8 addresses even more (what ever the size of those addresses (word size 4 bytes))
	// so that now we can add the ethernet header of 8 bytes (6 bytes of destination mac the router mac address) and 2 bytes of Eth type/length

	pkt->p_user   -= eth_headersize_v2();	// make this 14 // 8;	// Bytes
		// example from send arp function
			// pulling the p_user data pointer back 8 bytes to now write the ethernet packet bytes into it and then send it to the FPGA rtl to send this completed newly formed ethernet IEEE8.something approved packet

	pkt->p_length += eth_headersize_v2();  // make this 14 // // now the packet len is 8 (from ICMP) + 20 (from this IP data) + 8 (for ethernet packets) bytes = 36 bytes 
		// for send arp req its len was 28 for the arp packet, it actually was 36, then the ethpacket func subtracted 8 from it so it could add its size of 8 bytes later here

	//first the ICMP, then IP header is formed or ARP then at the end the ethernet header
	// The IP header length is already added to this. Then in the lower layer here at the eth layer
	// the eth header size is being added to this pkt p_length

	// crc is added in the rtl FPGA hardware at the end of the ethernet packet
	// 0xa25345b6fb5eul
	// tx bram value first 2 words in eth rtl b64553a2,00085efb,
	// whereas I want a25345b6, fb5e0008
	// I think this will be solved by the endian swap in the rtl as in zipversa, but I will recheck for other OSI layer packets

	// heap grown upwards.. Big Endian storage here

	// dst mac
	pkt->p_user[0] = (dst_mac >> 40l) & 0x0ff;  // might need to do only 0xff and not 0x0ff
	pkt->p_user[1] = (dst_mac >> 32l) & 0x0ff;

	pkt->p_user[2] = (dst_mac >> 24) & 0x0ff;
	pkt->p_user[3] = (dst_mac >> 16) & 0x0ff;
	pkt->p_user[4] = (dst_mac >>  8) & 0x0ff;
	pkt->p_user[5] = (dst_mac      ) & 0x0ff;

	// add src mac here
	pkt->p_user[6] = (src_mac >> 40l) & 0x0ff; 
	pkt->p_user[7] = (src_mac >> 32l) & 0x0ff;

	pkt->p_user[8] = (src_mac >> 24) & 0x0ff;
	pkt->p_user[9] = (src_mac >> 16) & 0x0ff;
	pkt->p_user[10] = (src_mac >>  8) & 0x0ff;
	pkt->p_user[11] = (src_mac      ) & 0x0ff;	 


	pkt->p_user[12] = (ethtype >> 8) & 0x0ff;  // #define	ETHERTYPE_ARP		0x0806
	pkt->p_user[13] = (ethtype     ) & 0x0ff;


	if (DEBUG)
		printf("\n\n\n****** Inside tx_ethpkt_v2() and entering tx_pkt_v3() ******\n\n\n");

	// tx_pkt_v2(pkt);  // i think the FPGA will add the extra bits to complete the min length requirement.. yes it will :3
	tx_pkt_v3(pkt, tx_pkt_current_start_mem_address);  // i think the FPGA will add the extra bits to complete the min length requirement.. yes it will :3

	if (DEBUG)
		printf("\n\n\n****** Inside tx_ethpkt_v2() and exited tx_pkt_v3() ******\n\n\n");
}

extern	void	rx_ethpkt(NET_PACKET *pkt) {
	pkt->p_user   += 8;  // go forward from the ethernet data layer and to the higher layers 
	pkt->p_length -= 8;		// subtract the ethernet packet length from the total length
}

extern	void	rx_ethpkt_v2(NET_PACKET *pkt) {  // strips of the eth header!
	pkt->p_user   += 14;  // +14 since in my eth rtl module the dst mac is also included so +6 bytes of that //+= 8;  // go forward from the ethernet data layer and to the higher layers 
	pkt->p_length -= 14;  //-= 14;		// subtract the ethernet packet length from the total length
}

void	dump_ethpkt(NET_PACKET *pkt) {
	pkt_reset(pkt);
	// txstr("DUMP: ETHPKT\nPKT : "); txhex((unsigned)pkt); txstr(" - "); txhex((unsigned)pkt->p_raw); txstr("\n");
	printf("ETH PROTO: %04x\n", ethpkt_ethtype(pkt));
	printf("ETH MAC  : %02x:%02x:%02x:%02x:%02x:%02x\n",
			pkt->p_user[0] & 0x0ff,
			pkt->p_user[1] & 0x0ff,
			pkt->p_user[2] & 0x0ff,
			pkt->p_user[3] & 0x0ff,
			pkt->p_user[4] & 0x0ff,
			pkt->p_user[5] & 0x0ff);

	if (ethpkt_ethtype(pkt) == ETHERTYPE_IP) {
		pkt->p_user += 8; pkt->p_length -= 8;
		dump_ippkt(pkt);
		pkt->p_user -= 8; pkt->p_length += 8;
	} else if (ethpkt_ethtype(pkt) == ETHERTYPE_ARP) {
		pkt->p_user += 8; pkt->p_length -= 8;
		dump_arppkt(pkt);
		pkt->p_user -= 8; pkt->p_length += 8;
	}
	printf("ETH CRC  : %02x%02x\n",
			pkt->p_user[pkt->p_length] & 0x0ff,
			pkt->p_user[pkt->p_length+1] & 0x0ff);
}

extern	void	ethpkt_mac(NET_PACKET *pkt, ETHERNET_MAC *mac) {
	ETHERNET_MAC	smac;  // in the orig zipversa proj, the dest mac was stripped off

	smac = 0;
	smac = (pkt->p_user[0] & 0x0ff);
	smac = (pkt->p_user[1] & 0x0ff) | (smac << 8);
	smac = (pkt->p_user[2] & 0x0ff) | (smac << 8);
	smac = (pkt->p_user[3] & 0x0ff) | (smac << 8);
	smac = (pkt->p_user[4] & 0x0ff) | (smac << 8);
	smac = (pkt->p_user[5] & 0x0ff) | (smac << 8);
	*mac = smac;
}

extern	void	ethpkt_mac_v2(NET_PACKET *pkt, ETHERNET_MAC *mac) {
	ETHERNET_MAC	dest_mac;

	dest_mac = 0;
	dest_mac = (pkt->p_user[0] & 0x0ff);
	dest_mac = (pkt->p_user[1] & 0x0ff) | (dest_mac << 8);
	dest_mac = (pkt->p_user[2] & 0x0ff) | (dest_mac << 8);
	dest_mac = (pkt->p_user[3] & 0x0ff) | (dest_mac << 8);
	dest_mac = (pkt->p_user[4] & 0x0ff) | (dest_mac << 8);
	dest_mac = (pkt->p_user[5] & 0x0ff) | (dest_mac << 8);
	*mac = dest_mac;
}

unsigned ethpkt_ethtype(NET_PACKET *pkt) {
	unsigned	v;
	v = (pkt->p_user[6] & 0x0ff) << 8;
	v = v | (pkt->p_user[7] & 0x0ff);

	return v;
}

unsigned ethpkt_ethtype_v2(NET_PACKET *pkt) {
	unsigned	v;
	v = (pkt->p_user[12] & 0x0ff) << 8;	// index 12 since in my eth rtl module the dst mac is also included so +6 bytes of that // (pkt->p_user[6] & 0x0ff) << 8;
	v = v | (pkt->p_user[13] & 0x0ff);	// v | (pkt->p_user[7] & 0x0ff);

	return v;
}