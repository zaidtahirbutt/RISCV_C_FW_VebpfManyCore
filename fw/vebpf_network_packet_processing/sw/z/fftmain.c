////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	fftmain.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	Execute a basic state machine
//
//	Upon either an incoming packet, or a complete outgoing packet, or
//	a completed FFT
//	...
//
//	1kpt FFT, requires 4kB memory
//	- Incoming data packet -> memory
//		It it doesn't match the expected packet, send a NAKA
//		If it does, ACK
//		If it's the next packet to the FFT
//			write to the FFT
//			Keep writing until all stored FFT data is written
//	- Incoming NAK
//		Repeat the requested  data
//	- FFT complete
//		Copy data to memory,
//			write first packet to the channel
//	- TX complete
//		Read FFT output to memory
//		
//	
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "pkt.h"
#include "ipproto.h"
#include "arp.h"
#include "icmp.h"
#include "protoconst.h"
#include "etcnet.h"
#include "ethproto.h"
#include "txfns.h"
#include "udpproto.h"

#define	FFTPORT	6783	// PORT number
#define	FFT_SIZE	FFT_LENGTH	// defined in board.h file #define	FFT_LENGTH	(1 << 10)  // = 'd010000000000 = 1024
// so this definition can be in a different header file


// Acknowledge all interrupts, and shut all interrupt sources off
#define	CLEARPIC	0x7fff7fff
// Turn off the master interrupt generation
#define	DISABLEINTS	0x80000000

#define	REPEATING_TIMER	0x80000000

// This running on RV32 on FPGA :3

unsigned	heartbeats = 0, lasthello; 	// Basic unsigned integer type. 2 bytes 
NET_PACKET	*waiting_pkt = NULL;

// NET_PACKET defined in pkt.h
	// Purpose:	Describes a packet that can be received, shared, processed,
	//		and transmitted again

void	fftpacket(NET_PACKET *pkt);
void	ffttimeout(void);

int	main(int argc, char **argv) {
	NET_PACKET	*rcvd;
	unsigned	now = 0, lastping = 0, lastfft = 0;
	unsigned	host_ip = DEFAULT_ROUTERIP;
	unsigned	pic;

	heartbeats = 0;
	lasthello  = 0;

	*_buspic = CLEARPIC;
	// static volatile unsigned *const _buspic = ((unsigned *)0x00600008);
	// #endif	// BUSPIC_ACCESS

	*_buspic = DISABLEINTS;

	// Clear the network reset
	_net1->n_txcmd = 0;
	// Set a network delay
	*_net1dly = 0x40; // Seems to work well  // dont see this being used in the openarty rtl
	{ // Set the MAC address
		char *macp = (char *)&_net1->n_mac;

		ETHERNET_MAC upper = DEFAULTMAC >> 32;
		unsigned	upper32 = (unsigned) upper;

		macp[1] = (upper32 >>  8) & 0x0ff;
		macp[0] = (upper32      ) & 0x0ff;
		macp[7] = (DEFAULTMAC >> 24) & 0x0ff;
		macp[6] = (DEFAULTMAC >> 16) & 0x0ff;
		macp[5] = (DEFAULTMAC >>  8) & 0x0ff;
		macp[4] = (DEFAULTMAC      ) & 0x0ff;
	}

	*_systimer = REPEATING_TIMER | (CLKFREQUENCYHZ / 10); // 10Hz interrupt

	waiting_pkt = NULL;

	printf("\n\n\n"
"+-----------------------------------------+\n"
"+----        Starting FFT test        ----+\n"
// "+----123456789               987654321----+\n"
"+-----------------------------------------+\n"
"\n\n\n");

	icmp_send_ping(host_ip);

	// We can still use the interrupt controller, we'll just need to poll it
	while(1) {
		heartbeats++;
		// Check for any interrupts
		pic = *_buspic;

		if (pic & BUSPIC_TIMER) {
			// We've received a timer interrupt
			now++;

			if ((now - lastping >= 200)&&(waiting_pkt == NULL)) {
				icmp_send_ping(host_ip);

				lastping = now;
			}
			if ((now - lastfft >= 5)&&(waiting_pkt == NULL)) {
				ffttimeout();

				lastfft = now;
			}

			if ((now - lasthello) >= 3000) {
				// Every five minutes, pause to say hello
				printf("\n\nHello, World!\n\n", *_pwrcount);
				lasthello = now;
			}

			*_buspic = BUSPIC_TIMER;
		}

		if (pic & BUSPIC_NETRX) {
			unsigned	ipsrc, ipdst;

			// We've received a packet
			rcvd = rx_pkt();
			*_buspic = BUSPIC_NETRX;	// from board.h #define	BUSPIC_NETRX	BUSPIC(7)
			if (NULL != rcvd) {

			// Don't let the subsystem free this packet (yet)
			rcvd->p_usage_count++;

			switch(ethpkt_ethtype(rcvd)) {
			case ETHERTYPE_ARP:
				// printf("RXPKT - ARP\n");
				rx_ethpkt(rcvd);
				rx_arp(rcvd); // Frees the packet
				break;
			case ETHERTYPE_IP: {
				unsigned	subproto;

				// printf("RXPKT - IP\n");
				rx_ethpkt(rcvd);

				ipsrc = ippkt_src(rcvd);
				ipdst = ippkt_dst(rcvd);
				subproto = ippkt_subproto(rcvd);
				rx_ippkt(rcvd);

				if (ipdst == my_ip_addr) {
				switch(subproto) {
					case IPPROTO_ICMP:
						if (rcvd->p_user[0]==ICMP_PING)
							icmp_reply(ipsrc, rcvd);
						// else
						//	ignore other ICMP pkts
						//
						// Free the packet
						free_pkt(rcvd);
						break;
					case IPPROTO_UDP:
						if (FFTPORT == udp_dport(rcvd)){
							rx_udp(rcvd);		
								/*
								ln = ((pkt->p_user[4] & 0x0ff) << 8)
										| (pkt->p_user[5] & 0x0ff);
								pkt->p_user   += udp_headersize(); 
								if (ln <= 8)
									pkt->p_length = 0;
								else if (pkt->p_length > ln)
									pkt->p_length = ln - udp_headersize();  // bigger than the whole ethernet packet udp packet? Chop i tup and send in smaller sized packets?
								else
									pkt->p_length -= udp_headersize();
								*/
							fftpacket(rcvd);	// fft packet transmits on the ethernet port as well
							// frees the packet
						} else
							free_pkt(rcvd);
						break;
					default:
						printf("UNKNOWN-IP -----\n");
						pkt_reset(rcvd);
						dump_ethpkt(rcvd);
						printf("\n");
						// Free the packet
						free_pkt(rcvd);
						break;
				}}}
				break;
			default:
				printf("Received unknown ether-type %d (0x%04x)\n",
					ethpkt_ethtype(rcvd),
					ethpkt_ethtype(rcvd));
				pkt_reset(rcvd);
				dump_ethpkt(rcvd);
				printf("\n");
				// Free the packet
				free_pkt(rcvd);
				break;
			}

			// Now we can free the packet ourselves
			free_pkt(rcvd);
		}} else if (_net1->n_rxcmd & ENET_RXCLRERR) {
			printf("Network has detected an error, %08x\n", _net1->n_rxcmd);
			_net1->n_rxcmd = ENET_RXCLRERR | ENET_RXCLR;
		}

		if (pic & BUSPIC_NETTX) {
			// We've finished transmitting our last packet.
			// See if another's waiting, and then transmit that.a
			if (waiting_pkt != NULL) {
				NET_PACKET	*pkt = waiting_pkt;
// txstr("Re-transmitting the busy packet\n");
				waiting_pkt = NULL;
				tx_pkt(pkt);
				
				// clearing the tx interrput here by writing back on that interrupt bit in hardware
				*_buspic = BUSPIC_NETTX;
			}
		}
	}
}



///////////////////////////////////////////////



// in tx_busy() we are just alotting waiting_pkt with the tx_pkt ... in our main() we need code that keeps checking if
// the waiting_pkt is not NULL, then try to send that waiting_pkt out.. and since this is udp, we will drop any previous
// waiting_pkt when tx_busy() is called from tx_pkt()... I'll use the same methodolgy in my code..
void	tx_busy(NET_PACKET *txpkt) {
	if (waiting_pkt == NULL) {
		// printf("TX-BUSY\n");
		waiting_pkt = txpkt;
	} else if (txpkt != waiting_pkt) {
// txstr("Busy collision--deleting waiting packet\n");
		free_pkt(waiting_pkt);
		waiting_pkt = txpkt;
	}
}

typedef enum	FFT_STATE_E {
	FFT_INPUT, FFT_OUTPUT
} FFT_STATE;

int		fft_id    = -1;
unsigned	fft_srcip = 0;
int		fft_port  = 0;
int		fft_posn  = 0;
FFT_STATE	fft_state = FFT_INPUT;

void	reset_fft(void) {
	// A basic write to the control port will reset the FFT
	*_wbfft_ctrl = 0;
}

uint16_t	pkt_uint16(NET_PACKET *pkt, int pos) {
	unsigned	v;

	v = (pkt->p_user[pos  ] & 0x0ff);
	v = (pkt->p_user[pos+1] & 0x0ff) | (v << 8);  // undoing the net_wr16t from testfft.cpp from host side
	//v = 0000 1000 ? i.e., d8 from testfft.cpp?
	return (uint16_t)v;
}

uint32_t	pkt_uint32(NET_PACKET *pkt, int pos) {
	unsigned	v = 0;

	v = (pkt->p_user[pos  ] & 0x0ff);  // WRITING BYTE BY BYTE
	v = (pkt->p_user[pos+1] & 0x0ff) | (v << 8);
	v = (pkt->p_user[pos+2] & 0x0ff) | (v << 8);
	v = (pkt->p_user[pos+3] & 0x0ff) | (v << 8);

	return v;
}

void	hton32(char *ptr, uint32_t val) {
	ptr[3] = val; val >>= 8;
	ptr[2] = val; val >>= 8;
	ptr[1] = val; val >>= 8;
	ptr[0] = val;  // size of char storage is 1 byte, to which the char var points to
}

void	fftpacket(NET_PACKET *pkt) {
	NET_PACKET	*txpkt;
	unsigned	srcip, dstip, sport, ln;

	{
		char	*puser;
		int	length;

		// the pkt here has been passed through rx_udp which does the following:
		/*
		ln = ((pkt->p_user[4] & 0x0ff) << 8)
				| (pkt->p_user[5] & 0x0ff);
		pkt->p_user   += udp_headersize(); 
		if (ln <= 8)
			pkt->p_length = 0;
		else if (pkt->p_length > ln)
			pkt->p_length = ln - udp_headersize();  // bigger than the whole ethernet packet udp packet? Chop i tup and send in smaller sized packets?
		else
			pkt->p_length -= udp_headersize();
		*/

		puser = pkt->p_user;  // eth and ip and udp headers already taken off from the functions before this function
		length= pkt->p_length;  // eth and ip and udp lengths already taken off

		pkt_reset(pkt);  
		// resetting so that we are able to use these functions rx_ethpkt then ippkt_src ippkt_dst then 
		//rx_ippkt then udp_sport
			// pkt_reset does this below
				// pkt->p_length= pkt->p_rawlen;  // this would include length of udp header aswell from the recieved udp packet
				// pkt->p_user = pkt->p_raw;
		rx_ethpkt(pkt);
			// pkt->p_user   += 8;  // go forward from the ethernet data layer and to the higher layers 
			// pkt->p_length -= 8;		// subtract the ethernet packet length from the total length
		srcip = ippkt_src(pkt);
		dstip = ippkt_dst(pkt);
		rx_ippkt(pkt);
			// pkt->p_user   += sz;  // move to higher OSI layer (e.g udp) :3
			// pkt->p_length -= sz;

		sport = udp_sport(pkt);
			// unsigned sport = ((pkt->p_user[0] & 0x0ff) << 8)
			// 	| (pkt->p_user[1] & 0x0ff);

		pkt->p_user   = puser;  // value of pointer
		pkt->p_length = length;
	}
	unsigned	pkt_id, pkt_posn;

	pkt_id   = pkt_uint16(pkt, 0);  // must be related to FFT id and FFT pos
		//pkt_id = v = 0000 1000 ? i.e., d8 from testfft.cpp?
	pkt_posn = pkt_uint16(pkt, 2);

	/*
	printf("FFT PACKET %s: src=%3d.%3d.%3d.%3d:%d FFT ID #%d, posn:%4d\n",
			(fft_state == FFT_INPUT) ? "(IN )"
			: ((fft_state == FFT_OUTPUT) ? "(OUT)"
			: "(?\?\?)"),
			(srcip >> 24)&0x0ff,
			(srcip >> 16)&0x0ff,
			(srcip >>  8)&0x0ff,
			(srcip      )&0x0ff,
			sport, pkt_id, pkt_posn);
	*/
	switch(fft_state) {
	case FFT_INPUT: {
		if ((pkt_id == fft_id) && (srcip == fft_srcip)
				&& (fft_port == sport)
				&& (pkt_posn == fft_posn)) {

			for(unsigned k=4; k < pkt->p_length; k+=4) {  // was 516 bytes un the testfft.cpp host file right?

				_wbfft_data[fft_posn] = pkt_uint32(pkt, k);  // wb is wishbone bus fft data?
				/* 
					#ifdef	WBFFT_ACCESS
					#define	_BOARD_HAS_WBFFT
					static volatile unsigned *const _wbfft_ctrl = ((unsigned *)0x00900000);
					static volatile unsigned *const _wbfft_data = (unsigned *)&_wbfft_ctrl[FFT_LENGTH];; // #define	FFT_LENGTH	(1 << 10)  // = 'd010000000000 = 1024
					#endif	// WBFFT_ACCESS
				
				*/

				// k = 4; k < pkt->p_length is // 512 +4 = 516; k+=4
					// runs for 128 times for first iteration
					// keeps on running till fft_posn = 1024 (8 times 128)

				fft_posn++;  // = 128.. do this for (fft length - 4)/4 cx k+=4 i.e, till 516 - 4/4 = 512/4 = 128  
			}

			// ACK where we are at now in our current state
			txpkt = new_udppkt(4);
				// The header length and pointer values are skipped to write the udp data payload here below
				//pkt->p_user   += udp_headersize();  // adding 8 bytes to the use char ptr
				//pkt->p_length -= udp_headersize();

			txpkt->p_user[0] = (fft_id >> 8)&0x0ff;
			txpkt->p_user[1] = (fft_id     )&0x0ff;
			txpkt->p_user[2] = (fft_posn >> 8)&0x0ff;  //fft post after 1st iteration is 128
			txpkt->p_user[3] = (fft_posn     )&0x0ff;

			//tx_udp(NET_PACKET *pkt, unsigned dest, unsigned sport, unsigned dport) {
			tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);
				// fft_srcip is source ip cx this ip it came from the Host which was the source of the pkt for our fpga device
				// fft_port is the source port from HOST.. The port number of the HOST which was the source of pkt for our device
				// FFTPORT is the SOURCE PORT # OF OUR DEVICE (FPGA) FFTPORT	6783
				// fft_port is FFTPORT + 1
				// fftsrcip is coming from HOST, but I can use this from simple_icmp_loop.c unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)

			if (fft_posn == FFT_SIZE)
				fft_state = FFT_OUTPUT;  // oh so AS SOOON AS OUR FFT INPUT WTIRES TO FFT RTL modules and fft_posn reaches 1024
										 // our FFT_STATE changes to FFT_OUTPUT, so now our FPGA will output the FFT results back to the
										 // HOST
		} else if ((pkt_id != fft_id)
				|| (fft_port != sport)) {
			reset_fft();

			fft_id = pkt_id;  // fft_id updated here
			fft_srcip = srcip;
			fft_port  = sport;
			fft_posn  = 0;

			// ACK where we are at now with this new packet
			txpkt = new_udppkt(4);

			txpkt->p_user[0] = (pkt_id >> 8)&0x0ff;  // updated pkt_id read from the incoming udp packet :3
			txpkt->p_user[1] = (pkt_id     )&0x0ff;
			txpkt->p_user[2] = 0;					// 0 FPGA pos :3
			txpkt->p_user[3] = 0;

			tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);  // #define	FFTPORT	6783  // fft_port  = sport; sport from the host pc
		} else {
			// ACK where we are at now in our current state
			txpkt = new_udppkt(4);

			txpkt->p_user[0] = (fft_id >> 8)&0x0ff;
			txpkt->p_user[1] = (fft_id     )&0x0ff;
			txpkt->p_user[2] = (fft_posn >> 8)&0x0ff;
			txpkt->p_user[3] = (fft_posn     )&0x0ff;

			tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);
		}} break;
	case FFT_OUTPUT: {
		// In this case, the user requests the result and we provide
		// it to him.
		unsigned	pkt_id, pkt_posn;

		pkt_id   = pkt_uint16(pkt, 0);
		pkt_posn = pkt_uint16(pkt, 2);
		if((pkt_id == fft_id)&&(srcip == fft_srcip)) {
			// ACK where we are at now in our current state
			if (pkt_posn >= 2*FFT_LENGTH) {  // 1024 len FFT has been written to the HOST successfully.. pkt_posn uptill 1024 was used to read 
											// 1024 len data from host,... Then pkt_posn further incremented uptill 1024x2 to now write the FFT
										   // data back to the host. After that is done the fft_state moves to FFT_INPUT to again
										  // recieve data from host to apply the FFT to using the FPGA rtl
				reset_fft();
				fft_state = FFT_INPUT;
			} else {
				ln = FFT_LENGTH - pkt_posn;  // pkt_posn starts at 1024  // FFT_LENGTH is 1024
					// ln = 0 for first iteration
				// ffttimeout() has the following assignment of ln variable
					// ln = 2*FFT_LENGTH - fft_posn;
				if (ln >= 128)
					ln = 128;
				txpkt = new_udppkt(4 + ln*4);

				txpkt->p_user[0] = (fft_id >> 8)&0x0ff;
				txpkt->p_user[1] = (fft_id     )&0x0ff;
				txpkt->p_user[2] = (fft_posn >> 8)&0x0ff;
				txpkt->p_user[3] = (fft_posn     )&0x0ff;

				for(unsigned k=0; k<ln; k++)
					hton32(&txpkt->p_user[4+k*4],
						_wbfft_data[k+fft_posn]);
				// memcpy(&txpkt->p_user[4], (void *)_wbfft_data,
				//	ln*sizeof(unsigned));

				tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);
			}
			if (pkt_posn > fft_posn)
				fft_posn = pkt_posn;
		} else if (pkt_id != fft_id) {
			reset_fft();

			fft_id    = pkt_id;
			fft_state = FFT_INPUT;
			fft_srcip = srcip;
			fft_posn  = 0;

			// ACK where we are at now with this new packet
			txpkt = new_udppkt(4);

			txpkt->p_user[0] = (pkt_id >> 8)&0x0ff;
			txpkt->p_user[1] = (pkt_id     )&0x0ff;
			txpkt->p_user[2] = 0;
			txpkt->p_user[3] = 0;

			tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);
		}} break;
	default:
		fft_id = -1;
		reset_fft();
		fft_state = FFT_INPUT;
		break;
	}

	free_pkt(pkt);
}

void	ffttimeout(void) {
	NET_PACKET	*txpkt;
	unsigned	ln;

	switch(fft_state) {
	case FFT_INPUT: if (fft_posn > 0) {

		// ACK where we are at now in our current state
		txpkt = new_udppkt(4);  // new_udppkt_v2() available now
			// here 4 means the tx udp pkt has 4 bytes of data payload which is being filled in below 

		// 4 bytes of udp data payload
		txpkt->p_user[0] = (fft_id >> 8)&0x0ff;
		txpkt->p_user[1] = (fft_id     )&0x0ff;
		txpkt->p_user[2] = (fft_posn >> 8)&0x0ff;
		txpkt->p_user[3] = (fft_posn     )&0x0ff;

// printf("FFT: TX INPUT IDLE, posn = %d\n", fft_posn);
		tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);  // tx_udp_v2 available now
		} break;
	case FFT_OUTPUT: {
		// In this case, the user requests the result and we provide
		// it to him.
		if (fft_posn < 2* FFT_SIZE) {
			// ACK where we are at now in our current state
			ln = 2*FFT_LENGTH - fft_posn;
				// fft_posn = FFT_LENGTH = 1024
				// ln = 2*FFT_LENGTH - fft_posn = 1024!!;
			if (ln >= 128)
				ln = 128;
			if (ln <= 0) {
				fft_state = FFT_INPUT;
			} else {
				txpkt = new_udppkt(4 + ln*4);

				txpkt->p_user[0] = (fft_id >> 8)&0x0ff;
				txpkt->p_user[1] = (fft_id     )&0x0ff;
				txpkt->p_user[2] = (fft_posn >> 8)&0x0ff;
				txpkt->p_user[3] = (fft_posn     )&0x0ff;

				for(unsigned k=0; k<ln; k++)
					hton32(&txpkt->p_user[4+k*4],
						_wbfft_data[k+fft_posn - FFT_SIZE]);
				// memcpy(&txpkt->p_user[4], (void *)_wbfft_data,
				//	ln*sizeof(unsigned));

				tx_udp(txpkt, fft_srcip, FFTPORT, fft_port);
			}
		}}
		break;
	default:
		fft_id = -1;
		reset_fft();
		fft_state = FFT_INPUT;
		break;
	}
}
