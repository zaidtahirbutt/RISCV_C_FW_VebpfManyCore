////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	pingtest.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	Test whether or not we can ping a given (fixed) host IP.
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
#include "board.h"
#include "pkt.h"
#include "ipproto.h"
#include "arp.h"
#include "icmp.h"
#include "protoconst.h"
#include "etcnet.h"
#include "ethproto.h"
#include "txfns.h"


// Acknowledge all interrupts, and shut all interrupt sources off
#define	CLEARPIC	0x7fff7fff  // 1111111111111110111111111111111 // 1 1111 1111 1111 1101 1111 1111 1111
// Turn off the master interrupt generation
#define	DISABLEINTS	0x80000000  // 1000 0000 0000 0000 0000 0000 0000 0000

#define	REPEATING_TIMER	0x80000000

unsigned	heartbeats = 0, lasthello;
NET_PACKET	*waiting_pkt = NULL;

int	main(int argc, char **argv) {
	NET_PACKET	*rcvd;
	unsigned	now = 0, lastping = 0;
	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define	IPADDR(A,B,C,D)	((((A)&0x0ff)<<24)|(((B)&0x0ff)<<16)|(((C)&0x0ff)<<8)|(D&0x0ff))
	unsigned	pic;

	heartbeats = 0;
	lasthello  = 0;

	*_buspic = CLEARPIC; // will come to this later #define	CLEARPIC	0x7fff7fff  // 1111111111111110111111111111111 // 1 1111 1111 1111 1101 1111 1111 1111
	*_buspic = DISABLEINTS;
	// Turn off the master interrupt generation
	// #define	DISABLEINTS	0x80000000  // 1000 0000 0000 0000 0000 0000 0000 0000
	
	/*
	#ifdef	BUSPIC_ACCESS
	#define	_BOARD_HAS_BUSPIC
	static volatile unsigned *const _buspic = ((unsigned *)0x00600008);  //11000000000000000001000
	#endif	// BUSPIC_ACCESS
	*/

	 //static volatile unsigned *const _buspic = ((unsigned *)0x00600008);  //11000000000000000001000
	// Slashing last two bits of _buspic pointer 1 1000 0000 0000 0000 0010

	// Clear the network reset
	_net1->n_txcmd = 0;	// here the txcmd bit is set to 0 here (the data here on this address _net1->n_txcmd)  
	/* 
	typedef	struct ENETPACKET_S {
		unsigned	n_rxcmd, n_txcmd;
		uint64_t	n_mac;	// 8 bytes unsigned or 64 bit
		unsigned	n_rxmiss, n_rxerr, n_rxcrc, n_txcol;
	} ENETPACKET;

	*/


	//drives the precmd register and other regs in 	// ENETPACKETS.V equal to zero

	//******************************************from enetpackets.v in the rtl folder of zipversa in the ulrich for of zipversa in my laptop START ******************************************
		// 	if ((wr_ctrl)&&(wr_addr==3'b010))	// SETTING HARDWARE MAC ADDRESS USING our RV :3
		// begin
		// // also this wr_ctrl is when we are writing to the _net1->n_txcmd or _net1->n_rxcmd since i_wb_we is here and it is 1 for writing
		// // for net1->n_txcmd
		// 	//wr_addr <= i_wb_addr[2:0]	
		// // from pingtest.c when we are reading _net1->n_rxcmd and _net1->n_txcmd
		// //(address?)n_txcmd =>  0x00500004 = b'0101 0000 0000 0000 0000 0100
		//n_rxcmd =>  0x00500000 = b'0101 0000 0000 0000 0000 0000"
			// cutting the bottom two bits and including i_wb_addr[(MAW+1):0] bits where MAW is 9,
			// so i_wb_addr[10:0] is 11 bits, so cutting n_txcmd and n_rxcmd bottom two bits and 
			// upper bits to make it 11 bits for the i_wd_addr bus makes the addresses now as
				// n_txcmd_wb => 11b'000 0000 0001
				// n_rxcmd_wb => 11b'000 0000 0000
					// sp wr_ctrl is 1 for this and now we'll see how txcmd and rxcmd is read

	{ // Set the MAC address  (device)
		// type casting the pointer
		char *macp = (char *)&_net1->n_mac;  // VIP step here that I missed that &_net1->n_mac is a struct pointer and is being casted as a char pointer so that it can be increased in terms of bytes rather than size of stucts
		// hence macp[0] and macp[1] have a difference of 1 byte since it was cast to char* ptr
		// we are giving *macp pointer the address of _net1->n_mac and setting it essentially

		ETHERNET_MAC upper = DEFAULTMAC >> 32;  // typedef	uint64_t ETHERNET_MAC;
		unsigned	upper32 = (unsigned) upper;

		//maacp[2] and macp[3] ignored in rtl
		macp[1] = (upper32 >>  8) & 0x0ff;  // char* will point to a byte of data
		macp[0] = (upper32      ) & 0x0ff;
		macp[7] = (DEFAULTMAC >> 24) & 0x0ff;
		macp[6] = (DEFAULTMAC >> 16) & 0x0ff;
		macp[5] = (DEFAULTMAC >>  8) & 0x0ff;
		macp[4] = (DEFAULTMAC      ) & 0x0ff;
	}

	//******************************************from enetpackets.v in the rtl folder of zipversa in the ulrich for of zipversa in my laptop START ******************************************
		// 	if ((wr_ctrl)&&(wr_addr==3'b010))	// SETTING HARDWARE MAC ADDRESS USING our RV :3
		// begin
		// // also this wr_ctrl is when we are writing to the _net1->n_txcmd or _net1->n_rxcmd since i_wb_we is here and it is 1 for writing
		// // for net1->n_txcmd
		// 	//wr_addr <= i_wb_addr[2:0]	
		// // from pingtest.c when we are reading _net1->n_rxcmd and _net1->n_txcmd
		// //n_txcmd =>  0x00500004 = b'0101 0000 0000 0000 0000 0100
		//n_rxcmd =>  0x00500000 = b'0101 0000 0000 0000 0000 0000"
			// cutting the bottom two bits and including i_wb_addr[(MAW+1):0] bits where MAW is 9,
			// so i_wb_addr[10:0] is 11 bits, so cutting n_txcmd and n_rxcmd bottom two bits and 
			// upper bits to make it 11 bits for the i_wd_addr bus makes the addresses now as
				// n_txcmd_wb => 11b'000 0000 0001
				// n_rxcmd_wb => 11b'000 0000 0000
					// sp wr_ctrl is 1 for this and now we'll see how txcmd and rxcmd is read

		
		// ****************************************** from zipversa pingtest.c START // ****************************************** 
		/* 
		// Clear the network reset
		_net1->n_txcmd = 0;
		{ // Set the MAC address
			char *macp = (char *)&_net1->n_mac;

			// we are giving *macp pointer the address of _net1->n_mac and setting it essentially

			ETHERNET_MAC upper = DEFAULTMAC >> 32;  // typedef	uint64_t ETHERNET_MAC;
			unsigned	upper32 = (unsigned) upper;

			macp[1] = (upper32 >>  8) & 0x0ff;  // char* will point to a byte of data
			macp[0] = (upper32      ) & 0x0ff;
			macp[7] = (DEFAULTMAC >> 24) & 0x0ff;
			macp[6] = (DEFAULTMAC >> 16) & 0x0ff;
			macp[5] = (DEFAULTMAC >>  8) & 0x0ff;
			macp[4] = (DEFAULTMAC      ) & 0x0ff;
		}
		
		//board.h file 
		typedef	struct ENETPACKET_S {
			unsigned	n_rxcmd, n_txcmd;
			uint64_t	n_mac;	// 8 bytes unsigned or 64 bit
			unsigned	n_rxmiss, n_rxerr, n_rxcrc, n_txcol;
		} ENETPACKET;

		static volatile ENETPACKET *const _net1 = ((ENETPACKET *)0x00500000);  // Ethnet data struct pointer address
		// ENETPACKET *const _net1 = 0x00500000 = 0101 0000 0000 0000 0000 0000

		// so n_mac is after 8 bytes from the base address of _net1 = 0x00500000 = 0101 0000 0000 0000 0000 0000
		// i.e. it is at &_net1->n_mac = 0x00500008 = b'0101 0000 0000 0000 0000 1000
		// &_net1->n_mac according to zipversa wishbone bus address space is, cut lower two
		// bits and add 11 bits after that as follows:
		// &_net1->n_mac_wb = b'000 0000 0010, which now satisfies the above if condition of
		// if ((wr_ctrl)&&(wr_addr==3'b010)). Which was used when &_net1->n_mac was accessed
		// in the snipped of the c code from pingtest.c above as char *macp = (char *)&_net1->n_mac;.
		// macp[1] = (upper32 >>  8) & 0x0ff; etc
		// since n_mac is uint64_t, it occupies 64 bits i.e., 8 bytes.
		// Hence for this word address of if ((wr_ctrl)&&(wr_addr==3'b010)), we are putting
		// hw_mac[47:40] <= wr_data[15:8]; and hw_mac[39:32] <= wr_data[7:0]; because in c code
		// macp[1] = (upper32 >>  8) & 0x0ff; and macp[0] = (upper32      ) & 0x0ff;
		// and when in the if condition below this, the address goes to the next word address
		// if ((wr_ctrl)&&(wr_addr==3'b011)) because we move to the next word, then the
		// bits are written as follows: Commented infront of them.

		// ****************************************** from zipversa pingtest.c END // ****************************************** 
		*/

	// 		if (wr_sel[1])
	// 			hw_mac[47:40] <= wr_data[15:8];
	// 		if (wr_sel[0])
	// 			hw_mac[39:32] <= wr_data[7:0];
	// 	end
	// 	if ((wr_ctrl)&&(wr_addr==3'b011))
	// 	begin
	// 		if (wr_sel[3])
	// 			hw_mac[31:24] <= wr_data[31:24];  // macp[7] = (DEFAULTMAC >> 24) & 0x0ff; from pingtest.c
	// 		if (wr_sel[2])
	// 			hw_mac[23:16] <= wr_data[23:16];  // from pingtest.c macp[6] = (DEFAULTMAC >> 16) & 0x0ff;
	// 		if (wr_sel[1])
	// 			hw_mac[15: 8] <= wr_data[15: 8]; // from pingtest.c macp[5] = (DEFAULTMAC >>  8) & 0x0ff;
	// 		if (wr_sel[0])
	// 			hw_mac[ 7: 0] <= wr_data[ 7: 0]; // from pingtest.c macp[4] = (DEFAULTMAC      ) & 0x0ff;
	// 	end
	// end

	//******************************************from enetpackets.v in the rtl folder of zipversa in the ulrich for of zipversa in my laptop END******************************************

	

	*_systimer = REPEATING_TIMER | (CLKFREQUENCYHZ / 10); // 10Hz interrupt

	//#define	CLKFREQUENCYHZ 50000000
	//#define	REPEATING_TIMER	0x80000000
	//static volatile unsigned *const _systimer = ((unsigned *)6291496);  //0x00600028  //110001010010001010010010110
	// cutting last two bits of systimer 1 1000 1010 0100 0101 0010 0101

	waiting_pkt = NULL;

	printf("\n\n\n"
"+-----------------------------------------+\n"
"+----       Starting Ping test        ----+\n"
// "+----123456789               987654321----+\n"
"+-----------------------------------------+\n"
"\n\n\n");

	icmp_send_ping(host_ip);  //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define	IPADDR(A,B,C,D)	((((A)&0x0ff)<<24)|(((B)&0x0ff)<<16)|(((C)&0x0ff)<<8)|(D&0x0ff))
	// from arp_lookup func in arp.c
					// so first time this above isnt entered and arp req is sent and then the processor waits for the arp reply
					// after getting arp reply the processor updates the arp table in which router mac addr is given to it
	//		○ https://www.youtube.com/watch?v=tXzKjtMHgWI&ab_channel=CertBros
	//			ARP to router example at the end

	// flow of code is first icmp ping is tried to transmitted but the router mac isnt in our device's arp table so 
	// first arp req is sent to the router which responds to this device that recieves the arp response and saves the 
	// router mac address in its arp table then this device is able to send icmp ping which is an IP packet, arp packet 
	// is not an IP packet (the Ether type of ARP packet is different from ARP packet which is in the ethernet layer 2
	// packet) whereas an ICMP packet is an IP packet.
		// I believe if I give the varibale router_mac_addr in arp.c, the true value of the router mac before compiling,
		// the device wont have to send an ARP req (you can see that in the if condition if(somethings && (router_mac_addr)), so if we give
		// this variable a value, its boolean wont be zero).

	// ICMP sent here
	// in icmp_send_ping in the function tx_pkt function in here n_txcmd is being set! 
	// After getting data from eth module in FPGA

	// We can still use the interrupt controller, we'll just need to poll it
	while(1) {
		heartbeats++;
		// Check for any interrupts  // pic is from interrupt controller I think 
		pic = *_buspic;  // _buspic pointer address points to status of the interrupts on the bus in the zipversa rtl hw,there is a bus interrupt controller in the zipversa fpga hw rtl 
		// I dont think an interrupt service routine based on interrupts to the processor are used in this c code but
		// the bus interrupt controller module in the FPGA and its interrupt outputs are being used in this c code.


		// // BUSPIC_TIMER = b 10 = 2
		/*  from rtl
			assign	bus_int_vector = {
				1'b0,
				1'b0,
				1'b0,
				1'b0,
				1'b0,
				1'b0,
				flashdbg_int,
				net1rx_int,
				net1tx_int,
				enetscope_int,
				uartrxf_int,
				uarttxf_int,
				spio_int,
				systimer_int,
				wbfft_int
			};	
		*/
		// BUSPIC_TIMER refers to systimer_int above from rtl
		if (pic & BUSPIC_TIMER) { // in the final version we will have some interrupt here (I think its a bus timer interrupt from the FPGA hw)	// also need to implement bus interrupt controller in fgpa rtl hw
			// We've received a timer interrupt
			now++;

			if ((now - lastping >= 20)&&(waiting_pkt == NULL)) { // make sure we don't have a packet waiting in line to be sent due to the hw tx pipeline being busy (FPGA ethernet module)
				icmp_send_ping(host_ip);// waiting for it? :3 waiting_pkt == NULL

				lastping = now;  
			}

			if ((now - lasthello) >= 3000) {
				printf("\n\nHello, World! Ticks since startup = 0x%08x\n\n", *_pwrcount);
				lasthello = now;  
			}

			*_buspic = BUSPIC_TIMER; // BUSPIC_TIMER = b 10 = 2
		}

		// BUSPIC_NETRX = = b 1000 0000 = d 128 = 0x80 //#define	BUSPIC_NETRX	BUSPIC(7)
		if (pic & BUSPIC_NETRX) { // interrupt telling us that we have recieved a packet  // fpga interrupt controller
			unsigned	ipsrc, ipdst;

			// We've received a packet


			//	Will study the rx pkt after tx pkt in icmp_send_ping
			rcvd = rx_pkt();
			*_buspic = BUSPIC_NETRX;  // settings of fpga interrupt controller? // BUSPIC_NETRX = = b 1000 0000 = d 128 = 0x80 
			if (NULL != rcvd) {

			// Don't let the subsystem free this packet (yet)
			rcvd->p_usage_count++;

			switch(ethpkt_ethtype(rcvd)) {
			case ETHERTYPE_ARP:        //#define	ETHERTYPE_ARP		0x0806
				printf("RXPKT - ARP\n");
				rx_ethpkt(rcvd);
				rx_arp(rcvd); // Frees the packet  // free pkt needs to happen 2 times cx we did pkt usage++ so it happens once here and then once out of case statement
				// from arp_lookup func in arp.c
					// so first time this above isnt entered and arp req is sent and then the processor waits for the arp reply
					// after getting arp reply the processor updates the arp table in which router mac addr is given to it
					//		○ https://www.youtube.com/watch?v=tXzKjtMHgWI&ab_channel=CertBros
					//			ARP to router example at the end
				break;
			case ETHERTYPE_IP: {
				unsigned	subproto;

				printf("RXPKT - IP\n");
				rx_ethpkt(rcvd);

				ipsrc = ippkt_src(rcvd);
				ipdst = ippkt_dst(rcvd);
				subproto = ippkt_subproto(rcvd);  // sub protocol?
				rx_ippkt(rcvd);

				if (ipdst == my_ip_addr) {
				switch(subproto) {
					case IPPROTO_ICMP:
						if (rcvd->p_user[0] == ICMP_PING)	icmp_reply(ipsrc, rcvd);
						else
							printf("RX PING <<------ SUCCESS!!!\n");  // our main job of icmp ping-ing is done
						// Free the packet
						free_pkt(rcvd);
						break;
					default:
						printf("UNKNOWN-IP -----\n");  //not directed at our device
						pkt_reset(rcvd);
						dump_ethpkt(rcvd);
						printf("\n");
						// Free the packet
						free_pkt(rcvd);  // removing the data
						break;
				}}}
				break;
			default:
				printf("Received unknown ether-type %d (0x%04x)\n",  // some other ether type
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

			// ENET_RXCLRERR = b'0111 1000 0000 0000 0000
			// ENET_RXCLRERR | ENET_RXCLR = 0x7C000 = d'507904 = b'0111 1100 0000 0000 0000
		}

		// #define	BUSPIC_NETTX	BUSPIC(6)  //#define BUSPIC(X) (1<<X) = 1000000 = 64  //#define BUSPIC(X) (1<<X)	
		if (pic & BUSPIC_NETTX) {  // will come back to this later 
			// so yes I was right, we have assign	o_tx_int = !tx_busy; assign	o_rx_int = (rx_valid)&&(!rx_clear);
			// pins in the zipversa code that send interrupts to the rv core when tx is not busy (i think for when its not busy) (for tx int) 
				//#define	BUSPIC_NETTX	BUSPIC(6)  //#define BUSPIC(X) (1<<X) = 1000000 = 64
				// from zipversa assign	bus_int_vector = { 1'b0,1'b0,1'b0,1'b0,	1'b0,1'b0,flashdbg_int,	net1rx_int,	net1tx_int,	enetscope_int, uartrxf_int, uarttxf_int, spio_int, systimer_int, wbfft_int};
					// the tx int net1tx_int is the 7th bit i.e., bus_int_vector[6]  #define BUSPIC(X) (1<<X)	
					// theres also net1tx_int in a rv int vector assign	picorv_int_vec = { (31)1'b0, net1rx_int,  net1tx_int, uartrxf_int, uarttxf_int, systimer_int, gpio_int};
					
			// so pic I think reads the bus interrupt rtl hw module and there is a controller for that there in the rtl hw 
			// what do the interrupts to the rv core do?
				// https://www.opensourceforu.com/2011/01/handling-interrupts/
				// https://www.geeksforgeeks.org/interrupts/
			// for initial testing of code we can go without the interrupt controller etc and just tx packets and see the ethernet pipeline simulation
			// So I dont think zipversa is using  interrupt request (irq) of riscv and instead is using its own interrupt controller

			// We've finished transmitting our last packet.
			// See if another's waiting, and then transmit that.a
			
			//
			if (waiting_pkt != NULL) {  // VIPPPP // in finished code we will have some sort of interrupt telling us that the packet transmission in FPGA hw has been completed, only then we will send this waiting packet
				NET_PACKET	*pkt = waiting_pkt;
				txstr("Re-transmitting the busy packet\n");
				waiting_pkt = NULL;  // tx_busy puts txpkt in waiting_pkt 
				tx_pkt(pkt);
				*_buspic = BUSPIC_NETTX;  // tx bit in interrupt controller set to 0 here
			}
		}
	}
}

// This function is being used in pkt.c. The txpkt() looks at the FPGA ethernet module hw tx ctrl register and if it busy
	// sending tx packet, then txpkt() uses this function to hold the txpkt till 
void	tx_busy(NET_PACKET *txpkt) {
	if (waiting_pkt == NULL) {
		// printf("TX-BUSY\n");
		waiting_pkt = txpkt;
	} else if (txpkt != waiting_pkt) {
txstr("Busy collision--deleting waiting packet\n");
		free_pkt(waiting_pkt);
		waiting_pkt = txpkt;
	}
}
