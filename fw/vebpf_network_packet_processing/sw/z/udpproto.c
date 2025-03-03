////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	udpproto.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To encapsulate common functions associated with the UDP protocol
//		portion of the network stack.
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
// #include <stdio.h>  // not commenting this out was giving an error while using printf
#include <string.h>
#include "pkt.h"
#include "etcnet.h"
#include "protoconst.h"
#include "ipproto.h"
#include "ipcksum.h"
#include "udpproto.h"
#include "board.h"


extern void printf(char *c, ...);  // including ../../../../riscv_subsystem/sw/utils.h in two files
								  // was giving compilation error. This extern method works. 

unsigned	udp_headersize(void) {
	return 8;  // udp header has 8 bytes https://en.wikipedia.org/wiki/User_Datagram_Protocol		
}

NET_PACKET *new_udppkt(unsigned len) {
	NET_PACKET	*pkt;  // ptrs so that we just pass on ptr address values instead of making object copies and passin objects which would take up a lot of extra mem space

	pkt = new_ippkt(len + udp_headersize());  // 4 + 8  // 4 bytes for fftId and fftpos? yes 4 bytes of udp data payload
		// udp header has 8 bytes https://en.wikipedia.org/wiki/User_Datagram_Protocol
	pkt->p_user   += udp_headersize();  // adding 8 bytes to the use char ptr
	pkt->p_length -= udp_headersize();
}

NET_PACKET *new_udppkt_v2(unsigned len) {
	NET_PACKET	*pkt;  // ptrs so that we just pass on ptr address values instead of making object copies and passin objects which would take up a lot of extra mem space

	pkt = new_ippkt_v2(len + udp_headersize());  // 4 + 8  // 4 bytes for fftId and fftpos? yes 4 bytes of udp data payload
		// udp header has 8 bytes https://en.wikipedia.org/wiki/User_Datagram_Protocol
	pkt->p_user   += udp_headersize();  // adding 8 bytes to the use char ptr
	pkt->p_length -= udp_headersize();
}

// tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);  // src ip is from the rx packet so src ip is the dest ip for our device to send the udp packet to
//tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);  // #define	FFTPORT	6783  // fft_port  = sport; sport from the host pc
void	tx_udp(NET_PACKET *pkt, unsigned dest,
				unsigned sport, unsigned dport) {
	unsigned	cksum;

	pkt->p_user   -= udp_headersize();  // drag the pointer udp_headersize() size steps back since we already wrote the fftid and fpga post on the 4 user bytes in the fft example
	pkt->p_length += udp_headersize();

	pkt->p_user[0] = (sport >> 8) & 0x0ff;  // so in the UDP packet we have our S_Port then D_Port then pkt length, we had done txpkt = new_udppkt(4); before tx udp 
	pkt->p_user[1] = (sport     ) & 0x0ff;
	pkt->p_user[2] = (dport >> 8) & 0x0ff;
	pkt->p_user[3] = (dport     ) & 0x0ff;
	pkt->p_user[4] = (pkt->p_length >> 8) & 0x0ff;
	pkt->p_user[5] = (pkt->p_length     ) & 0x0ff;
	pkt->p_user[6] = 0;  // not mandatory to send checksum for the UDP packet
	pkt->p_user[7] = 0;

	// COMMENTING OUT UDP CKSUM since its not mandatory
//	cksum = ipcksum(pkt->p_length, pkt->p_user);
//
//	pkt->p_user[6] = (cksum >> 8) & 0x0ff;
//	pkt->p_user[7] = (cksum     ) & 0x0ff;

	printf("TX UDP: length=%4d, %02x:%02x,%02x:%02x,%02x:%02x (%02x,%02x)\n",
	pkt->p_length,
	pkt->p_user[0]&0x0ff, pkt->p_user[1]&0x0ff,
	pkt->p_user[2]&0x0ff, pkt->p_user[3]&0x0ff,
	pkt->p_user[4]&0x0ff, pkt->p_user[5]&0x0ff,
	pkt->p_user[6]&0x0ff, pkt->p_user[7]&0x0ff);

	//printf("Testing extern printf function while inside tx_udp() function %d \n", 323);

	tx_ippkt(pkt, IPPROTO_UDP, my_ip_addr, dest);  // my_ip address is being inserted here as well along with the IPPROTO_UDP IP Protocol Option
}


void	tx_udp_v2(NET_PACKET *pkt, unsigned dest,
				unsigned sport, unsigned dport, void *tx_pkt_current_start_mem_address) {
	unsigned	cksum;

	pkt->p_user   -= udp_headersize();  // drag the pointer udp_headersize() size steps back since we already wrote the fftid and fpga post on the 4 user bytes in the fft example
	pkt->p_length += udp_headersize();

	pkt->p_user[0] = (sport >> 8) & 0x0ff;  // so in the UDP packet we have our S_Port then D_Port then pkt length, we had done txpkt = new_udppkt(4); before tx udp 
	pkt->p_user[1] = (sport     ) & 0x0ff;
	pkt->p_user[2] = (dport >> 8) & 0x0ff;
	pkt->p_user[3] = (dport     ) & 0x0ff;
	pkt->p_user[4] = (pkt->p_length >> 8) & 0x0ff;
	pkt->p_user[5] = (pkt->p_length     ) & 0x0ff;
	pkt->p_user[6] = 0;  // not mandatory to send checksum for the UDP packet
	pkt->p_user[7] = 0;

	// COMMENTING OUT UDP CKSUM since its not mandatory
	// cksum = ipcksum(pkt->p_length, pkt->p_user);
//
	// pkt->p_user[6] = (cksum >> 8) & 0x0ff;
	// pkt->p_user[7] = (cksum     ) & 0x0ff;

	// commenting out printing for now.
	// printf("TX UDP: length=%4d, %02x:%02x,%02x:%02x,%02x:%02x (%02x,%02x)\n",
	// pkt->p_length,
	// pkt->p_user[0]&0x0ff, pkt->p_user[1]&0x0ff,
	// pkt->p_user[2]&0x0ff, pkt->p_user[3]&0x0ff,
	// pkt->p_user[4]&0x0ff, pkt->p_user[5]&0x0ff, 
	// pkt->p_user[6]&0x0ff, pkt->p_user[7]&0x0ff);

	//printf("Testing extern printf function while inside tx_udp() function %d \n", 323);

	if (DEBUG)
		printf("\n\n\n****** Inside tx_udp_v2() and entering tx_ippkt_v2() ******\n\n\n");

	// tx_ippkt(pkt, IPPROTO_UDP, my_ip_addr, dest);  // my_ip address is being inserted here as well along with the IPPROTO_UDP IP Protocol Option
	tx_ippkt_v2(pkt, IPPROTO_UDP, my_ip_addr, dest, tx_pkt_current_start_mem_address);  // my_ip address is being inserted here as well along with the IPPROTO_UDP IP Protocol Option

	if (DEBUG)
		printf("\n\n\n****** Inside tx_udp_v2() and exited tx_ippkt_v2() ******\n\n\n");
}

void	rx_udp(NET_PACKET *pkt) {
	unsigned ln = ((pkt->p_user[4] & 0x0ff) << 8)
		| (pkt->p_user[5] & 0x0ff);

	pkt->p_user   += udp_headersize();
	if (ln <= 8)
		pkt->p_length = 0;
	else if (pkt->p_length > ln)
		pkt->p_length = ln - udp_headersize();  // bigger than the whole ethernet packet udp packet? Chop i tup and send in smaller sized packets?
	else
		pkt->p_length -= udp_headersize();
}

unsigned	udp_sport(NET_PACKET *pkt) {
	unsigned sport = ((pkt->p_user[0] & 0x0ff) << 8)
		| (pkt->p_user[1] & 0x0ff);

	return sport;
}

unsigned	udp_dport(NET_PACKET *pkt) {
	unsigned dport = ((pkt->p_user[2] & 0x0ff) << 8)
		| (pkt->p_user[3] & 0x0ff);

	return dport;
}

void	dump_udppkt(NET_PACKET *pkt) {
	unsigned	cksum, sport, dport, len, pktsum;

	sport = udp_sport(pkt);
	dport = udp_dport(pkt);
	len = ((pkt->p_user[4] & 0x0ff) << 8)
		| (pkt->p_user[5] & 0x0ff);

	pktsum = ((pkt->p_user[6] & 0x0ff) << 8)
		| (pkt->p_user[7] & 0x0ff);

	pkt->p_user[6] = 0; pkt->p_user[7] = 0;
	
	// comment this out later
	cksum = ipcksum(len, pkt->p_user);
	// https://en.wikipedia.org/wiki/User_Datagram_Protocol
		// so the checksum uses the pseudo ipv4 header plus udp header and data
		// but this implementation does not use the pseudo ipv4 header info
		// so I believe its a custom checksum cx txudp function has it
		// commented out, so its available there..
		// I can just ignore it since it won't match

	// Put the checksum values back
	pkt->p_user[6] = (cksum >> 8) & 0x0ff;
	pkt->p_user[7] = (cksum     ) & 0x0ff;

	printf("UDP SPORT : %d\n", sport);
	printf("UDP DPORT : %d\n", dport);
	printf("UDP LENGTH: %d bytes", len);
	
	if (len != pkt->p_length)
		printf(" -- DOESN\'T MATCH %d\n", pkt->p_length);
	
	printf("UDP CKSUM : %04x", pktsum);


	if (pktsum != cksum)
		printf(" -- DOESN\'T MATCH %04x\n", cksum); 
	printf("UDP DATA  :\n");
	for(unsigned k=8; k<len; k++) {
		if ((k & 0x0f) == 8)
			printf("%*s: ", 14, "");
		printf("%08x ", pkt->p_user[k]);
		if ((k & 0x0f) == 0)
			printf(" ");
		else if ((k & 0x0f) == 7)
			printf("\n");
	} if ((len & 0x0f) != 8)
		printf("\n");
}

// in this version of dump we are printing the udp data as a string instead of hex
// cx we don't have hex printing available yet
void	dump_udppkt_v2(NET_PACKET *pkt) {
	unsigned	cksum, sport, dport, len, pktsum;

	sport = udp_sport(pkt);
	dport = udp_dport(pkt);
	len = ((pkt->p_user[4] & 0x0ff) << 8)
		| (pkt->p_user[5] & 0x0ff);

	pktsum = ((pkt->p_user[6] & 0x0ff) << 8)
		| (pkt->p_user[7] & 0x0ff);

	pkt->p_user[6] = 0; pkt->p_user[7] = 0;
	
	// comment this out later
	// don't need to calculate the checksum if I am not using the same checksum algo for sending the pkt as in the txpkt func for udp
	// cksum = ipcksum(len, pkt->p_user);
	// https://en.wikipedia.org/wiki/User_Datagram_Protocol
		// so the checksum uses the pseudo ipv4 header plus udp header and data
		// but this implementation does not use the pseudo ipv4 header info
		// so I believe its a custom checksum cx txudp function has it
		// commented out, so its available there..
		// I can just ignore it since it won't match cx we are not 
		// running the custom checksum function function while TX-ing the pkt
		// as the txpkt function of this lib is using it so the rxpkt
		// will be able to verify that checksum cx its is using the same 
		// checksum function, but since I am TX-ing the pkt from my laptop
		// without using this custom checksum function, hence it won't match
		// when the rxpkt checksum runs on it .. btw talking about the UDP header
		// checksum here 

	// Put the checksum values back
	pkt->p_user[6] = (cksum >> 8) & 0x0ff;
	pkt->p_user[7] = (cksum     ) & 0x0ff;

	printf("UDP SPORT : %d\n", sport);
	printf("UDP DPORT : %d\n", dport);
	printf("UDP LENGTH: %d bytes", len);
	
	if (len != pkt->p_length)
		printf(" -- DOES NOT MATCH %d\n", pkt->p_length);
		// in experiment 2023_11_17_edgetestbed_a100T21_stressTest3_sync.c I was getting following message:
			// UDP LENGTH: 17 bytes -- DOE'T MATCH 26
		// so that there was some error so I put a wireshark filter ip.addr == 192.168.1.128
		// and found that total len of UDP payload was infact 17 bytes that included 8 bytes of 
		// UDP header data and 9 bytes of UDP data payload.
		// Then I checked that why is pkt->p_length equal to = 26 bytes,
		// the reason for this 26 bytes length was that the ethernet rxpkt
		// was padded with 0s after the UDP data payload because the total
		// length of ethernet rxpkt uptill the UDP data payload was less than the min ethernet pkt len
		// of which is 64 bytes, out ethernet receiver pipeline removes the last 4 CRC bytes so 
		// the min len would become 60 bytes. Wireshark also shows this 60 bytes length since I guess it
		// doesn't show the last 4 CRC bytes, infact it doesn't show them. 
	    // So I confirmed from the rxpkt in wireshark that after the 17 bytes of UDP header + payload, the padded 0s
		// made the total length equal to 26 which is infact the length of the remaining rxpkt pkt->p_length.
		// We don't need to look at the padded 0s, UDP header will tell us that (UDP LENGTH: 17 bytes vs 26 bytes total len)

	// printf("UDP CKSUM : %04x", pktsum);
	// printf("UDP CKSUM : %d", pktsum);


	// if (pktsum != cksum)
	// 	// printf(" -- DOESN\'T MATCH %04x\n", cksum); 
	// 	printf(" -- DOES N0T MATCH the calculated CHECKSUM %d\n", cksum); 
	printf("UDP DATA  :\n");
	for(unsigned k=8; k<len; k++) {
	
		// if ((k & 0x0f) == 8)
			// printf("%*s: ", 14, "");

		// printf("%08x ", pkt->p_user[k]);
		// printf("%s ", pkt->p_user[k]);
		// printf("UDP data payload byte[%d] = (in string) %s = (in decimal %d) \n", (k-8), pkt->p_user[k], pkt->p_user[k]);
		// printf("UDP data payload byte[%d] = (in string) %s = (in decimal %d) \n", (k-8), pkt->p_user[k], pkt->p_user[k]);
			// decimal values were being displayed correctly.. When I sent ZAID as UDP payload, the correct decimals were being 
			// displayed after I checked the decimal to ASCII converter.. Need to look at whats the issue with string print

		// https://stackoverflow.com/questions/22621952/convert-char-to-string-in-c#:~:text=Store%20all%20character%20in%20an,That's%20it.
		char character;//to be scanned
		char merge[2];// this is just temporary array to merge with      
		character = pkt->p_user[k];
		merge[0] = character;
		merge[1] = '\0';
		//now you have changed it into a string

		// printf("UDP data payload byte[%d] = (in string) %s = (in decimal %d) \n", (k-8), merge, pkt->p_user[k]);
			// works

		printf("UDP data payload byte[%d] = %s \n", (k-8), merge);
		
		if ((k & 0x0f) == 0)
			printf(" ");
		else if ((k & 0x0f) == 7)
			printf("\n");
	} 

	if ((len & 0x0f) != 8)
		printf("\n");
}
