////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	pkt.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	
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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "board.h"
#include "pkt.h"
//#include "txfns.h"
#include "ethproto.h"
#define 	memorder 0
//#include "../../../../riscv_subsystem/sw/utils.h" 
// #include "simple_icmp_loop.c"  // didnt resolve the extern func not detected
// #include "../../../../riscv_subsystem/sw/utils.h"  // for printf

extern void printf(char *c, ...);  // including ../../../../riscv_subsystem/sw/utils.h in two files
								  // was giving compilation error. This extern method works.

extern void write_led(int val);  // extern means its defined somewhere else in the compiled files

#ifndef	NULL
#define	NULL	(void *)0l
#endif

// Make this constant equal to 1 if you are generating hex file for simulation 
// #define SIMULATION_TESTING 0 //1 //0

extern void	tx_busy(NET_PACKET *);  // defined in main and this is how it is able to work => https://stackoverflow.com/questions/6618921/calling-a-function-from-another-file-in-the-same-directory-in-c
void * MemcpySW(void* dst, const void* src, unsigned int cnt);
void * MemcpyLW(void* dst, const void* src, unsigned int cnt);
//void	tx_busy(NET_PACKET *); 

// tx_busy will delete the old tx_busy pkt on its second call 
	// also in pingtest.c, the processor waits for interrupt from the ethernet rtl module and when its done tx-ing the processor 
	// gets and interrupt from the fpga rtl hw and the processor then sends the waiting packet

// void	tx_busy(NET_PACKET *txpkt) {
// 	if (waiting_pkt == NULL) {
// 		// printf("TX-BUSY\n");
// 		waiting_pkt = txpkt;
// 	} else if (txpkt != waiting_pkt) {
// 		//txstr("Busy collision--deleting waiting packet\n");
// 		free_pkt(waiting_pkt);
// 		waiting_pkt = txpkt;
// 	}
// }
// defined in fftmain.c
	//NET_PACKET	*waiting_pkt = NULL;
	/* 

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

	
	*/

// https://stackoverflow.com/questions/14114749/c-function-pointer-syntax
	//  int *foo(int) would mean a function named foo that takes an int parameter and returns a pointer to an int.
	// so rx_pkt func returns NET_PACKET pointer 
NET_PACKET	*rx_pkt(void) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS
	// if (_net1->n_rxcmd & ENET_RXAVAIL) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))
	// unsigned debug = __atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder);
	// printf("entered rx_pkt(), value of _net1->rxcmd is %d \n",debug);
	// printf("entered rx_pkt(), value of _net1->rxcmd & ENET_RXAVAIL is %d \n",(debug & ENET_RXAVAIL));
	if ((__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder))  & ENET_RXAVAIL) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))

		// printf("Did enter rx_pkt() starting if ((__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder))  & ENET_RXAVAIL) { condition \n");

		// when we request for some data at a pointer address, we get some data back for that pointer address
				// either we load word (load data) or store word (store data) :3

		// &_net1->n_rxcmd 0101 0000 0000 0000 0000 0000
				// cut last two bits for wb bus &_net1_wb->n_rxcmd 0001 0100 0000 0000 0000 0000
						//from rtl  5'h00: o_wb_data <= w_rx_ctrl;	// for _net1->n_rxcmd// for // wishbone output data, address i_wb_addr[2:0] of 000 is given the ethernet outputs rx control register
        
								/* 

										w_rx_ctrl = {	// 32 bit rx control register
													rx_link_spd, !rx_full_duplex, !rx_link_up,  // 4 bits //rx_full_duplex info coming from input Ethernet frame as seen below I guess :3
													w_maw,	// 4 bits  // total bits for lenght (in bytes since its MAW+2) the memory depth register  
													{(24-20){1'b0}}, // 4 bits // 4 0s
													(rx_valid)&&(rx_broadcast)&&(!rx_clear), // 1-bit // if this bit is 1 means we have a valid rx frame right?
													rx_crcerr, rx_err, rx_miss,		// 3-bits
													// 16-bits follow
													rx_busy, (rx_valid)&&(!rx_clear),
													{(14-MAW-2){1'b0}}, rx_len };

								*/
										// so 32 bit word w_rx_ctrl is returned when we do _net1->n_rxcmd and we bitwise & it with ENET_RXAVAIL
												// w_rx_ctrl[14] = (rx_valid)&&(!rx_clear) so when we have a valid rx packet w_rx_ctrl[14] = 1 and
												// when it is &-ed with ENET_RXAVAIL, we get 1 and enter this if condition :3

        // from board.h 
        //static volatile ENETPACKET *const _net1 = ((ENETPACKET *)0x00500000);

        // are we reading the data at this mem address?? _net1->n_rxcmd

        /* from board.h

        typedef	struct ENETPACKET_S {
			unsigned	n_rxcmd, n_txcmd;
			uint64_t	n_mac;	// 8 bytes unsigned or 64 bit
			unsigned	n_rxmiss, n_rxerr, n_rxcrc, n_txcol;
		} ENETPACKET;


        */


		unsigned	rxv;  // unsigned because __atomic_load_n((unsigned*)
		unsigned         rx_len_register_accessed;
		// read rx ctrl register from ethernet rtl mod
		
		//rxv = _net1->n_rxcmd;  // so 32 bit word w_rx_ctrl is returned when we do _net1->n_rxcmd
		rxv = __atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder);  // so 32 bit word w_rx_ctrl is returned when we do _net1->n_rxcmd
		
		// printf("Inside pkt.c The 32 bit rx pkt wr_rx_ctrl control word (int) = %d \n", rxv);  
	

		unsigned pktlen = ENET_RXLEN(rxv);  // #define	ENET_RXLEN(CMD)		((CMD) & 0x03fff = b'11 1111 1111 1111) from board.h 
		
		// txcmd address was used here after rx len was equated to it in rtl
		// rx_len_register_accessed = __atomic_load_n((unsigned*)&(_net1->n_txcmd), memorder); // DIRECT RX LEN IS WRITTEN here 

		// printf("Inside pkt.c inside RTL!! (using the txcmd addressing) Rx pkt length rx_len_register_accessed = %d \n", rx_len_register_accessed);

		// we get packet len plus some extra 0s ({(14-MAW-2){1'b0}}, rx_len };) :3 
		// 

		// ask DAN this?
		// how is rx packetlength received from the FPGA by this simple instrtuction?
		// How does the memory mapping and its access work??


		// in board.h 
		//#define	ENET_RXLEN(CMD)		((CMD) & 0x03fff)
		// so this means pktlen = rxv & 0x03fff
		
		// printf("Inside pkt.c Rx pkt length = %d \n", pktlen);
			
		if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			//_net1->n_rxcmd = ENET_RXCLRERR | ENET_RXCLR;  // ENET_RXCLR = 0x004000 = 0100 0000 0000 0000
			__atomic_store_n((unsigned*)&(_net1->n_rxcmd), (ENET_RXCLRERR | ENET_RXCLR), memorder);

			printf("Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			if (rxv & ENET_RXCLRERR)	{ 
				// printf("Did enter ONLY if (rxv & ENET_RXCLRERR) \n");
				//write_led(3);  // debugging with LEDs
				}
			if ((pktlen > 2047)) { 
				// printf("Did enter ONLY if (pktlen > 2047) \n");
				//write_led(4); // debugging with LEDs 
			}

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		// NET_PACKET	*pkt = malloc(sizeof(NET_PACKET) + pktlen +2);  // why the extra 2 byte :3

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET)+ pktlen);  // no extra 2 bytes :3

		pkt->p_usage_count = 1;
		
		// pkt->p_rawlen = (ENET_RXLEN(_net1->n_rxcmd));
		//pkt->p_rawlen = (ENET_RXLEN(__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder)));
		pkt->p_rawlen = pktlen;
		

		
		pkt->p_length = pkt->p_rawlen;

		// printf("Inside pkt.c Rx pkt length (p_length version) = %d \n", pkt->p_length);
		
		// address of pointer "pkt" go sizeof(NET_PACKET) ahead of it further
		// and this is the ADDRESS of pointer p_rawlen
		pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

		pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes

		//MemcpySW((char *)_netbtx, pkt->p_user, pkt->p_length);
		//MEMCPY REPLACEMENT: has been replaced with the code above
		//memcpy((char *)_netbtx, pkt->p_user, pkt->p_length);  // = 32'b0000 0000 1000 0000 0000 1000 0000 0000    // 12th bit is 1 (1st bit start at 0) or netbtx [11] = 1 :3 also netbtx [23] = 1;  
		
		// memcpy(pkt->p_raw, (char *)_netbrx, pkt->p_rawlen+2);
		//MemcpyLW(pkt->p_raw, (char *)_netbrx, pkt->p_rawlen+2);
		
		MemcpyLW(pkt->p_raw, (char *)_netbrx, pkt->p_rawlen); // this works for the new verilog eth design
		// removed +2 from above because the simulation of 
		// zaidtahir@zaidtahir-ubuntu:~/projects/verilog-ethernet_DISL_z4/example/Arty/fpga/tb/fpga_core/test_fpga_core.py
		// was showing two extra reads by riscv

		// looked at here and realised the Endian swap problem here as well
			// the rv proc will read at addrr 0x00 for the first byte, but that byte
			// would be stored at p_raw[0], since there is word addressing in rv proc, 
			// p_raw[0] will get the LSB of the 1st word at address 0x00 where as according to
			// our c code here pkt->p_raw[0] has to be the MSB of the first word, i.e., the first
			// recieved byte in the eth rx rtl pipeline. So inorder to deal with this, I will activate
			// Endian_swap in the rx pipeline as well.
		
		// void *memcpy(void *dest, const void * src, size_t n)		// memory mapped io function
		//static volatile unsigned *const _netbrx = ((unsigned *)0x00800000);  = b'100000000000000000000000 => wb lower2 bits cut => b'10 0000 0000 0000 0000 0000
			// from rtl 5'b10???: begin  // for the 1 here, // the i_wb_addr[MAW] = netb_sel signal in main and not the address signal :3
			// o_wb_data <= rx_wb_data;
				// rx_wb_data  <= rxmem[wb_memaddr];	// wb_memaddr 10 bit address for ethernet
				// so the wb_memaddr as the pointer in the memcpy increases byte by byte since its a char pointer :3
					// but since the last two bits are cut off for the ethernet module so we get the same word for the 4 bytes from lets say
					// address 0-3, and the picorv gives the bytes from that word (the axiwstrb lines are not at play here, they are for writing
					// since for reading we have a whole processor to deal with data manipulation but for writing we donot have that and hence
					// we rely on strobes when writing)
		// here the memory at this address is copied 

		//#define	_BOARD_HAS_ENETB
		//static volatile unsigned *const _netbrx = ((unsigned *)0x00800000);
		//static volatile unsigned *const _netbtx = ((unsigned *)(0x00800000 + (0x0400<<1)));



		// _net1->n_rxcmd = ENET_RXCLRERR | ENET_RXCLR; // #define	ENET_RXCLR		0x004000
		__atomic_store_n((unsigned*)&(_net1->n_rxcmd), (ENET_RXCLRERR | ENET_RXCLR), memorder); 
		// #define	ENET_RXCLR		0x004000 // same as RXAVAIL // 0100 0000 0000 0000 i.e., the 15th index idx[14] is 1 which is rx_bram_av?
		// #define	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
		
		// how do these commands work?? _net1->n_rxcmd  = ENET_RXCLRERR | ENET_RXCLR;
		// does just typing the address give us the data contents there??

		return pkt;
	}
#endif
	return NULL;
}


// https://stackoverflow.com/questions/14114749/c-function-pointer-syntax
	//  int *foo(int) would mean a function named foo that takes an int parameter and returns a pointer to an int.
	// so rx_pkt2 func returns NET_PACKET pointer 
NET_PACKET	*rx_pkt2(void) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);
			
		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// clear rx pkt avail entry according to the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			// printf("Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET)+ pktlen);  // size in bytes right? for pktlen

		pkt->p_usage_count = 1;
	
		pkt->p_rawlen = pktlen;
		
		pkt->p_length = pkt->p_rawlen;

		pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

		pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
		
		MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr.

		// clear rx pkt avail entry according to the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		return pkt;
	}
#endif
	return NULL;
}
 

// https://stackoverflow.com/questions/14114749/c-function-pointer-syntax
	//  int *foo(int) would mean a function named foo that takes an int parameter and returns a pointer to an int.
	// so rx_pkt3 func returns NET_PACKET pointer 
NET_PACKET	*rx_pkt3(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);
			
		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// clear rx pkt avail entry according to the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			// printf("Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET)+ pktlen);

		pkt->p_usage_count = 1;
	
		pkt->p_rawlen = pktlen;
		
		pkt->p_length = pkt->p_rawlen;

		pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

		pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
		
		MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 

		// reading VeBPF results into vebpf result variables
		*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
		*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
		*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr.

		// clear rx pkt avail entry according to the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
			// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		return pkt;
	}
#endif
	return NULL;
}

// https://stackoverflow.com/questions/14114749/c-function-pointer-syntax
	//  int *foo(int) would mean a function named foo that takes an int parameter and returns a pointer to an int.
	// so rx_pkt3 func returns NET_PACKET pointer 
NET_PACKET	*rx_pkt4_VeBPF_Demo(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);

		if (!(SIMULATION_TESTING)) {
		
			printf("Inside rx_pkt4_VeBPF_Demo() rxpktlen = %d \n\n", pktlen);
		
		}		// don't need this for simulation

		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// clear rx pkt avail entry according to the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			if (!(SIMULATION_TESTING)) {
			
				printf("ERROR!!!!!!! Inside rx_pkt4_VeBPF_Demo() Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			}	// don't need this for simulation
					
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET)+ pktlen);

		pkt->p_usage_count = 1;
	
		pkt->p_rawlen = pktlen;
		
		pkt->p_length = pkt->p_rawlen;

		pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

		pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
		
		//MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 
			// commenting this out since filtering is being done by VeBPF filter and we don't need the rxpkt for anything else..

		// reading VeBPF results into vebpf result variables
		*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
		*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
		*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr.

		// clear rx pkt avail entry according to the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
			// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		// led indicating that a rxpkt was read AND rdptr was incremented
		// write_led(4);

		return pkt;
	}
#endif
	return NULL;
}

NET_PACKET	*rx_pkt_v5(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error, unsigned flag_load_rxpkt_and_VeBPF_result) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);

		if (!(SIMULATION_TESTING)) {
		
			printf("Inside rx_pkt_v5() rxpktlen = %d \n\n", pktlen);
		
		}		// don't need this for simulation

		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// clear rx pkt avail entry according to the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			if (!(SIMULATION_TESTING)) {
			
				printf("ERROR!!!!!!! Inside rx_pkt4_VeBPF_Demo() Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			}	// don't need this for simulation
					
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET)+ pktlen);

		pkt->p_usage_count = 1;
	
		pkt->p_rawlen = pktlen;
		
		pkt->p_length = pkt->p_rawlen;

		pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

		pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
		
		// Even if we don't laod the rxpkt data into, the returned "pkt" won't be NULL because we have to return a NULL 
		// for a NULL to be returned (duhh). So by not loading rxpkt data here, we'll just get a rxpkt with partially filled
		// information which would include "pkt->p_length", "pkt->p_rawlen", etc.
		if (flag_load_rxpkt_and_VeBPF_result) {
		
			MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 
				// prev comment: commenting this out since filtering is being done by VeBPF filter and we don't need the rxpkt for anything else..
				// (char *)rx_pkt_start_mem_addr typecasting "unsigned int rx_pkt_start_mem_addr" as a char pointer here

			// reading VeBPF results into vebpf result variables
			*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
			*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
			*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

		}

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr. IMP!!!

		// clear rx pkt avail entry according to the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
			// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		return pkt;
	}
#endif
	return NULL;
}

NET_PACKET	*rx_pkt_v6(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error, unsigned flag_load_rxpkt_and_VeBPF_result) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		

		if ((!(SIMULATION_TESTING)) && DEBUG) {
		
			printf("Inside rx_pkt_v6() rxpktlen = %d \n\n", pktlen);
		
		}		// don't need this for simulation

		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// clear rx pkt avail entry according to the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			if (!(SIMULATION_TESTING)) {
			
				printf("ERROR!!!!!!! Inside rx_pkt_v6() Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			}	// don't need this for simulation
					
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET) + pktlen);

		pkt->p_usage_count = 1;

		// we dont need the fields below if we dont want to load the rxpkt
		if (flag_load_rxpkt_and_VeBPF_result) {
	
			pkt->p_rawlen = pktlen;
			
			pkt->p_length = pkt->p_rawlen;

			pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

			pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
			
			unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);

			// Even if we don't laod the rxpkt data into, the returned "pkt" won't be NULL because we have to return a NULL 
			// for a NULL to be returned (duhh). So by not loading rxpkt data here, we'll just get a rxpkt with partially filled
			// information which would include "pkt->p_length", "pkt->p_rawlen", etc.
			// if (flag_load_rxpkt_and_VeBPF_result) {
		
			MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 
				// prev comment: commenting this out since filtering is being done by VeBPF filter and we don't need the rxpkt for anything else..
				// (char *)rx_pkt_start_mem_addr typecasting "unsigned int rx_pkt_start_mem_addr" as a char pointer here

			// reading VeBPF results into vebpf result variables
			*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
			*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
			*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

		}

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr. IMP!!!

		// clear rx pkt avail entry according to the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
			// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		return pkt;
	}
#endif
	return NULL;
}

NET_PACKET	*rx_pkt_v6_only_UDP_hdr(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error, unsigned flag_load_rxpkt_and_VeBPF_result) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		

		if ((!(SIMULATION_TESTING)) && DEBUG) {
		
			printf("Inside rx_pkt_v6() rxpktlen = %d \n\n", pktlen);
		
		}		// don't need this for simulation

		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// clear rx pkt avail entry according to the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			if (!(SIMULATION_TESTING)) {
			
				printf("ERROR!!!!!!! Inside rx_pkt_v6() Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			}	// don't need this for simulation
					
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET) + pktlen);

		pkt->p_usage_count = 1;

		// we dont need the fields below if we dont want to load the rxpkt
		if (flag_load_rxpkt_and_VeBPF_result) {
	
			pkt->p_rawlen = pktlen;
			
			pkt->p_length = pkt->p_rawlen;

			pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

			pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
			
			unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);

			// Even if we don't laod the rxpkt data into, the returned "pkt" won't be NULL because we have to return a NULL 
			// for a NULL to be returned (duhh). So by not loading rxpkt data here, we'll just get a rxpkt with partially filled
			// information which would include "pkt->p_length", "pkt->p_rawlen", etc.
			// if (flag_load_rxpkt_and_VeBPF_result) {
		
			// MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 
				// prev comment: commenting this out since filtering is being done by VeBPF filter and we don't need the rxpkt for anything else..
				// (char *)rx_pkt_start_mem_addr typecasting "unsigned int rx_pkt_start_mem_addr" as a char pointer here

			// for this version of the rxpkt function rx_pkt_v6_only_UDP_hdr
			// we are only reading the rxpkthdr bytes.. uptill UDP hdr.. 
				// for eth protocol hdr strip we have: pkt->p_user   += 14; 
				// for ip protocol hdr strip we have: where pkt->p_user   += sz;  // move to higher OSI layer (e.g udp) :3 
					// where sz is the IHL dynamic size of the IP hdr
					// we can think of that as the min hdr len for now = 20 bytes
			// total bytes to read for UDP header for normal IP hdr len = 14 + 20 + 8 bytes = 42 bytes but I am using MAX UDP header len
			// for the VeBPF firewall, to cater for the max IP hdr len, so I have to be uniform with that experiment and read the max UDP
			// hdr of 21 words = 84 bytes here as well.

			unsigned max_udp_hdr_len = 84;

			// when I didn't have this if condition and only had the function with "max_udp_hdr_len" constant, it didn't work cx I guess
			// the riscv was reading XXs where memory hadn't been populated yet
			if (pkt->p_rawlen > max_udp_hdr_len) {

				MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, max_udp_hdr_len); 
			
			} else {

				MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 

			}

			
			

			// reading VeBPF results into vebpf result variables
			*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
			*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
			*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

		}

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr. IMP!!!

		// clear rx pkt avail entry according to the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
			// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		return pkt;
	}
#endif
	return NULL;
}


// this function is for VeBPF filtering
void	*rx_pkt_v6_reduced() {

	// csr1 register: // csr1 gives rx pkt len and rx status words
	// unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	// if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
	
	// unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
	

	// if ((!(SIMULATION_TESTING)) && DEBUG) {
	
	// 	printf("Inside rx_pkt_v6() rxpktlen = %d \n\n", pktlen);
	
	// }		// don't need this for simulation

	// if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
		
	// 	// clear rx pkt avail entry according to the rx fifo rd ptr
	// 	__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

	// 	// increment the rx fifo rd ptr
	// 	__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

	// 	if (!(SIMULATION_TESTING)) {
		
	// 		printf("ERROR!!!!!!! Inside rx_pkt_v6() Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
		
	// 	}	// don't need this for simulation
				
	// 	write_led(5); // there is an error in rx pkt len

	// 	return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
	// }
	

	// NET_PACKET	*pkt = malloc(sizeof(NET_PACKET) + pktlen);

	// pkt->p_usage_count = 1;

	// // we dont need the fields below if we dont want to load the rxpkt
	// if (flag_load_rxpkt_and_VeBPF_result) {

	// 	pkt->p_rawlen = pktlen;
		
	// 	pkt->p_length = pkt->p_rawlen;

	// 	pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

	// 	pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
		
	// 	unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);

	// 	// Even if we don't laod the rxpkt data into, the returned "pkt" won't be NULL because we have to return a NULL 
	// 	// for a NULL to be returned (duhh). So by not loading rxpkt data here, we'll just get a rxpkt with partially filled
	// 	// information which would include "pkt->p_length", "pkt->p_rawlen", etc.
	// 	// if (flag_load_rxpkt_and_VeBPF_result) {
	
	// 	MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 
	// 		// prev comment: commenting this out since filtering is being done by VeBPF filter and we don't need the rxpkt for anything else..
	// 		// (char *)rx_pkt_start_mem_addr typecasting "unsigned int rx_pkt_start_mem_addr" as a char pointer here

	// 	// reading VeBPF results into vebpf result variables
	// 	*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
	// 	*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
	// 	*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

	// }

	// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr. IMP!!!

	// clear rx pkt avail entry according to the rx fifo rd ptr
	__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
		// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

	// increment the rx fifo rd ptr
	__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

	// return pkt;
	// }
// #endif
	// return NULL;
}


// function for testing for bugs
NET_PACKET	*rx_pkt4_VeBPF_Demo_debug_v1(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	rx_csr1  = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);  

	if (NET_RX_PKT_AVAIL(rx_csr1) && (!NET_RX_FIFO_EMPTY(rx_csr1))) {		//#define	ENET_RXAVAIL		0x004000 = 0100 0000 0000 0000 (1 on bit no [14] (15th bit))  
		
		unsigned pktlen = NET_RX_PKT_LEN(rx_csr1);  
		unsigned rx_pkt_start_mem_addr = __atomic_load_n((unsigned*)&(_net_csrs->csr2), memorder);

		if (!(SIMULATION_TESTING)) {
		
			printf("Inside rx_pkt4_VeBPF_Demo() rxpktlen = %d \n\n", pktlen);
		
		}		// don't need this for simulation

		if ((pktlen > 2047)) {  // 	ENET_RXCLRERR		(ENET_RXMISS|ENET_RXERR|ENET_RXCRC|ENET_RXBUSY)
			
			// commenting out clear rx pkt avail as a seeded fault to check for bugs in the code
			// clear rx pkt avail entry according to the rx fifo rd ptr
			//__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);

			// increment the rx fifo rd ptr
			__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);

			if (!(SIMULATION_TESTING)) {
			
				printf("Inside rx_pkt4_VeBPF_Demo() Entered rx_pkt() Error condition, i.e., if ((rxv & ENET_RXCLRERR)||(pktlen > 2047)) condition \n");
			
			}	// don't need this for simulation
					
			write_led(5); // there is an error in rx pkt len

			return NULL;  // the rcvd pkt is greater than its max posisble length so just discard it and send error and clear bits to rx ctrl eth rtl registers
		}
		

		NET_PACKET	*pkt = malloc(sizeof(NET_PACKET)+ pktlen);

		pkt->p_usage_count = 1;
	
		pkt->p_rawlen = pktlen;
		
		pkt->p_length = pkt->p_rawlen;

		pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // Points to the beginning of raw packet memory

		pkt->p_user = &pkt->p_raw[0];  // VIP // p_user; is Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
		
		//MemcpyLW(pkt->p_raw, (char *)rx_pkt_start_mem_addr, pkt->p_rawlen); 
			// commenting this out since filtering is being done by VeBPF filter and we don't need the rxpkt for anything else..

		// reading VeBPF results into vebpf result variables
		*vebpf_dest = NET_RX_PKT_VeBPF_DEST(rx_csr1);
		*vebpf_valid = NET_RX_PKT_VeBPF_VALID(rx_csr1);
		*vebpf_error = NET_RX_PKT_VeBPF_ERROR(rx_csr1);

		// clear and inc in this order always. Inc rd ptr at the end cx clr needs to go to the current rd ptr.

		// commenting out clear rx pkt avail as a seeded fault to check for bugs in the code
		// clear rx pkt avail entry according to the rx fifo rd ptr
		// __atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_PKT_AVAIL_CLEAR, memorder);	// 0b1000
			// remember to clear the rx pkt first the increment the rx pkt rd ptr cx some rtl logic depends on this flow

		// increment the rx fifo rd ptr
		__atomic_store_n((unsigned*)&(_net_csrs->csr1), NET_RX_FIFO_RD_PTR_INC, memorder);	// 0b0001 0000

		// led indicating that a rxpkt was read AND rdptr was incremented
		// write_led(4);

		return pkt;
	}
#endif
	return NULL;
}

void	pkt_reset(NET_PACKET *pkt) {
	if (NULL == pkt)
		return;

	pkt->p_length= pkt->p_rawlen;
	pkt->p_user = pkt->p_raw;
}

void	tx_pkt(NET_PACKET *pkt) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// #define	ENET_TXBUSY		0x004000 = 24'b0000 0000 0100 0000 0000 0000

	// unsigned debug; // delete this later

	// if (_net1->n_txcmd & ENET_TXBUSY)
	if ((__atomic_load_n((unsigned*)&(_net1->n_txcmd), memorder)) & ENET_TXBUSY){
		// debug = __atomic_load_n((unsigned*)&(_net1->n_txcmd), memorder);
		
		// _net1->n_txcmd & ENET_TXBUSY part:
			// _net1->n_txcmd is the processor reading from the address of the pointer that we declared as _net1 in our board.h file which was a struct ENETPACKET pointer
			// n_txcmd is the second element of the struct, the first is n_rx_cmd, both are variable type unsigned (unsigned int i.e., 4 bytes)
			// The address of pointer _net1 is:
				//static volatile ENETPACKET *const _net1 = ((ENETPACKET *)0x00500000) = 32'b0000 0000 0101 0000 0000 0000 0000 0000; // *const means pointer is constant and cannot be changed :3 
	 		// so the address the processor send when it wants to read _net1->n_txcmd is 
	 		// &_net1->n_txcmd = _net1 + 0x4 = 0x00500004 = 32'b0000 0000 0101 0000 0000 0000 0000 0100
			// Then the data received by processor will be bit-wise and-ed with ENET_TXBUSY = 0x004000 = 24'b0000 0000 0100 0000 0000 0000
			// What is that data exactly that will be received when memory address (&_net1->n_txcmd = _net1 + 0x4 = 0x00500004) of _net1->n_txcmd is read?
				// It will be the transmit control (w_tx_ctrl) word that is being stored by the ethernet rtl as shown below the edgetestversion comments below
					// the 15th bit of tx ctrl reg is tx_busy which is being matched with ENET_TXBUSY 0x004000 => 0100 0000 0000 0000
					// when it is being bit wise anded with it so if it busy, then we get tx_busy here, if not then we send data
						// w_tx_ctrl & ENET_TXBUSY 0x004000 = 24'b0000 0000 0100 0000 0000 0000
							// w_tx_ctrl[14] = tx_busy & ENET_TXBUSY (it has one at ENET_TXBUSY[14])
								// so if the transmitter is busy and tx_busy bit is 1, it will enter this if condition and
								// tx_busy(pkt); function will be called which will save the packet on its first call and then save the new packet and delete the old on on its second call
								// in the while1 loop of pingtest.c there is a if condition that checks the interrupt from the ethernet rtl hw that I think is generated after tx is complete and sent to processor
								// in any case that if statement sends the waiting packet again using tx_pkt() i.e., this function
									// so yes I was right, we have assign	o_tx_int = !tx_busy; assign	o_rx_int = (rx_valid)&&(!rx_clear);
									// pins in the zipversa code that send interrupts to the rv core when tx is not busy (i think for when its not busy) (for tx int) 
										// From pingtest.c regarding using interrupts for waiting packets:
											// so yes I was right, we have assign	o_tx_int = !tx_busy; assign	o_rx_int = (rx_valid)&&(!rx_clear);
											// pins in the zipversa code that send interrupts to the rv core when tx is not busy (i think for when its not busy) (for tx int) 
												//#define	BUSPIC_NETTX	BUSPIC(6)  //#define BUSPIC(X) (1<<X) = 1000000 = 64
												// from zipversa assign	bus_int_vector = { 1'b0,1'b0,1'b0,1'b0,	1'b0,1'b0,flashdbg_int,	net1rx_int,	net1tx_int,	enetscope_int, uartrxf_int, uarttxf_int, spio_int, systimer_int, wbfft_int};
													// the tx int net1tx_int is the 7th bit i.e., bus_int_vector[6]
													// theres also net1tx_int in a rv int vector assign	picorv_int_vec = { (31)1'b0, net1rx_int,  net1tx_int, uartrxf_int, uarttxf_int, systimer_int, gpio_int};
											// so pic I think reads the bus interrupt rtl hw module and there is a controller for that there in the rtl hw 
											// what do the interrupts to the rv core do?
												// https://www.opensourceforu.com/2011/01/handling-interrupts/
												// https://www.geeksforgeeks.org/interrupts/
											// for initial testing of code we can go without the interrupt controller etc and just tx packets and see the ethernet pipeline simulation
			// conclusion is: we can change the address of _net1, the opertaion of _net1->n_txcmd & ENET_TXBUSY happens to the data i.e., the tx control word recieved as a result

		// EDGETESTBED version _net1->n_txcmd & ENET_TXBUSY part:
		 	// Edgetestbed version will have a 1 appended at the MSb of _net1 so 
		 	// &_net1->n_txcmd = _net1 + 0x4 = 0x20500004 = 32'b0010 0000 0101 0000 0000 0000 0000 0100
				// crossbar will remove the 2 on the MSb considering our _net1 STARTADDR is 0x20000000
			// rest _net1->n_txcmd & ENET_TXBUSY is happening on the recieved data i.e., the tx control word

	



		// the data at address of _net1->n_txcmd (n_txcmd =>  0x00500004 = 0101 0000 0000 0000 0000 0100)
		// is being fetched by the rv process and that data is w_tx_ctrl reg
		// need to get used to this idea of data being fetched after address of a pointer is sent

			/* w_tx_ctrl = { tx_spd, 2'b00, w_maw, {(24-20){1'b0}}, // // 32 bit tx control register
			((RXSCOPE) ? 1'b0:1'b1),
			!config_hw_ip_check,	// command from rv to check hw ip and hw mac? // command from rv to check crc
			!o_net_reset_n,!config_hw_mac,
			// 16 bits follow
			!config_hw_crc, tx_busy, // command from rv to check crc
				{(14-MAW-2){1'b0}}, tx_len };
				*/
			// From my DISL ethernet code the tx ctrl is as below (using openarty x zipversa)/ Here the 

			/* w_tx_ctrl = { 4'b00, w_maw, {(24-20){1'b0}},
			((RXSCOPE) ? 1'b0:1'b1),
			!config_hw_ip_check,
			!o_net_reset_n,!config_hw_mac,
			// 16 bits follow
			!config_hw_crc, tx_busy,
				{(14-MAW-2){1'b0}}, tx_len };  // tx_len is MAW+1:0 i.e., 10:0 or 11 bits, 14-MAW-2 = 3, so total bits are 13:0 (14 bits),
				// tx_busy is at index w_tx_ctrl[14] and it means its the 15th bit, right most bit here is LSb
				*/
				// so tx_busy is ALSO the 15th bit in DISL rtl

		// the 15th bit of tx ctrl reg is tx_busy which is being matched with ENET_TXBUSY 0x004000 => 0100 0000 0000 0000
		// when it is being bit wise anded with it so if it busy, then we get tx_busy here, if not then we send data
		// ************************************* VIP above **********************************************************



		// this is v imp. How does this tell us that tx is busy?
		// what is _net1->n_txcmd and how is it set? My suspicion is that it is being set 
		// by ethernet module rtl, since its address space lies in that of the ethernet rtl
		// let me confirm from there! 
		// when we want to write to _net1->n_txcmd or even when we just want to read what is at
		// "_net1->n_txcmd" this memory location, for reading the cpu will send a load word command
		// to this memory address which means that the cpu will send out a memory address at its output 
		// memory location and since address of 
		// _net1 is ENETPACKET *const _net1 = 0x00500000 = 0101 0000 0000 0000 0000 0000
		// and typedef	struct ENETPACKET_S { 	unsigned	n_rxcmd, n_txcmd; uint64_t	n_mac;	
		// 8 bytes unsigned or 64 bit unsigned	n_rxmiss, n_rxerr, n_rxcrc, n_txcol; } ENETPACKET;
		// This also means n_rxcmd is at 
		// n_rxcmd => "ENETPACKET *const _net1 = 0x00500000 = b'0101 0000 0000 0000 0000 0100" address location 
		// and n_txcmd is located at 4 bytes ahead of n_rxcmd address which is
		// n_txcmd =>  0x00500004 = 0101 0000 0000 0000 0000 0100, so when reading this data the cpu
		// will send this address to the address line and will wait for the data to arrive
		// at the input data line. Also same for writing, if cpu want to make _net1->n_txcmd = 0,
		// the cpu will send this address n_txcmd =>  0x00500004 = 10100000000000000000100 and
		// the data to be written along with it to the output data line! 
		// Will confirm this in the rtl now!!!!!!!

		//This described above is a new concept and application of memory mapped io for me in computer architecture!!


		tx_busy(pkt);  // so we wait for the busy packet to complete tx :3  // we count the waiting packet in the while 1 loop in main
	} 
	else {
		unsigned	txcmd;

		// so now we have the complete packet with the ICMP, IP then ETHERNET info. We have not added SRC MAC address, CRC at the end of ethernet
		// and no preamble as well cx the hardware/FPGA is adding it as seen in the rtl file
		// also the packet length is 36bytes which is less than the min packet length, I think the FPGA hw is reponsible for adding the extra bits to make a valid ethernet packet

		// so now we AL launch this packet to the ethernet by writing it on memorymapped io address of _netbtx
			// p_length is 36 bytes as was updated during the packet formation as it was passing thru each function


		

		MemcpySW((char *)_netbtx, pkt->p_user, pkt->p_length);

		//MEMCPY REPLACEMENT: has been replaced with the code above
		//memcpy((char *)_netbtx, pkt->p_user, pkt->p_length);  // = 32'b0000 0000 1000 0000 0000 1000 0000 0000    // 12th bit is 1 (1st bit start at 0) or netbtx [11] = 1 :3 also netbtx [23] = 1;  
		
		//static volatile unsigned *const _netbtx = ((unsigned *)(0x00800000 + (0x0400<<1)));  // 0x400<<1 is 0x800 = d'2048 = b'1000 0000 0000
		// 0x800 + 0x00800000 = 0x800800 = d'8390656 = b'1000 0000 0000 1000 0000 0000    // 12th bit is 1 (1st bit start at 0) or netbtx [11] = 1 :3 also netbtx [23] = 1;  

		//https://aticleworld.com/how-to-use-memcpy-and-how-to-write-your-own-memcpy/
			// I think thats why doing char* cast to _netbtx ptr
			// the src pointer and dest pointr is increased by 1 byte for each byte copy in a for loop :3

		//https://www.tutorialspoint.com/c_standard_library/c_function_memcpy.htm
		// void *memcpy(void *dest, const void * src, size_t n)		// memory mapped io function

		//static volatile unsigned *const _netbtx = ((unsigned *)(0x00800000 + (0x0400<<1)));  // 0x400<<1 is 0x800 = d'2048 = b'1000 0000 0000
		// 0x800 + 0x00800000 = 0x800800 = d'8390656 = b'1000 0000 0000 1000 0000 0000    // 12th bit is 1 (1st bit start at 0) or netbtx [11] = 1 :3 also netbtx [23] = 1;
		// dont see addresses in the zipversa main file for the memcpy of the btx and brx address in c file for icmppingsend

			// now leme check this address and values in zipversa since it doesnt have any crossbar. Leme see those conditions :3

		//static volatile unsigned *const _netbrx = ((unsigned *)0x00800000); 0x00800000 = d'8388608 = b'1000 0000 0000 0000 0000 0000

		// EDGETESTBED _netbtx adress:
			// static volatile unsigned *const _netbtx = ((unsigned *)(0x20800000 + (0x0400<<1)));  // 0x400<<1 is 0x800 = d'2048 = b'1000 0000 0000
				// 0x800 + 0x20800000 = 0x20800800 = d'8390656 = b'0010 0000 1000 0000 0000 1000 0000 0000    // 12th bit is 1 (1st bit start at 0) or netbtx [11] = 1 :3 also netbtx [23] = 1;

		txcmd = ENET_TXGO | pkt->p_length;	// #define	ENET_TXGO		0x004000  // p_length is 36 bytes as was updated during the packet formation as it was passing thru each function
		txcmd |= ENET_NOHWIPCHK;	// we arent checking if the output packet is IP packet? :3	// #define	ENET_NOHWIPCHK		0x040000
		
		 
		// ****************************************** THIS WAS WRONG START HERE ********************************************
		// config_hw_ip_check is in the tx ctrl reg since we know after sending a tx packet (ARP) that we might need to disable the
		// config_hw_ip_check or enable it if we are sending an ARP packet or IP or whatever
			// ENET_NOHWIPCHK 1000000000000000000 => wishbone bus cutting of lower two bits => 1 0000 0000 0000 0000
				// output data from rv at txcmd for ENET_NOHWIPCHK that is input wb data for the ethernet rtl is i_wb_data is 1 at 
				// i_wb_data[16] and 0 at i_wb_data[18]
					/* from rtl
						config_hw_ip_check <= (!wr_data[18]);	// command from rv to check hw ip and hw mac? 
						o_net_reset_n <= (!wr_data[17]);
						config_hw_mac <= (!wr_data[16]);
					*/
						//config_hw_ip_check is 1 then :P but what happens in ARP packet tx?
						// ENET_TXGO 100000000000000 => cut two bits for wb bus => 1 0000 0000 0000
		// ****************************************** THIS WAS WRONG END HERE ********************************************
		// ******************************* SOLUTION TO THE WRONG Explanation START ***********************************************
			/*
				The explanation I gave for the wishbone bus cutting of the lower two bits of txcmd |= ENET_NOHWIPCHK; as
				"ENET_NOHWIPCHK 1000000000000000000 => wishbone bus cutting of lower two bits => 1 0000 0000 0000 0000" and then I 
				said that "output data from rv at txcmd for ENET_NOHWIPCHK that is input wb data for the ethernet rtl is i_wb_data is 1 at 
				i_wb_data[16] and 0 at i_wb_data[18]" which was completely wrong since this is not the address that I am dealing with, this
				is the data bus not the address bus and there is no bit cutting at the data bus for the wishbone in zipcpu for the ethernet
				rtl module!!! Hence  txcmd |= ENET_NOHWIPCHK = 0100 0000 0000 0000 0000; means that i_wb_data[18] is 1 which means in the rtl
				it becomes 0 since it is being not-ted which disables the IPchecking module in the rx pipeline.
			
			******************************* SOLUTION TO THE WRONG Explanation END ***********************************************

			*/

		// now it is understood how plength and ENET_NOHWIPCHK get transferred to rtl hardware. See exp below the 
		// _net1->n_txcmd = txcmd; command.

		// 36 = 0x24
		// txcmd = 0x44024 :3

		// _net1->n_txcmd has p_length and ENET_TXGO and ENET_NOHWIPCHK all in one and is saved in this particular variable now
		
		// Replacing with Atomic below
		//_net1->n_txcmd = txcmd;
		__atomic_store_n((unsigned*)&(_net1->n_txcmd), txcmd, memorder);


		// writing to tx control register in the ethernet FPGA rtl hw 
			//ENET_TXGO means start transmitting?

		// writing data to the pointer location :3 data outputted at the bus output line of the picorv and its up to us what to do with that data
		// whether to write it somewhere and save it, or just use that data instantaneously just like what we are doing in the memory mapped io
		// ENETPACKET.V peripheral	


		// important registers being set in the rtl module with this as follows
		/* if ((wr_ctrl)&&(wr_addr==3'b001))
		begin	
		// this is for //n_txcmd =>  0x00500004 = b'0101 0000 0000 0000 0000 0100
		// n_txcmd_wb => 11b'000 0000 0001

		// also this wr_ctrl is when we are writing to the _net1->n_txcmd or _net1->n_rxcmd since i_wb_we is here and it is 1 for writing
		// for net1->n_txcmd
			//wr_addr <= i_wb_addr[2:0]
			//wr_ctrl<=((i_wb_stb)&&(i_wb_we)		// wr is wishbone read? wishbone bus control. Logical and-ing wishbone strobe (here strobe means the bus is active or not I thinkg)
				//&&(i_wb_addr[(MAW+1):MAW] == 2'b00));  // MAW = 9 here

		// from pingtest.c when we are reading _net1->n_rxcmd and _net1->n_txcmd
		//n_txcmd =>  0x00500004 = b'0101 0000 0000 0000 0000 0100
		//n_rxcmd =>  0x00500000 = b'0101 0000 0000 0000 0000 0000"
			// cutting the bottom two bits and including i_wb_addr[(MAW+1):0] bits where MAW is 9,
			// so i_wb_addr[10:0] is 11 bits, so cutting n_txcmd and n_rxcmd bottom two bits and 
			// upper bits to make it 11 bits for the i_wd_addr bus makes the addresses now as
				// n_txcmd_wb => 11b'000 0000 0001
				// n_rxcmd_wb => 11b'000 0000 0000
					// sp wr_ctrl is 1 for this and now we'll see how txcmd and rxcmd is read

		 // TX command register TX COMMAND REG BEING ACCESSED!!!!!!!!!!!!! VIP!!!
				// ACCORDING TO THE INPUT DATA!!!

			// Set the transmit speed
			if (wr_sel[3] && wr_data[24])
			begin
				tx_spd <= wr_data[31:30];	// options from 2 bits. Dont know what they are :3
			end

			// the speed is already set in open arty so I dont need this tx_spd here


			// Reset bit must be held down to be valid
			if (wr_sel[2])
			begin
				config_hw_ip_check <= (!wr_data[18]);	// command from rv to check hw ip and hw mac? 
				o_net_reset_n <= (!wr_data[17]);
				config_hw_mac <= (!wr_data[16]);
			end

			if (wr_sel[1])
			begin
				config_hw_crc <= (!wr_data[15]);	// command from rv to check crc
				pre_cmd <= (wr_data[14]);
				tx_cancel <= (tx_busy)&&(!wr_data[14]);
			end

			if (wr_sel[1:0] == 2'b11)
			begin
//		14'h0	| SW-CRCn |NET-RST|BUSY/CMD | 14 bit length(in octets)|

//	1	Transmitter control 		// 14 bit reg
//		14'h0	|NET_RST|SW-MAC-CHK|SW-CRCn|BUSY/CMD | 14 bit length(in octets)|

				if (!tx_cmd && !tx_busy)
					tx_len <= wr_data[(MAW+1):0]; // 12 bits for WRITE DATA here
			end
		end 
*/

		// here n_txcmd is being set! After getting data from eth module in FPGA 
		// is n_txcmd sent to FPGA Ethernet module somehow???? To set the ethernet output controls regs?


		// dump_raw(pkt);
		// dump_ethpkt(pkt);
		free_pkt(pkt);
	}
#else
	free_pkt(pkt);
#endif
}

void	tx_pkt_v2(NET_PACKET *pkt) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// // Debugging info
	// if ((DEBUG) || (SIMULATION_TESTING)) {


	// 	if (SIMULATION_TESTING && DEBUG) {

	// 		// printing error message
	// 		printf("There is an error in tx_ippkt_v2");

	// 		// led = 5 means there is an error
	// 		write_led(5); 

	// 	} else if (SIMULATION_TESTING) {

	// 		// led = 5 means there is an error
	// 		write_led(5); 

	// 	} else if (DEBUG) {

	// 		// printing error message
	// 		printf("There is an error in tx_ippkt_v2");

	// 		// led = 5 means there is an error
	// 		write_led(5); 

	// 	}

	// } 

	// We need to check if tx_pkt len is less than 1514 bytes (Additional 4 bytes of CRC will be appended at the end of the tx_pkt) 
	// otherwise drop the packet and raise an error message that tx_pkt len is too long	
	if (pkt->p_length > 1518) {

		// drop packet
		// dump_ippkt(pkt);
			// dump meaning print the packet details
		free_pkt(pkt);

		// Output debugging info
		if ((DEBUG) || (SIMULATION_TESTING)) {


			if (SIMULATION_TESTING && DEBUG) {

				// printing error message
				printf("There is an error in tx_ippkt_v2");

				// led = 5 means there is an error
				write_led(5); 

			} else if (SIMULATION_TESTING) {

				// led = 5 means there is an error
				write_led(5); 

			} else if (DEBUG) {

				// printing error message
				printf("There is an error in tx_ippkt_v2");

				// led = 5 means there is an error
				write_led(5); 

			}

		} 

	}

	// check if desc_table_tx is not full
		// checking outside this function.. before it is called

	// if desc_table_tx is full then return with tx_busy ORRRR check if desc_table_tx is full outside tx_pkt_v2 function so that
	// we dont' have to worry about checking this condition inside the tx_pkt_v2() function and we can simple focus on transmitting the
	// tx_pkt.. 

	// After we Check if desc_table_tx is full or not then we need to check if the length of our tx_pkt can fit into the available memory we have left for tx_pkts

	// tx_pkt_active[wr_ptr] will also be checked and if it's value is 1 then the code will wait till it's value is not 1 anymore. 

	// If all these check pass, we call the tx_pkt_v2() function. This function needs the current starting memory address for the current tx_pkt... the tx_pkt..
	// this function also needs starting memory address of the current tx_pkt passed by reference so that when we increment it after the tx_pkt is transmitted
	// so that we can keep track of the current starting memory address of the next tx_pkt and that we can keep track if that address is exceeding or rolling-over
	// the upper limits for the memory range allocated to tx_pkts.. If we can not fit a tx_pkt within the upper range of the memory range allocated to tx_pkts
	// we will wait for another tx_pkt to be transmitted by comparing rd_ptr and wr_ptr of desc_table.. and we will check how much memory we have now at the 
	// base address..
		// one imp point here....... how to track the length of the tx_pkt that was transmitted by network subsystem and how much memory we need to add back to 
		// available memory..?

	// This function will write the starting memory address of the current tx_pkt to the desc_table_tx entry wr_ptr in the network subsystem
	// then the tx_pkt length in bytes and words to the desc_table_tx entry wr_ptr in the network subsystem

	// This function will then write the tx_pkt to the memory address starting at the starting memory address of the current tx_pkt that was 
	// written to desc_table_tx in network subsystem

	// 


	// if (_net1->n_txcmd & ENET_TXBUSY)
	if ((__atomic_load_n((unsigned*)&(_net1->n_txcmd), memorder)) & ENET_TXBUSY){


		tx_busy(pkt);  // so we wait for the busy packet to complete tx :3  // we count the waiting packet in the while 1 loop in main

	} 
	else {

		unsigned	txcmd;

		MemcpySW((char *)_netbtx, pkt->p_user, pkt->p_length);

		txcmd = ENET_TXGO | pkt->p_length;	// #define	ENET_TXGO		0x004000  // p_length is 36 bytes as was updated during the packet formation as it was passing thru each function
		txcmd |= ENET_NOHWIPCHK;	// we arent checking if the output packet is IP packet? :3	// #define	ENET_NOHWIPCHK		0x040000
		
		// Replacing with Atomic below
		//_net1->n_txcmd = txcmd;
		__atomic_store_n((unsigned*)&(_net1->n_txcmd), txcmd, memorder);

		free_pkt(pkt);
	}

#else

	free_pkt(pkt);

#endif

}

// tx_pkt_v3 is for TX of tx_pkt from memory by network subsystem
void	tx_pkt_v3(NET_PACKET *pkt, void *tx_pkt_current_start_mem_address) {
#define NET1_ACCESS
#ifdef	NET1_ACCESS

	// We need to check if tx_pkt len is less than 1514 bytes (Additional 4 bytes of CRC will be appended at the end of the tx_pkt) 
	// otherwise drop the packet and raise an error message that tx_pkt len is too long	
	if (pkt->p_length > 1518) {

		// drop packet
		// dump_ippkt(pkt);
			// dump meaning print the packet details
		free_pkt(pkt);

		// Output debugging info
		if ((DEBUG) || (SIMULATION_TESTING)) {


			if (SIMULATION_TESTING && DEBUG) {

				// printing error message
				printf("There is an error in tx_pkt_v3");

				// led = 5 means there is an error
				write_led(5); 

			} else if (SIMULATION_TESTING) {

				// led = 5 means there is an error
				write_led(5); 

			} else if (DEBUG) {

				// printing error message
				printf("\nThere is an error in tx_pkt_v3\n");

				// led = 5 means there is an error
				write_led(5); 

				// dont wana add delay in tx_pkt function

			}

		} 

	}

	if (SIMULATION_TESTING) {
		write_led(15);
		write_led(0);
		write_led(15);	
	}


	// writing the tx_pkt to the memory location we calculated earlier, the starting memory address
	MemcpySW(tx_pkt_current_start_mem_address, pkt->p_user, pkt->p_length);

	if (SIMULATION_TESTING) {
		write_led(9);
		write_led(0);
		write_led(9);
		write_led(0);
	}

	// now update the desc_table_tx entries 

	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and (unsigned)tx_pkt_current_start_mem_address = %d ******\n\n\n", (unsigned)tx_pkt_current_start_mem_address);

	// update "desc_table_tx_pkt_start_mem_addr" desc_table_tx entry
	__atomic_store_n((unsigned*)&(_net_csrs->csr11_tx1), (unsigned)tx_pkt_current_start_mem_address, memorder);

	// calculating word size of tx_pkt for writing it to desc_table_tx
	unsigned tx_pkt_word_size = 0;

	if ((pkt->p_length & 0x03) > 0) {

		tx_pkt_word_size = (pkt->p_length >> 2) + 1;

	} 
	else {

		tx_pkt_word_size = (pkt->p_length >> 2);

	}

	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and tx_pkt_word_size = %d ******\n\n\n", tx_pkt_word_size);

	// update "desc_table_tx_pkt_len_words" desc_table_tx entry
	__atomic_store_n((unsigned*)&(_net_csrs->csr10_tx1), tx_pkt_word_size, memorder);	

	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and pkt->p_length = %d ******\n\n\n", pkt->p_length);

	// update "desc_table_tx_pkt_len" desc_table_tx entry
	__atomic_store_n((unsigned*)&(_net_csrs->csr9_tx1), pkt->p_length, memorder);

	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and updating csr12_tx1 NET_TX_AVAIL_BIT = %d ******\n\n\n", NET_TX_AVAIL_BIT);

	// update "desc_table_tx_pkt_avail" desc_table_tx entry
	__atomic_store_n((unsigned*)&(_net_csrs->csr12_tx1), NET_TX_AVAIL_BIT, memorder);

	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and Increment the wr_ptr using wr_fifo_ptr_tx_pkt_inc_reg desc_table_tx entry updating csr5_tx1  = %d ******\n\n\n", NET_TX_FIFO_WR_PTR_INC);	
	
	if(!SIMULATION_TESTING)
		printf("\n\n\n****** With DEBUG = 0, Inside tx_pkt_v3() and Increment the wr_ptr using wr_fifo_ptr_tx_pkt_inc_reg desc_table_tx entry updating csr5_tx1  = %d ******\n\n\n", NET_TX_FIFO_WR_PTR_INC);	
	
	// Getting stuck after thie write... the riscv is missing reads after getting control back from network subsystem after it has read the txpkt from memory

	write_led(15);

	
	// // Increment the wr_ptr using "wr_fifo_ptr_tx_pkt_inc_reg" desc_table_tx entry
	__atomic_store_n((unsigned*)&(_net_csrs->csr5_tx1), NET_TX_FIFO_WR_PTR_INC, memorder);

	// we are stuck here in SYN.. and all leds are LIT
	

	// unsigned	txcmd;


	// txcmd = ENET_TXGO | pkt->p_length;	// #define	ENET_TXGO		0x004000  // p_length is 36 bytes as was updated during the packet formation as it was passing thru each function
	// txcmd |= ENET_NOHWIPCHK;	// we arent checking if the output packet is IP packet? :3	// #define	ENET_NOHWIPCHK		0x040000
	
	// // Replacing with Atomic below
	// //_net1->n_txcmd = txcmd;
	// __atomic_store_n((unsigned*)&(_net1->n_txcmd), txcmd, memorder);


	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and entering free_pkt(pkt) ******\n\n\n");	
		
	if(!SIMULATION_TESTING)
		printf("\n\n\n****** With DEBUG = 0, Inside tx_pkt_v3() and entering free_pkt(pkt) ******\n\n\n");	

	// Increment the wr_ptr using "wr_fifo_ptr_tx_pkt_inc_reg" desc_table_tx entry
	// __atomic_store_n((unsigned*)&(_net_csrs->csr5_tx1), NET_TX_FIFO_WR_PTR_INC, memorder);

	free_pkt(pkt);

	if (DEBUG)
		printf("\n\n\n****** Inside tx_pkt_v3() and exited free_pkt(pkt) ******\n\n\n");



	// }

#else

	free_pkt(pkt);

#endif

}


NET_PACKET	*new_pkt(unsigned msglen) {   // for ARP msglen = 36, and p_user =+ 8  // msglen = 40, 32 + 8 from udp packt = 40
	NET_PACKET	*pkt;
	unsigned	pktlen;

	pkt = (NET_PACKET *)malloc(sizeof(NET_PACKET) + msglen);  
	// for pkt size msglen is being added 
	// just allocating this much mem (sizeof(NET_PACKET) + msglen) for the pkt struct pointer, this
	// is where msglen comes into use 


	pkt->p_usage_count = 1;  // Imp I missed this!  // used in freeing the packet

	//p_raw is a char* ptr 
	pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);  // pointer arithematic happening here
		// ONLY GOING FORWARD the size of NET_PACKET... not moving forward NET_PACKET + msglen ... 
			// now we can write msglen bytes after NET_PACKET sized bytes..
		// giving the pointer its address value!!!!! // Casting pkt pointer address as char* byte pointer and adding to it the sizeof NET_PACKET (in bytes because we can add it in bytes to char ptr) meaning praw will point to end of struct which actually is *p_user, where our packet memory is 
		// we have the ptr the pkt. We are adding to the the size of Net Packet
		// so p_raw is like the the end of the pointer without considering msglen

	//p_raw is a char* ptr 
	//*p_user is a char* pointer
	pkt->p_user= pkt->p_raw;   // giving the pointer its address value!!!!! // giving the address of where p_user would begin // start after memloc of pkt normally finished (something like that)
	// so p_user is increased as this function returns the pkt to its parent function, I think its ethernetpkt, which further forwards the pkt to 
	// another parent function which further increases the p_user. I think p_user is increased by 28, since after that we add 8 bytes of ICMP. So we are making the packet top down.



	pkt->p_rawlen = msglen;		// 36  // will be 36 + 6 = 42 when I add the src mac bytes 
	pkt->p_length = pkt->p_rawlen;	// // // plength is 36 now?  yes :3  // will be 36 + 6 = 42 when I add the src mac bytes 

	return pkt;
}

void	free_pkt(NET_PACKET *pkt) {
	
	if (NULL == pkt) {
		return;
	}

	if (pkt->p_usage_count > 0) {
		pkt->p_usage_count --;  // usage was 1 for txpkt :3
			// for rx pkt the pkt usage is automatically reset to 1...
	}

	// VIP !! Since this function is executed line by line.. so after usage is subtraced once (when it was 1 initially for rxpkt),
	// the p_usage goes to 0 and the class object is freed as per the if condition below 

	    // p_usage is 0 after this subtraction
	if (pkt->p_usage_count == 0) {  // so after tx packet has been sent its usage (which was 1) has been subtracted in the line above
		// and now the pusage is = 0, so now that packet i.e., the pointer which is taking up memory for that packet will be deleted 
		// along with that memory it was taking so now our processor will have memory to work with

		free(pkt); // entered here https://www.tutorialspoint.com/c_standard_library/c_function_free.htm
		//The C library function void free(void *ptr) deallocates the memory previously allocated by a call to calloc, malloc, or realloc.
	} else {

		if (SIMULATION_TESTING) {

			// error code (led = 5 means some error) for flagging that p_usage wasn't reduced to 0 even after free_pkt()
			write_led(0);
			write_led(5);
			write_led(0);
			write_led(5);
			write_led(11);
			write_led(0);

		}

	}
}

// #include <stdio.h>
#include <stdint.h>

void	dump_raw(NET_PACKET *pkt) {
	unsigned	posn = 0;

	//printf("RAW-PACKET DUMP -----\n");
	// printf("RAW-PACKET DUMP -----\n");
	posn = 4;
	if ((pkt->p_user != pkt->p_raw)
		// && (pkt->p_user > pkt->p_raw)
		// && (pkt->p_raw + pkt->p_rawlen > pkt->p_user)
		) {
		// printf("(RAW)  "); posn = 7;
		for(unsigned k=0; k < pkt->p_user - pkt->p_raw; k++) {
			// printf("%02x ", pkt->p_raw[k]); posn +=4;
			if ((k&7) == 7) {
				posn = 7; //printf("\n%*s", posn, "");
			}
		} //printf("\n(USER) ");
		for(unsigned k=7; k<posn; k++)
			;// printf(" ");
	} else if (pkt->p_user == pkt->p_raw) {
		;//printf("(RAW)  ");
	} else
		//printf("(USER) ");

	for(unsigned k=0; k < pkt->p_length; k++) {
		//printf("%02x ", pkt->p_user[k]); posn +=3;
		if ((k&7) == 7) {
			posn = 7; //printf("\n%*s", posn, "");
		}
	} //printf("\n");
}

//memcpy((char *)_netbtx, pkt->p_user, pkt->p_length); 
void * MemcpySW(void* dst, const void* src, unsigned int cnt)
{
    char *pszDest = (char *)dst;
    const char *pszSource =( const char*)src;
    if((pszDest!= NULL) && (pszSource!= NULL))
    {
        while(cnt) //till cnt
        {
            //Copy byte by byte
            //*(pszDest++)= *(pszSource++);  
            // Code from ethernet RTL below:
            // assign write_to_tx_mem = (write_flag) && (modified_reduced_axi_awaddr[MAW+1:MAW] == 2'b11);
			// if (axi_wstrb_buff[3])  
			// 	txmem[modified_reduced_axi_awaddr[(MAW-1):0]][7:0] <= axi_wdata_buff[31:24];  // addr len is (MAW-1) i.e. 8:0, 9 bit words, 512 words = 2048 bytes which is greater than max packet len
			// if (axi_wstrb_buff[2])
			// 	txmem[modified_reduced_axi_awaddr[(MAW-1):0]][15:8] <= axi_wdata_buff[23:16];
			// if (axi_wstrb_buff[1])
			// 	txmem[modified_reduced_axi_awaddr[(MAW-1):0]][23:16] <= axi_wdata_buff[15:8];
			// if (axi_wstrb_buff[0])
			// 	txmem[modified_reduced_axi_awaddr[(MAW-1):0]][31:24] <= axi_wdata_buff[7:0];
 		       	// atomic store at _netbtx is storing the src ptr data here to these addresses as we are doing pszDest++
        		// reversed ENDIANs btw but that is an rtl issue
            __atomic_store_n((char*)(pszDest++), *(pszSource++), memorder);  //p++ returns p then increments it for the next iter
            // https://stackoverflow.com/questions/8208021/how-to-increment-a-pointer-address-and-pointers-value
            --cnt;
        }
    }
    return dst;
}

//memcpy(pkt->p_raw, (char *)_netbrx, pkt->p_rawlen+2);
void * MemcpyLW(void* dst, const void* src, unsigned int cnt)
{
    char *pszDest = (char *)dst;
    const char *pszSource =( const char*)src;
    if((pszDest!= NULL) && (pszSource!= NULL))
    {
        while(cnt) //till cnt
        {
            //Copy byte by byte
            //*(pszDest++)= *(pszSource++);
            //__atomic_store_n((char*)(pszDest++), *(pszSource++), memorder);  //p++ returns p then increments it for the next iter
            *(pszDest++) = __atomic_load_n((char*)(pszSource++), memorder);
            --cnt;
        }
    }
    return dst;
}

void* AlignToFourByteMemAddress(void *input_pointer) {

	// if the input pointer is not a 4 byte aligned, round it UP.
	if ((unsigned)input_pointer % 4 != 0) {

		// input_pointer is a copy of the input argument void *input_pointer
		input_pointer = (void*)((unsigned)input_pointer +  (4 - ((unsigned)input_pointer % 4)));

	}

	// return updated or not updated but definitely 4 byte aligned value of the input pointer
	return input_pointer;

}

// this function will add to available memory once the current rd_ptr indicates that the memory has been consumed by the tx_pkt_DMA
// void	update_tx_pkt_avail_memory(unsigned &tx_pkt_prev_rd_ptr, unsigned &tx_pkt_curr_rd_ptr, unsigned &tx_pkt_current_available_memory, TX_PKTS_DESC_TABLE_TX_STATUS &tx_pkts_status_desc_table_tx_obj) {
	// https://stackoverflow.com/questions/17168623/does-c-even-have-pass-by-reference
	// so C does not have pass by reference.. thats why so many functions use pointers as arguments 
void	update_tx_pkt_avail_memory(unsigned *tx_pkt_prev_rd_ptr, unsigned *tx_pkt_curr_rd_ptr, unsigned *tx_pkt_current_available_memory, TX_PKTS_DESC_TABLE_TX_STATUS *tx_pkts_status_desc_table_tx_obj) {

	// using -> with tx_pkts_status_desc_table_tx_obj when accessing its variable when we are accessing its pointer to object, 
	// using .  with tx_pkts_status_desc_table_tx_obj when we are accessing its object
		// both dereferece operators "->" and "*" work
			// z1_p->x[2] = 25;    
			// (*z1_p).x[3] = 100;

	unsigned counter = 0;
	unsigned desc_table_tx_depth = DESC_TABLE_TX_DEPTH;

	if (SIMULATION_TESTING) {

		// led = 20 means update_tx_pkt_avail_memory() running
		write_led(20); 
		write_led(*tx_pkt_curr_rd_ptr); 
		write_led(*tx_pkt_prev_rd_ptr); 

	}

	// there is a race condition between prev_rd_ptr and curr_rd_ptr wrt to wr_ptr which has been taken care of in tx_pkt()

	// if curr_rd_ptr has been incremented by tx_pkt_dma 
	if ((*tx_pkt_prev_rd_ptr) !=  (*tx_pkt_curr_rd_ptr)) {

		// if curr_rd_ptr is > than prev_rd_ptr in the range from 0 - (desc_table_tx_depth-1), means that curr_rd_ptr has not rolled over (desc_table_tx_depth - 1)
		if ((*tx_pkt_curr_rd_ptr) > (*tx_pkt_prev_rd_ptr)) {

			if ((*tx_pkt_curr_rd_ptr) < desc_table_tx_depth) {
				
				// increment prev_rd_ptr till (curr_rd_ptr - 1), since the curr_rd_ptr is presently transmitting the tx_pkt in the tx_pkt_dma
				for (unsigned i = (*tx_pkt_prev_rd_ptr); i < (*tx_pkt_curr_rd_ptr); i = i + 1) {

					// parse through tx_pkts_status_desc_table_tx_obj that was updated by the wr_ptr, now update it by rd_ptr and add back the 
					// memory consumed by that tx_pkt to "tx_pkt_current_available_memory"

					// this should be 1 since
					if (tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i] != 1) {

						// print error msg and turn on warning led
		   				if ((DEBUG) || (SIMULATION_TESTING)) {


		   					if (SIMULATION_TESTING && DEBUG) {

		   						// printing error message
		   						printf("There is an error in update_tx_pkt_avail_memory() in main()");

		   						// led = 5 means there is an error
		   						write_led(5);

		   						// writing led6 after led5 is just to locatlize the bug
		   						write_led(6);

		   						// wait 2 seconds
		   						// sleep(2000000); 
		   							// no sleep when SIM & DEBUG 

		   					} else if (SIMULATION_TESTING) {

		   						// led = 5 means there is an error
		   						write_led(5); 

		   						// writing led6 after led5 is just to locatlize the bug
		   						write_led(6);
		   						write_led(tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i]);


		   					} else if (DEBUG) {

		   						// printing error message
		   						printf("\n\nThere is an error in update_tx_pkt_avail_memory in main() with relevant leds ON (5 and 6)\n\n");

		   						// led = 5 means there is an error
		   						write_led(5); 

		   						// delay introduced for testing on hardware

		   						// wait 2 seconds
		   						// sleep(2000000);

		   						// writing led6 after led5 is just to locatlize the bug
		   						write_led(6);

		   						// wait 2 seconds
		   						// sleep(2000000);

		   					}

		   				}


					}

					if (SIMULATION_TESTING) {
						write_led(21); 
						write_led(i); 
						write_led(tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i]);
					} 

					// overwriting by 2 means this bin was updated after rd_ptr had gone past this bin's index 
					tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i] = 2;

					if (SIMULATION_TESTING) {
						write_led(tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i]);
					}

					// update the tx_pkt_current_available_memory
					(*tx_pkt_current_available_memory) = (*tx_pkt_current_available_memory) + tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[i];

					// if we are dubugging on hardware or doing simulation, then we update the debug csr register
					if (DEBUG || SIMULATION_TESTING) {

						// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
						__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), (*tx_pkt_current_available_memory), memorder);
					} 

					if (SIMULATION_TESTING) {
						write_led(22); 
						write_led(i);
					}
					
					counter = i;
					
					if (SIMULATION_TESTING) {
						write_led(counter);
					}
				}

				if (SIMULATION_TESTING) {
					write_led(23); 
					write_led(counter);
					write_led(*tx_pkt_prev_rd_ptr);
				}

				// update the value of prev_rd_ptr to the value tx_pkt_curr_rd_ptr - 1
				// *tx_pkt_prev_rd_ptr = counter;
					// error since we need to count TILL "tx_pkt_curr_rd_ptr" not tx_pkt_curr_rd_ptr - 1,
					// otherwise we will be recounting the tx_pkt_curr_rd_ptr - 1 values again and again
					// e.g, we keep recounting when tx_pkt_prev_rd_ptr = 0 and tx_pkt_curr_rd_ptr = 1 

				*tx_pkt_prev_rd_ptr = *tx_pkt_curr_rd_ptr;

				if (SIMULATION_TESTING) {
					write_led(*tx_pkt_prev_rd_ptr);
				}
			}
			// else if (tx_pkt_curr_rd_ptr >= desc_table_tx_depth)
			else {


				// print error msg and turn on warning led
   				if ((DEBUG) || (SIMULATION_TESTING)) {


   					if (SIMULATION_TESTING && DEBUG) {

   						// printing error message
   						printf("\nThere is an error in update_tx_pkt_avail_memory() in main() with relevant leds ON (5 and 7)\n");

   						// led = 5 means there is an error
   						write_led(5);

   						// writing led7 after led5 is just to locatlize the bug
   						write_led(7);

   						// wait 2 seconds
   						// sleep(2000000); 
   							// no sleep when SIM & DEBUG 

   					} else if (SIMULATION_TESTING) {

   						// led = 5 means there is an error
   						write_led(5); 

   						// writing led7 after led5 is just to locatlize the bug
   						write_led(7);


   					} else if (DEBUG) {

   						// printing error message
   						printf("\nThere is an error in update_tx_pkt_avail_memory in main() with relevant leds ON (5 and 7)\n");
						printf("\n*tx_pkt_curr_rd_ptr = %d\n", *tx_pkt_curr_rd_ptr);
						printf("\ndesc_table_tx_depth = %d\n", desc_table_tx_depth);
   						

   						// led = 5 means there is an error
   						write_led(5); 

   						// delay introduced for testing on hardware

   						// wait 2 seconds
   						// sleep(2000000);

   						// writing led7 after led5 is just to locatlize the bug
   						write_led(7);

   						// wait 2 seconds
   						// sleep(2000000);

   					}

   				}

			}
		
		}
		// else if tx_pkt_curr_rd_ptr < tx_pkt_prev_rd_ptr
		else {

			// first we will parse prev_rd_ptr till (desc_table_tx_depth - 1)
			// then we will parse from 0 till (curr_rd_ptr - 1)

			for (unsigned i = (*tx_pkt_prev_rd_ptr); i < desc_table_tx_depth; i = i + 1) {

				// this should be 1 since
				if (tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i] != 1) {

					// print error msg and turn on warning led
	   				if ((DEBUG) || (SIMULATION_TESTING)) {


	   					if (SIMULATION_TESTING && DEBUG) {

	   						// printing error message
	   						printf("There is an error in update_tx_pkt_avail_memory() in main()");

	   						// led = 5 means there is an error
	   						write_led(5);

	   						// writing led6 after led5 is just to locatlize the bug
	   						write_led(2);

	   						// wait 2 seconds
	   						// sleep(2000000); 
	   							// no sleep when SIM & DEBUG 

	   					} else if (SIMULATION_TESTING) {

	   						// led = 5 means there is an error
	   						write_led(5); 

	   						// writing led6 after led5 is just to locatlize the bug
	   						write_led(2);


	   					} else if (DEBUG) {

	   						// printing error message
	   						printf("\nThere is an error in update_tx_pkt_avail_memory in main( with relevant leds ON (5 and 2))\n");

	   						// led = 5 means there is an error
	   						write_led(5); 

	   						// delay introduced for testing on hardware

	   						// wait 2 seconds
	   						// sleep(2000000);

	   						// writing led6 after led5 is just to locatlize the bug
	   						write_led(2);

	   						// wait 2 seconds
	   						// sleep(2000000);

	   					}

	   				}


				}

				// overwriting by 2 means this bin was updated after rd_ptr had gone past this bin's index 
				tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i] = 2;

				// update the tx_pkt_current_available_memory
				*tx_pkt_current_available_memory = (*tx_pkt_current_available_memory) + tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[i];

				// if we are dubugging on hardware or doing simulation, then we update the debug csr register
				if (DEBUG || SIMULATION_TESTING) {

					// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
					__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), *tx_pkt_current_available_memory, memorder);
				} 

				counter = i;

			}

			// update the value of prev_rd_ptr
			*tx_pkt_prev_rd_ptr = counter;

			// Now we will parse from 0 till (curr_rd_ptr - 1)
			for (unsigned i = 0; i < *tx_pkt_curr_rd_ptr; i = i + 1) {

				// this should be 1 since
				if (tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i] != 1) {

					// print error msg and turn on warning led
	   				if ((DEBUG) || (SIMULATION_TESTING)) {


	   					if (SIMULATION_TESTING && DEBUG) {

	   						// printing error message
	   						printf("There is an error in update_tx_pkt_avail_memory() in main()");

	   						// led = 5 means there is an error
	   						write_led(5);

	   						// writing led6 after led5 is just to locatlize the bug
	   						write_led(3);

	   						// wait 2 seconds
	   						// sleep(2000000); 
	   							// no sleep when SIM & DEBUG 

	   					} else if (SIMULATION_TESTING) {

	   						// led = 5 means there is an error
	   						write_led(5); 

	   						// writing led6 after led5 is just to locatlize the bug
	   						write_led(3);


	   					} else if (DEBUG) {

	   						// printing error message
	   						printf("\nThere is an error in update_tx_pkt_avail_memory in main() with relevant leds ON (5 and 3)\n");

	   						// led = 5 means there is an error
	   						write_led(5); 

	   						// delay introduced for testing on hardware

	   						// wait 2 seconds
	   						// sleep(2000000);

	   						// writing led6 after led5 is just to locatlize the bug
	   						write_led(3);

	   						// wait 2 seconds
	   						// sleep(2000000);

	   					}

	   				}


				}

				// overwriting by 2 means this bin was updated after rd_ptr had gone past this bin's index 
				tx_pkts_status_desc_table_tx_obj->tx_pkt_active[i] = 2;

				// update the tx_pkt_current_available_memory
				*tx_pkt_current_available_memory = (*tx_pkt_current_available_memory) + tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[i];

				// if we are dubugging on hardware or doing simulation, then we update the debug csr register
				if (DEBUG || SIMULATION_TESTING) {

					// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
					__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), *tx_pkt_current_available_memory, memorder);
				} 

				counter = i;


			}

			// update the value of prev_rd_ptr
			// *tx_pkt_prev_rd_ptr = counter;
				// not correct as per the comments in the previous exact assignment

			*tx_pkt_prev_rd_ptr = *tx_pkt_curr_rd_ptr;


		}

	}
	// if curr_rd_ptr hasn't changed at all
	else {

		if (SIMULATION_TESTING) {

			write_led(8);

		}

		if (DEBUG) {

			printf("\ntx_pkt_prev_rd_ptr ==  tx_pkt_curr_rd_ptr in update_tx_pkt_avail_memory() \n");
		}

	}

}

int     tx_pkt_memory_availability_calculation(NET_PACKET *txpkt, unsigned *tx_pkt_curr_wr_ptr, unsigned *tx_pkt_prev_wr_ptr, unsigned *tx_pkt_curr_rd_ptr, 
												unsigned *tx_pkt_prev_rd_ptr, unsigned *tx_pkt_current_available_memory, TX_PKTS_DESC_TABLE_TX_STATUS *tx_pkts_status_desc_table_tx_obj,
												unsigned tx_pkt_upper_limit_mem_addr, unsigned tx_pkt_start_mem_addr) {

	// return 1 for enough space to send a txpkt
	// return 0 for NOT enough space to send a txpkt
	// return -1 for an error condition

	// changing all variables as follows:
		// tx_pkt_curr_wr_ptr => *tx_pkt_curr_wr_ptr
		// tx_pkt_prev_wr_ptr => *tx_pkt_prev_wr_ptr
		// tx_pkt_curr_rd_ptr => *tx_pkt_curr_rd_ptr
		// &tx_pkt_curr_rd_ptr => tx_pkt_curr_rd_ptr
		// tx_pkt_prev_rd_ptr => *tx_pkt_prev_rd_ptr
		// &tx_pkt_prev_rd_ptr => tx_pkt_prev_rd_ptr
		// tx_pkt_current_available_memory => *tx_pkt_current_available_memory
		// &tx_pkt_current_available_memory => tx_pkt_current_available_memory
		// tx_pkts_status_desc_table_tx_obj. => tx_pkts_status_desc_table_tx_obj->
		// &tx_pkts_status_desc_table_tx_obj => tx_pkts_status_desc_table_tx_obj
			// using -> with tx_pkts_status_desc_table_tx_obj when accessing its variable when we are accessing its pointer to object, 
			// using .  with tx_pkts_status_desc_table_tx_obj when we are accessing its object
				// both dereferece operators "->" and "*" work
					// z1_p->x[2] = 25;    
					// (*z1_p).x[3] = 100;

	void *tx_pkt_curr_calculated_start_mem_addr = NULL;
    
    void *tx_pkt_calculated_start_mem_addr_curr_rd_ptr = NULL;
    void *tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta = NULL;
   	void *tx_pkt_calculated_start_mem_addr_prev_rd_ptr = NULL;
    void *tx_pkt_calculated_start_mem_addr_prev_rd_ptr_delta = NULL;
    

    void *tx_pkt_calculated_start_mem_addr_curr_wr_ptr_delta = NULL;
    void *tx_pkt_calculated_start_mem_addr_curr_wr_ptr = NULL;
    void *tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta = NULL;
    void *tx_pkt_calculated_start_mem_addr_prev_wr_ptr = NULL;

    // csr for csr_tx1
	unsigned	net_csr5_tx1;
	unsigned	net_csr5_tx1_FULL, net_csr5_tx1_EMPTY;

	// indicating start of while loop of  tx of tx_pkt
	if (SIMULATION_TESTING) {
		write_led(2);

		write_led(12); 
		write_led(*tx_pkt_curr_wr_ptr);
		write_led(*tx_pkt_curr_rd_ptr);
		write_led(0);
	}

	// check if desc_table_tx is not full

	if (DEBUG) {

		printf("\nbefore reading csr *tx_pkt_curr_wr_ptr = %d\n", *tx_pkt_curr_wr_ptr);
		printf("\nbefore reading csr *tx_pkt_curr_rd_ptr = %d\n", *tx_pkt_curr_rd_ptr);
		
		printf("\nDirectly printing __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder) = %d\n",  __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder));
		printf("\nDirectly printing __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder) = %d\n",  __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder));

	}


	// reading desc_table_tx wr_ptr from network subsystem 
	*tx_pkt_curr_wr_ptr = __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder);
	*tx_pkt_curr_rd_ptr = __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder);
		// reading the rd_ptr before reading empty conditions cx we are removing all rd_ptr reads 
		// in the if conditions below since rd_ptr keeps getting changed by tx_pkt_dma
		// so we will read the rd_ptr once at the start of while loop and will remove 
		// further rd_ptr reads in the if conditions below along with removing all
		// while loops in the if conditions below that kept on reading the rd_ptr
		// cx the problem with that was that if according to the value of rd_ptr
		// the desc_table_tx was not empty and our code entered that if condition
		// and after entering that non-empty if condition, the desc_table_tx got
		// empty and we kept on reading the rd_ptr, that rd_ptr would have been incremented
		// to the value == wr_ptr (desc_table_tx empty cond) and in that non-empty if condition
		// we would now be assuming that the desc_table_tx is not empty and the rd_ptr is pointing
		// to a memory address that was written to by the wr_ptr in this non-empty state, but
		// actually the rd_ptr would be pointing to the memory address that was written to one rollover
		// before, by the wr_ptr cx the current wr_ptr hasn't filled in this value of mem address of 
		// the tx_pkts_status_desc_table_tx_obj, cx wr_ptr == rd_ptr in the empty desc_table_tx state
		// which we are assuming to be non-empty..

		// in order to deal with the problem above.. we will first read the rd_ptr once at the start of this while loop
		// and thenn read the empty condition csr.. if not empty we will continue with this read rd_ptr value, in the worst case
		// scenario this rd_ptr has been updated by tx_pkt_dma in real time in the network subsystem and we are now using the older
		// value of rd_ptr, but there is no problem in that since this older rd_ptr will be showing us that more memory is being used
		// than it actually is, which is okay cx we will just have to wait a bit longer for the rd_ptr to get updated and the mem to
		// be freed.. Hence we are removing all the while loops that are updating the rd_ptr in the if conditions below.. cx they
		// will lead to an empty state being considered as a non-empty state cx of being stuck in a while loop in the non-empty if condition..

	if (DEBUG) {

		printf("\nafter reading csr *tx_pkt_curr_wr_ptr = %d\n", *tx_pkt_curr_wr_ptr);
		printf("\nafter reading csr *tx_pkt_curr_rd_ptr = %d\n", *tx_pkt_curr_rd_ptr);

		printf("\nDirectly printing __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder) = %d\n",  __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder));
		printf("\nDirectly printing __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder) = %d\n",  __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder));
	}

	if (SIMULATION_TESTING) {
		write_led(3);

		write_led(13); 
		write_led(*tx_pkt_curr_wr_ptr);
		write_led(*tx_pkt_curr_rd_ptr);
		write_led(0);
	}


	// loading csr5 register from network subsystem
	net_csr5_tx1 = __atomic_load_n((unsigned*)&(_net_csrs->csr5_tx1), memorder);

	// first check if the desc_table_tx is full or empty
	net_csr5_tx1_FULL = NET_TX_PKT_DESC_TABLE_TX_FULL(net_csr5_tx1);
	net_csr5_tx1_EMPTY = NET_TX_PKT_DESC_TABLE_TX_EMPTY(net_csr5_tx1);



   	// if desc_table_tx is not full we will send another tx packet butttttt first
   	// we will check if we have enough memory available in the memory allocated for tx_pkts
   	// in DDR by riscv
   	if (!net_csr5_tx1_FULL) {

   		if (SIMULATION_TESTING) {
   			write_led(15);
   		}

   		if (DEBUG) {

			printf("\nEntered !net_csr5_tx1_FULL\n");
		}

   		// now checking if total avaialble memory currently for tx_pkts is enough to write a 3 tx_pkts in memory
   			// 1 MAX_TX_PKT_LEN for the upper limit and 2 MAX_TX_PKT_LEN for disatnce between rd_ptr and wr_ptr
   			// +4 for aligning mem address with 4 bytes

   		// // reading desc_table_tx wr_ptr from network subsystem 
   		// tx_pkt_curr_wr_ptr = __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder);
   		// tx_pkt_curr_rd_ptr = __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder);

   		// here we will recalculate available memory based on previous rd_ptr and current rd_ptr
   			// removing the &s since we are passing pointers directly now 
   		update_tx_pkt_avail_memory(tx_pkt_prev_rd_ptr, tx_pkt_curr_rd_ptr, tx_pkt_current_available_memory, tx_pkts_status_desc_table_tx_obj);
   		// update_tx_pkt_avail_memory(&tx_pkt_prev_rd_ptr, &tx_pkt_curr_rd_ptr, &tx_pkt_current_available_memory, &tx_pkts_status_desc_table_tx_obj);

   		// this condition is to take care of the race condition between prev_rd_ptr and curr_rd_ptr
   		// for some reason the wr_ptr and curr_rd_ptr are following eachother but prev_rd_ptr is stuck at 0,
   		// then the wr_ptr will lets say overwrite the idx = 0 and after curr_rd_ptr follows the wr_ptr to idx = 0,
   		// the prev_rd_ptr will == curr_rd_ptr and it will be assumed that the curr_rd_ptr didn't move.
   			// so to deal with this we will check "tx_pkt_active" bin of tx_pkts_status_desc_table_tx_obj at wr_ptr 
   			// and if its value is "1", it means that the wr_ptr has gone over this idx once before it its memory consumption
   			// hasn't been added back to the total avail memory since its value would have been == "2" if "update_tx_pkt_avail_memory()"
   			// function had run and the prev and curr rd_ptrs had ran over it.  
   		// if (tx_pkts_status_desc_table_tx_obj.tx_pkt_active[tx_pkt_curr_wr_ptr] == 1) {

   		// 	while (1) {

   		// 		// led = 4 means there is a warning
   		// 		write_led(4);

   		// 		update_tx_pkt_avail_memory(&tx_pkt_prev_rd_ptr, &tx_pkt_curr_rd_ptr, &tx_pkt_current_available_memory, &tx_pkts_status_desc_table_tx_obj);

   		// 		write_led(0);

   		// 		if (tx_pkts_status_desc_table_tx_obj.tx_pkt_active[tx_pkt_curr_wr_ptr] != 1) {

   		// 			break;
   		// 		}

   		// 	}

   		// }

   		// instead of running a while loop as in the commented out code above, we will use the outer most while loop for that and remove any more inner while loops
   		// if (tx_pkts_status_desc_table_tx_obj.tx_pkt_active[*tx_pkt_curr_wr_ptr] != 1) {
   		if (tx_pkts_status_desc_table_tx_obj->tx_pkt_active[*tx_pkt_curr_wr_ptr] != 1) {

   			if (DEBUG)
				printf("\nEntered if (tx_pkts_status_desc_table_tx_obj->tx_pkt_active[*tx_pkt_curr_wr_ptr] != 1)\n");


	   		// if there is enough mem available then we will move forward with transmission of tx_pkt by writing it in mem first
	   		if (*tx_pkt_current_available_memory > (3*(MAX_ETH_PKT_LEN + 4))) {

	   			if (DEBUG)
					printf("\nEntered if (*tx_pkt_current_available_memory > (3*(MAX_ETH_PKT_LEN + 4))) with *tx_pkt_current_available_memory = %d\n", *tx_pkt_current_available_memory);

	   			if (SIMULATION_TESTING)
	   				write_led(14);	   			
	   			

	   			if ((*tx_pkt_curr_wr_ptr >= DESC_TABLE_TX_DEPTH) || (*tx_pkt_curr_wr_ptr < 0) || 
	   				(*tx_pkt_curr_rd_ptr >= DESC_TABLE_TX_DEPTH) || (*tx_pkt_curr_rd_ptr < 0)) {

	   				// if desc_table_tx wr_ptr is out of range then throw and error and exit the main function or start
	   				// a while(1) loop printing out the error
	   				
	   				// no need of a while(1) loop.. I just need a blip of LED5 going HIGH that wil tell me there was an error somewhere, which can be debugged in detail to LOCALIZE it
	   				// while (1) {

	   				// print error msg and turn on warning led
	   				if ((DEBUG) || (SIMULATION_TESTING)) {


	   					if (SIMULATION_TESTING && DEBUG) {

	   						// printing error message
	   						printf("There is an error in tx_pkt_curr_wr_ptr || tx_pkt_curr_rd_ptr in main()");

	   						// led = 5 means there is an error
	   						write_led(5);

	   						// writing led1 after led5 is just to locatlize the bug
	   						write_led(1);

	   						// wait 2 seconds
	   						// sleep(2000000); 
	   							// no sleep when SIM & DEBUG 

	   					} else if (SIMULATION_TESTING) {

	   						// led = 5 means there is an error
	   						write_led(5); 

	   						// writing led1 after led5 is just to locatlize the bug
	   						write_led(1);


	   					} else if (DEBUG) {

	   						// printing error message
	   						printf("There is an error in tx_pkt_curr_wr_ptr || tx_pkt_curr_rd_ptr in main() waiting 4 seconds with led = 5 and 1");
	   						printf("\n*tx_pkt_curr_wr_ptr = %d\n", *tx_pkt_curr_wr_ptr);
	   						printf("\n*tx_pkt_curr_rd_ptr = %d\n", *tx_pkt_curr_rd_ptr);
	   						printf("\nDESC_TABLE_TX_DEPTH = %d\n", DESC_TABLE_TX_DEPTH);
	   						printf("\nDirectly printing __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder) = %d\n",  __atomic_load_n((unsigned*)&(_net_csrs->csr7_tx1), memorder));
	   						printf("\nDirectly printing __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder) = %d\n",  __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder));

	   						// led = 5 means there is an error
	   						write_led(5); 

	   						// delay introduced for testing on hardware

	   						// wait 2 seconds
	   						// sleep(2000000);

	   						// writing led1 after led5 is just to locatlize the bug
	   						write_led(1);

	   						// wait 2 seconds
	   						// sleep(2000000);

	   					}

	   				}

		   			// }

	   				// break from the while loop of the tx_pkt 
		   			// break;

		   			// not enough space to send txpkt or an error condition
		   			return -1;
	 
	   			
	   			} 
	   			else {


	   				if (DEBUG)
						printf("\nEntered else of this => if ((*tx_pkt_curr_wr_ptr >= DESC_TABLE_TX_DEPTH) || (*tx_pkt_curr_wr_ptr < 0) ||\n");

	   				if (SIMULATION_TESTING)
	   					write_led(13);

	   				// now we need to check if we have enough memory avaible to actually write the tx_pkt in it

	   				// so actually I don't think rd_ptr or wr_ptr being ahead of each other matter here ..
	   				// cx the rd_ptr can be ahead of wr_ptr but in mem address space the wr_ptr can still have 
	   				// mem address that is ahead of rd_ptr, the example would be: lets say we have given
	   				// total memory of 32KB for tx_pkts, while our desc_table_tx_depth = 4, and we have written 
	   				// 5 tx_pkts of 1024 bytes each and the value of rd_ptr currently is rd_ptr = 2 ad wr_ptr = 1,
	   				// so in this case even though the rd_ptr is infront of the wr_ptr but the mem address of tx_pkt for 
	   				// wr_ptr would be in front of mem address of tx_pkt for rd_ptr since we have 32KB of mem of tx_pkts..
	   				// in this case for rd_ptr = 2:
	   					// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[rd_ptr] = tx_pkt_start_mem_addr + (2 x 1024 byte tx_pkt)
	   						// tx_pkt_curr_calculated_start_mem_addr = tx_pkt_start_mem_addr + (2 x 1024 byte tx_pkt)
	   				// where as in this case for wr_ptr = 1 (rolled over):
	   					// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[wr_ptr] = tx_pkt_start_mem_addr + (5 x 1024 byte tx_pkt)
	   						// tx_pkt_curr_calculated_start_mem_addr = tx_pkt_start_mem_addr + (5 x 1024 byte tx_pkt)

	   				// This is what I meant that the "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr would be ahead of 
	   				// "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr, even if the rd_ptr is itself ahead of wr_ptr..

	   				// HENCE, we should be looking at "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr and wr_ptr
	   				// and check if the ""tx_pkt_curr_calculated_start_mem_addr" for rd_ptr is ahead of wr_ptr and vice versa.

	   				// 1) For the case when "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr is ahead of "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr and
	   				// if ""tx_pkt_curr_calculated_start_mem_addr"" for wr_ptr is within MAX_TX_PKT_LEN of "tx_pkt_upper_limit_mem_addr"
	   				// so if "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr is within MAX_TX_PKT_LEN of "tx_pkt_upper_limit_mem_addr",
	   				// then we will rollover the "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr back to "tx_pkt_start_mem_addr"
	   				// PROVIDED that we keep on checking rd_ptr and see "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr is subtracted by 
	   				// "tx_pkt_start_mem_addr" and if the difference is greater than MAX_TX_PKT_LEN then 
	   				// "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr is rolled back to "tx_pkt_start_mem_addr"

	   				// 2) For the case when "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr is ahead of "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr
	   				// We will keep checking that the difference between "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr from 
	   				// "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr is at least MAX_TX_PKT_LEN and as soon as it is, then the tx_pkt is allotted that
	   				// address "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr and the tx_pkt is written to memory and made available in the desc_table_tx
	   					// UPDATE: 
		   					// keep checking if there is at least 2 MAX_ETH_PKT_LEN space infront of tx_pkt_calculated_start_mem_addr_prev_wr_ptr 
	   						// that is not occupied by tx_pkt_calculated_start_mem_addr_curr_rd_ptr
		   					// cx the prev tx_pkt need 1 MAX_TX_PKT_LEN and the curr tx_pkt needs 1 MAX_TX_PKT_LEN

	   				// 3) For the case when desc_table_tx is EMPTY, we will use the condition #1, without checking rd_ptr and see 
	   				// "tx_pkt_curr_calculated_start_mem_addr" for rd_ptr is subtracted by 
	   				// "tx_pkt_start_mem_addr" and if the difference is greater than MAX_TX_PKT_LEN then 
	   				// "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr is rolled back to "tx_pkt_start_mem_addr".
	   				// We will check if "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr is within MAX_TX_PKT_LEN
	   				// of "tx_pkt_upper_limit_mem_addr" just directly alot "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr the value "tx_pkt_start_mem_addr",
	   				// if it is not withint MAX_TX_PKT_LEN of "tx_pkt_upper_limit_mem_addr", then will will alot "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr
	   				// to the wr_ptr entry of tx_pkts_status_desc_table_tx_obj

	   				// Now we'll make separate varibles of "tx_pkt_curr_calculated_start_mem_addr" for wr_ptr and rd_ptr

	   				// using prev_wr_ptr "tx_pkt_prev_wr_ptr" to check what was the starting memory address of the last tx_pkt 
	   				tx_pkt_calculated_start_mem_addr_prev_wr_ptr = tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_prev_wr_ptr];
	   				if (DEBUG || SIMULATION_TESTING) {
	   					// update the csr13_tx1 is for writing "tx_pkt_calculated_start_mem_addr_prev_wr_ptr" to network subsystem for debugging pruposes
						__atomic_store_n((unsigned*)&(_net_csrs->csr13_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr, memorder);
	   				}

	   				// calculating curr and prev rd_ptr mem address for tx_pkt
	   				// tx_pkt_calculated_start_mem_addr_prev_rd_ptr = tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_prev_rd_ptr];
	   				tx_pkt_calculated_start_mem_addr_curr_rd_ptr = tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_rd_ptr];
	   					// TODO: bug here.. this goes to 0 after tx of first tx_pkt
	   						// need to initialize all tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address entries to "tx_pkt_start_mem_addr" 
	   				if (DEBUG || SIMULATION_TESTING) {
	   					// update the csr14_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_rd_ptr" to network subsystem for debugging pruposes
						__atomic_store_n((unsigned*)&(_net_csrs->csr14_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr, memorder);
	   				}

	   				if (DEBUG || SIMULATION_TESTING) {
	   					// update the csr15_tx1 is for writing "tx_pkt_upper_limit_mem_addr" to network subsystem for debugging pruposes
						__atomic_store_n((unsigned*)&(_net_csrs->csr15_tx1), (unsigned)tx_pkt_upper_limit_mem_addr, memorder);
	   				}

	   				if (DEBUG || SIMULATION_TESTING) {
	   					// update the csr17_tx1 is for writing "tx_pkt_start_mem_addr" to network subsystem for debugging pruposes
						__atomic_store_n((unsigned*)&(_net_csrs->csr17_tx1), (unsigned)tx_pkt_start_mem_addr, memorder);
	   				}

	   				// we'll start with the 3rd point mentioned above, when the desc_table_tx is EMPTY
	   				if (net_csr5_tx1_EMPTY)	{

	   					if (DEBUG)
							printf("\nEntered if (net_csr5_tx1_EMPTY)\n");

	   					if (SIMULATION_TESTING)
	   						write_led(12);

	   					// checking out distance of tx_pkt_prev_wr_ptr tx_pkt from "tx_pkt_upper_limit_mem_addr" 
	   					tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta = (void*)((unsigned)tx_pkt_upper_limit_mem_addr - (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr);

	   					if (DEBUG || SIMULATION_TESTING) {
		   					// update the csr16_tx1 is for writing "tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta" to network subsystem for debugging pruposes
							__atomic_store_n((unsigned*)&(_net_csrs->csr16_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta, memorder);
		   				}


	   					// we need space for 2 tx_pkts in tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta since the prev tx_pkts needs to fit in after 
	   					// its start address and then the current tx_pkt need to fit in after the prev tx_pkt 
	   					// If our delat is less than 2 eth pkts len then we rollover the address of tx_pkt to tx_pkt_start_mem_addr since desc_table_tx is EMPTY
	   					// so we aren't worried about colliding with rd_ptr of DMA 
	   						// +4 for aligning mem address with 4 bytes
	   					if ((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta < (2* (MAX_ETH_PKT_LEN + 4))) {

	   						if (DEBUG)
								printf("\nEntered if ((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta < (2* (MAX_ETH_PKT_LEN + 4)))\n");

	   						if (SIMULATION_TESTING)
	   							write_led(11);

	   						tx_pkt_calculated_start_mem_addr_curr_wr_ptr = tx_pkt_start_mem_addr;
	   							// tx_pkt_start_mem_addr is already 4 byte aligned

	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

	   						// assign the tx_pkt_calculated_start_mem_addr_curr_wr_ptr to tx_pkts_status_desc_table_tx_obj at tx_pkt_curr_wr_ptr
	   						tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr] = tx_pkt_calculated_start_mem_addr_curr_wr_ptr;

	   					}
	   					// else if (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta >= (2* MAX_ETH_PKT_LEN))
	   					else {

	   						if (DEBUG)
								printf("\nEntered else of => if ((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta < (2* (MAX_ETH_PKT_LEN + 4)))\n");

	   						if (SIMULATION_TESTING)
	   							write_led(10);

	   						// if we have memory infront of the previous tx_pkt then we place the curr tx_pkt infront of it
	   						// we will add the len of the prev tx pkt (in bytes) infront of the start mem address of prev tx_pkt to calculate the start address of curr tx_pkt
	   						tx_pkt_calculated_start_mem_addr_curr_wr_ptr = (void*)((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr + tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_prev_wr_ptr]);

	   						// updating csr18 before alignment
	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

	   						// 4 byte align the tx_pkt_calculated_start_mem_addr_curr_wr_ptr
	   						tx_pkt_calculated_start_mem_addr_curr_wr_ptr = AlignToFourByteMemAddress(tx_pkt_calculated_start_mem_addr_curr_wr_ptr);

	   						// updating csr18 after alignment
	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

	   						// assign the tx_pkt_calculated_start_mem_addr_curr_wr_ptr to tx_pkts_status_desc_table_tx_obj at tx_pkt_curr_wr_ptr
	   						tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr] = tx_pkt_calculated_start_mem_addr_curr_wr_ptr;

	   					}	

	   					// insert tx_udp_v2 here

	   					if (DEBUG)
							printf("\nPopulating tx_pkts_status_desc_table_tx_obj\n"); 

	   					tx_pkts_status_desc_table_tx_obj->tx_pkt_active[*tx_pkt_curr_wr_ptr] = 1;
	   					tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_curr_wr_ptr] = txpkt->p_rawlen;
	   					// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr] = tx_pkt_curr_calculated_start_mem_addr;
	   						// already being allocated in the ifelse conditions above

	   					// updating current used and available tx_pkt memory 
	   					*tx_pkt_current_available_memory = *tx_pkt_current_available_memory - txpkt->p_rawlen;
	   					// tx_pkt_current_used_memory = tx_pkt_current_used_memory + txpkt->p_rawlen;

	   					// if we are dubugging on hardware or doing simulation, then we update the debug csr register
	   					if (DEBUG || SIMULATION_TESTING) {
	   						// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
	   						__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), *tx_pkt_current_available_memory, memorder);
	   					}

	   					// now we transmit this tx_pkt, where we will write the start address, tx_pkt len, 

	   					// alotting prev_wr_ptr just before the wr_ptr is incremebted
	   					*tx_pkt_prev_wr_ptr = *tx_pkt_curr_wr_ptr;

	   					// doing this tx_udp_v2() outside this function now
	   					// tx_udp_v2(txpkt, DESTIP, SRC_UDP_PORT, DEST_UDP_PORT, tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr]);

	   					// transmit the txpkt (write the tx_pkt to the mem and populate desc_table_tx at wr_ptr)
	   					// tx_pkt_v3(txpkt, tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);
	   						// this function is inside tx_udp_v2
	   							// txpkt->p_length value will be correct now since we aren't using tx_pkt_v3() directly, we are using it through tx_udp_v2() 

	   					if (SIMULATION_TESTING)
	   						write_led(2);

	   					// break from while loop of trying to send the tx_pkt our since it has been sent out in tx_pkt_v3 inside tx_udp_v2
	   					// break;

	   					// enough space to send txpkt
	   					return 1;

	   				}
	   				// if wr_ptr mem address is >= rd_ptr mem address
	   				else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) >= ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) {

	   					if (DEBUG)
							printf("\nEntered else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) >= ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr))\n"); 

	   					if (SIMULATION_TESTING)
	   						write_led(9);

	   					// checking out distance of tx_pkt_prev_wr_ptr tx_pkt from "tx_pkt_upper_limit_mem_addr" 
	   					tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta = (void*)((unsigned)tx_pkt_upper_limit_mem_addr - (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr);

	   					if (DEBUG || SIMULATION_TESTING) {
		   					// update the csr16_tx1 is for writing "tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta" to network subsystem for debugging pruposes
							__atomic_store_n((unsigned*)&(_net_csrs->csr16_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta, memorder);
		   				}

	   					// we need space for 2 tx_pkts in tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta since the prev tx_pkts needs to fit in after 
	   					// its start address and then the current tx_pkt need to fit in after the prev tx_pkt 
	   					// If our delat is less than 2 eth pkts len then we rollover the address of tx_pkt to tx_pkt_start_mem_addr since desc_table_tx is EMPTY
	   					// so we aren't worried about colliding with rd_ptr of DMA 

	   					// if (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta < (2* MAX_ETH_PKT_LEN) means that we don't have enough mem to store two tx_pkts infront of
	   					// the last starting mem address of the prev tx_pkt so we will rollover the starting mem address of the curr tx_pkt to "tx_pkt_start_mem_addr"
	   						// +4 for aligning mem address with 4 bytes
 	   					if ((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta < (2* (MAX_ETH_PKT_LEN + 4))) {

 	   						if (DEBUG)
								printf("\nEntered if ((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta < (2* (MAX_ETH_PKT_LEN + 4)))\n"); 


	   						// keep checking if there is at least 1 MAX_ETH_PKT_LEN space infront of tx_pkt_start_mem_addr that is not occupied by tx_pkt_calculated_start_mem_addr_curr_rd_ptr
	   						
	   						// while (1) {
 	   							// removing while loop according to the detailed comment under the parent while loop 

 	   						if (SIMULATION_TESTING)
   								write_led(8);

   							// read the current updated rd_ptr from DMA in network subsystem
   							
   							// tx_pkt_curr_rd_ptr = __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder);
   								// removing rd_ptr updates according to the detailed comment under the parent while loop 

   							// I thought this was incorrect but that was only for the case when desc_table_tx was EMPTY
   							// when it is not EMPTY then rd pointer != wr ptr and in that case the current rd ptr would be
   							// parsing through the start mem addresses of tx_pkts that were populated previously by the wr ptr
   							// hence we would be getting correct values.. I'll test this by blocking the TX_PKT_DMA_FSM

   							tx_pkt_calculated_start_mem_addr_curr_rd_ptr = tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_rd_ptr];

   							tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta = (void*)((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr - (unsigned)tx_pkt_start_mem_addr);

   							if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr20_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr20_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta, memorder);
			   				}

   							// if we have enough space infront of "tx_pkt_start_mem_addr" for 1 tx_pkt then we break from the while loop and assign "tx_pkt_start_mem_addr" to tx_pkt_calculated_start_mem_addr_curr_wr_ptr
   								// +4 for aligning mem address with 4 bytes
   							// if ((void*)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) > (MAX_ETH_PKT_LEN + 4)) {
			   					// sw/z/2024_2_12_edgetestbed_a100T27_tx_pipeline_v1_sim.c:593:129: warning: comparison between pointer and integer
   							if ((unsigned)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) > (MAX_ETH_PKT_LEN + 4)) {

   								if (DEBUG)
									printf("\nEntered if ((unsigned)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) > (MAX_ETH_PKT_LEN + 4))\n"); 

   								if (SIMULATION_TESTING)
   									write_led(7);

   								tx_pkt_calculated_start_mem_addr_curr_wr_ptr = tx_pkt_start_mem_addr;

   								// updating csr18 before alignment
		   						if (DEBUG || SIMULATION_TESTING) {
				   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
									__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
				   				}

   								// 4 byte align the tx_pkt_calculated_start_mem_addr_curr_wr_ptr
   								tx_pkt_calculated_start_mem_addr_curr_wr_ptr = AlignToFourByteMemAddress(tx_pkt_calculated_start_mem_addr_curr_wr_ptr);

   								// updating csr18 after alignment
		   						if (DEBUG || SIMULATION_TESTING) {
				   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
									__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
				   				}

   								// assign the tx_pkt_calculated_start_mem_addr_curr_wr_ptr to tx_pkts_status_desc_table_tx_obj at tx_pkt_curr_wr_ptr
   								tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr] = tx_pkt_calculated_start_mem_addr_curr_wr_ptr;

   								// break;

   								// TODO: 
   									// insert tx_udp_v2 here 

   								if (DEBUG)
									printf("\nPopulating tx_pkts_status_desc_table_tx_obj\n");

   								tx_pkts_status_desc_table_tx_obj->tx_pkt_active[*tx_pkt_curr_wr_ptr] = 1;
   								tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_curr_wr_ptr] = txpkt->p_rawlen;
   								// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr] = tx_pkt_curr_calculated_start_mem_addr;
   									// already being allocated in the ifelse conditions above

   								// updating current used and available tx_pkt memory 
   								*tx_pkt_current_available_memory = *tx_pkt_current_available_memory - txpkt->p_rawlen;
   								// tx_pkt_current_used_memory = tx_pkt_current_used_memory + txpkt->p_rawlen;

   								// if we are dubugging on hardware or doing simulation, then we update the debug csr register
   								if (DEBUG || SIMULATION_TESTING) {
   									// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
   									__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), *tx_pkt_current_available_memory, memorder);
   								}

   								// now we transmit this tx_pkt, where we will write the start address, tx_pkt len, 

   								// alotting prev_wr_ptr just before the wr_ptr is incremebted
   								*tx_pkt_prev_wr_ptr = *tx_pkt_curr_wr_ptr;

   								// tx_udp_v2(txpkt, DESTIP, SRC_UDP_PORT, DEST_UDP_PORT, tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr]);

   								// transmit the txpkt (write the tx_pkt to the mem and populate desc_table_tx at wr_ptr)
   								// tx_pkt_v3(txpkt, tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);
   									// this function is inside tx_udp_v2
   										// txpkt->p_length value will be correct now since we aren't using tx_pkt_v3() directly, we are using it through tx_udp_v2() 

   								if (SIMULATION_TESTING)
   									write_led(2);

   								// break from while loop of trying to send the tx_pkt our since it has been sent out in tx_pkt_v3 inside tx_udp_v2
   								// break;

   								// enough space to send txpkt
   								return 1;


	   							// }

	   						}
	   						// else if (tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) <= (MAX_ETH_PKT_LEN + 4)
	   						else {

	   							if (DEBUG)
									printf("\nEntered else == else if (tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) <= (MAX_ETH_PKT_LEN + 4)\n"); 

	   							// do nothing in else statement.. so we need to shift the tx_udp_v2 within these if elses :3
	   								// will go back to top of while loop and wait for memory to be freed again

	   							if (SIMULATION_TESTING)
	   								write_led(4); // warning

	   							if (DEBUG) {
	   								printf("\n(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) <= (MAX_ETH_PKT_LEN + 4) not enough space to send txpkt \n");
	   							}

	   							// not enough space to send txpkt
	   							return 0;
	   						}

	   					}

	   					// if (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta >= (2* MAX_ETH_PKT_LEN) means that we have enough mem to store two tx_pkts infront of
	   					// the last starting mem address of the prev tx_pkt before "tx_pkt_upper_limit_mem_addr" is reached
	   					// so we will add the len of prev tx_pkt to the starting wr_ptr mem address of prev tx_pkt which will be the starting mem address of curr tx_pkt now

	   					// else if (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta >= (2* MAX_ETH_PKT_LEN)) 					
	   					else {

	   						if (DEBUG)
									printf("\nEntered else == else if (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr_delta >= (2* MAX_ETH_PKT_LEN))\n"); 

	   						if (SIMULATION_TESTING)
	   							write_led(6);

	   						// if we have memory infront of the previous tx_pkt then we place the curr tx_pkt infront of it
	   						// we will add the len of the prev tx pkt (in bytes) infront of the start mem address of prev tx_pkt to calculate the start address of curr tx_pkt
	   						tx_pkt_calculated_start_mem_addr_curr_wr_ptr = (void*)((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr + tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_prev_wr_ptr]);

	   						// updating csr18 before alignment
	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

	   						// 4 byte align the tx_pkt_calculated_start_mem_addr_curr_wr_ptr
	   						tx_pkt_calculated_start_mem_addr_curr_wr_ptr = AlignToFourByteMemAddress(tx_pkt_calculated_start_mem_addr_curr_wr_ptr);

	   						// updating csr18 after alignment
	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

	   						// assign the tx_pkt_calculated_start_mem_addr_curr_wr_ptr to tx_pkts_status_desc_table_tx_obj at tx_pkt_curr_wr_ptr
	   						tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr] = tx_pkt_calculated_start_mem_addr_curr_wr_ptr;

	   						// TODO: 
									// insert tx_udp_v2 here 

	   						if (DEBUG)
								printf("\nPopulating tx_pkts_status_desc_table_tx_obj\n"); 

	   						tx_pkts_status_desc_table_tx_obj->tx_pkt_active[*tx_pkt_curr_wr_ptr] = 1;
   							tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_curr_wr_ptr] = txpkt->p_rawlen;
   							// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr] = tx_pkt_curr_calculated_start_mem_addr;
   								// already being allocated in the ifelse conditions above

   							// updating current used and available tx_pkt memory 
   							*tx_pkt_current_available_memory = *tx_pkt_current_available_memory - txpkt->p_rawlen;
   							// tx_pkt_current_used_memory = tx_pkt_current_used_memory + txpkt->p_rawlen;

   							// if we are dubugging on hardware or doing simulation, then we update the debug csr register
   							if (DEBUG || SIMULATION_TESTING) {
   								// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
   								__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), *tx_pkt_current_available_memory, memorder);
   							}

   							// now we transmit this tx_pkt, where we will write the start address, tx_pkt len, 

   							// alotting prev_wr_ptr just before the wr_ptr is incremebted
   							*tx_pkt_prev_wr_ptr = *tx_pkt_curr_wr_ptr;

   							// tx_udp_v2(txpkt, DESTIP, SRC_UDP_PORT, DEST_UDP_PORT, tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr]);

   							// transmit the txpkt (write the tx_pkt to the mem and populate desc_table_tx at wr_ptr)
   							// tx_pkt_v3(txpkt, tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);
   								// this function is inside tx_udp_v2
   									// txpkt->p_length value will be correct now since we aren't using tx_pkt_v3() directly, we are using it through tx_udp_v2() 
   							
   							if (SIMULATION_TESTING)
   								write_led(2);

   							// break from while loop of trying to send the tx_pkt our since it has been sent out in tx_pkt_v3 inside tx_udp_v2
   							// break;

   							// enough space to send txpkt
   							return 1;

	   					}


	   				}

	   				// if wr_ptr mem address is < rd_ptr mem address
	   				else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) < ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) {

	   					if (DEBUG)
							printf("\nEntered else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) < ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr))\n"); 

	   					// keep checking if there is at least 2 MAX_ETH_PKT_LEN space infront of tx_pkt_calculated_start_mem_addr_prev_wr_ptr that is not occupied by tx_pkt_calculated_start_mem_addr_curr_rd_ptr
	   					// cx the prev tx_pkt need 1 MAX_TX_PKT_LEN and the curr tx_pkt needs 1 MAX_TX_PKT_LEN
	   					
	   					// while (1) {
	   						// removing while loop according to the detailed comment under the parent while loop 
	   					
	   					if (SIMULATION_TESTING)
   							write_led(4);

   						// read the current updated rd_ptr from DMA in network subsystem

   						// tx_pkt_curr_rd_ptr = __atomic_load_n((unsigned*)&(_net_csrs->csr8_tx1), memorder);
   							// removing rd_ptr updates according to the detailed comment under the parent while loop

   						tx_pkt_calculated_start_mem_addr_curr_rd_ptr = tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_rd_ptr];

   						tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta = (void*)((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr - (unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr);

   						if (DEBUG || SIMULATION_TESTING) {
		   					// update the csr20_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta" to network subsystem for debugging pruposes
							__atomic_store_n((unsigned*)&(_net_csrs->csr20_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta, memorder);
		   				}


   						// if we have enough space infront of "tx_pkt_start_mem_addr" for 1 tx_pkt then we break from the while loop and assign "tx_pkt_start_mem_addr" to tx_pkt_calculated_start_mem_addr_curr_wr_ptr
   							// +4 for aligning mem address with 4 bytes
   						// if ((void*)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) >= (2 * (MAX_ETH_PKT_LEN + 4))) {
   							// <pre><b>sw/z/2024_2_12_edgetestbed_a100T27_tx_pipeline_v1_sim.c:685:121:</b> <font color="#A347BA"><b>warning: </b></font>comparison between pointer and integer </pre>
   						if ((unsigned)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) >= (2 * (MAX_ETH_PKT_LEN + 4))) {

   							if (DEBUG)
								printf("\nEntered if ((unsigned)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) >= (2 * (MAX_ETH_PKT_LEN + 4))) \n"); 

   							if (SIMULATION_TESTING)
   								write_led(3);

   							// if we have memory infront of the previous tx_pkt then we place the curr tx_pkt infront of it
   							// we will add the len of the prev tx pkt (in bytes) infront of the start mem address of prev tx_pkt to calculate the start address of curr tx_pkt
   							tx_pkt_calculated_start_mem_addr_curr_wr_ptr = (void*)((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr + tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_prev_wr_ptr]);

   							// updating csr18 before alignment
	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

   							// 4 byte align the tx_pkt_calculated_start_mem_addr_curr_wr_ptr
   							tx_pkt_calculated_start_mem_addr_curr_wr_ptr = AlignToFourByteMemAddress(tx_pkt_calculated_start_mem_addr_curr_wr_ptr);

   							// updating csr18 after alignment
	   						if (DEBUG || SIMULATION_TESTING) {
			   					// update the csr18_tx1 is for writing "tx_pkt_calculated_start_mem_addr_curr_wr_ptr" to network subsystem for debugging pruposes
								__atomic_store_n((unsigned*)&(_net_csrs->csr18_tx1), (unsigned)tx_pkt_calculated_start_mem_addr_curr_wr_ptr, memorder);
			   				}

   							// assign the tx_pkt_calculated_start_mem_addr_curr_wr_ptr to tx_pkts_status_desc_table_tx_obj at tx_pkt_curr_wr_ptr
   							tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr] = tx_pkt_calculated_start_mem_addr_curr_wr_ptr;

   							// break;

   							// TODO: 
									// insert tx_udp_v2 here 

   							tx_pkts_status_desc_table_tx_obj->tx_pkt_active[*tx_pkt_curr_wr_ptr] = 1;
   							tx_pkts_status_desc_table_tx_obj->tx_pkt_len_bytes[*tx_pkt_curr_wr_ptr] = txpkt->p_rawlen;
   							// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr] = tx_pkt_curr_calculated_start_mem_addr;
   								// already being allocated in the ifelse conditions above

   							// updating current used and available tx_pkt memory 
   							*tx_pkt_current_available_memory = *tx_pkt_current_available_memory - txpkt->p_rawlen;
   							// tx_pkt_current_used_memory = tx_pkt_current_used_memory + txpkt->p_rawlen;

   							// if we are dubugging on hardware or doing simulation, then we update the debug csr register
   							if (DEBUG || SIMULATION_TESTING) {
   								// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
   								__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), *tx_pkt_current_available_memory, memorder);
   							}

   							// now we transmit this tx_pkt, where we will write the start address, tx_pkt len, 

   							// alotting prev_wr_ptr just before the wr_ptr is incremebted
   							*tx_pkt_prev_wr_ptr = *tx_pkt_curr_wr_ptr;

   							// tx_udp_v2(txpkt, DESTIP, SRC_UDP_PORT, DEST_UDP_PORT, tx_pkts_status_desc_table_tx_obj->tx_pkt_current_start_mem_address[*tx_pkt_curr_wr_ptr]);

   							// transmit the txpkt (write the tx_pkt to the mem and populate desc_table_tx at wr_ptr)
   							// tx_pkt_v3(txpkt, tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);
   								// this function is inside tx_udp_v2
   									// txpkt->p_length value will be correct now since we aren't using tx_pkt_v3() directly, we are using it through tx_udp_v2() 
   						
   							if (SIMULATION_TESTING)
   								write_led(2);

   							// break from while loop of trying to send the tx_pkt our since it has been sent out in tx_pkt_v3 inside tx_udp_v2
   							// break;

   							// enough space to send txpkt
   							return 1;

   							// we aren't checking if the new curr wr_ptr mem address can be within 1 MAX_ETH_PKT_LEN distance of "tx_pkt_upper_limit_mem_addr"
   							// cx for the case when wr_ptr is > rd_ptr, it will always rollover to start mem address if it is within 1*MAX_ETH_PKT_LEN distance of "tx_pkt_upper_limit_mem_addr"
   							// so the rd_ptr will follow that write pointer and will neve be within 1*MAX_ETH_PKT_LEN distance of "tx_pkt_upper_limit_mem_addr"
   							// hence when we will check if mem address of prev wr_ptr is at a distance more than 2*MAX_ETH_PKT_LEN of rd_ptr mem address, then the current wr_ptr mem address
   							// will not surpass the "tx_pkt_upper_limit_mem_addr" since the rd_ptr mem address that it is following, is its self at least MAX_ETH_PKT_LEN bytes away from 
   							// "tx_pkt_upper_limit_mem_addr".

   						}

   						// else{ }?? no else since if there isn't enough space then go bck to start of while loop and wait for space to become available
   						// (unsigned)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) < (2 * (MAX_ETH_PKT_LEN + 4))
   						else {



   							if (SIMULATION_TESTING)
   								write_led(4); // warning

   							if (DEBUG) {
   								printf("\n(unsigned)(tx_pkt_calculated_start_mem_addr_curr_rd_ptr_delta) < (2 * (MAX_ETH_PKT_LEN + 4)) not enough space to send txpkt \n");
   							}

   							// not enough space to send txpkt
   							return 0;

   						}
	   					// }

	   				}
	   				// else if !(((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) < ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) || !else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) >= ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) 
	   				// print error msg and leds
	   				else {

	   					// raise an error if there is some other condition 

	   					// print error msg and turn on warning led
		   				if ((DEBUG) || (SIMULATION_TESTING)) {


		   					if (SIMULATION_TESTING && DEBUG) {

		   						// printing error message
		   						printf("error!! !(((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) < ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) || \\
			   							!else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) >= ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr))  \n unexpected if else condition");

		   						// led = 5 means there is an error
		   						write_led(5);

		   						// writing led2 after led5 is just to locatlize the bug
		   						write_led(2);

		   						// wait 2 seconds
		   						// sleep(2000000); 
		   							// no sleep when SIM & DEBUG 

		   					} else if (SIMULATION_TESTING) {

		   						// led = 5 means there is an error
		   						write_led(5); 

		   						// writing led2 after led5 is just to locatlize the bug
		   						write_led(2);


		   					} else if (DEBUG) {

		   						// printing error message
		   						printf("\nerror!! !(((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) < ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) || \\
			   							!else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) >= ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr))  \n unexpected if else condition with LED = 5 & 2 and 4 sec wait \n");

		   						// led = 5 means there is an error
		   						write_led(5); 

		   						// delay introduced for testing on hardware

		   						// wait 2 seconds
		   						// sleep(2000000);

		   						// writing led2 after led5 is just to locatlize the bug
		   						write_led(2);

		   						// wait 2 seconds
		   						// sleep(2000000);

		   					}

		   				}

		   				// break from the while loop of the tx_pkt 
			   			// break;

			   			write_led(5); // error

			   			if (DEBUG) {
			   				printf("\nerror!! !(((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) < ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr)) || \\
			   							!else if (((unsigned)tx_pkt_calculated_start_mem_addr_prev_wr_ptr) >= ((unsigned)tx_pkt_calculated_start_mem_addr_curr_rd_ptr))  \n unexpected if else condition with LED = 5 & 2 \n");
			   			}

			   			// not enough space to send txpkt or error
   						return -1;

	   				}


	   				// if there were any errors we would have broken from this while loop
	   				// otherwise if the desc_table_tx if full or there isn't enough
	   				// available tx_pkt memory, then the while loop will keep on looping
	   				// till there is space in desc_table_tx and in memory.. 
	   					// in the later stages of the code we will not get stuck into this while loop
	   					// we will send the tx_pkt to waiting_pkt and run the rest of the code and either wait for 
	   					// interrupt from waiting_pkt through network subsystem or from locally checking if there is enough mem for tx_pkts
	   					// and upon receiving that interrupt, we will run the tx_pkt function


	   				// update TX_PKTS_DESC_TABLE_TX_STATUS object

	   				// TODO: shift all this below with the "// insert tx_udp_v2 here " comments above

	   				/*

	   				tx_pkts_status_desc_table_tx_obj.tx_pkt_active[tx_pkt_curr_wr_ptr] = 1;
	   				tx_pkts_status_desc_table_tx_obj.tx_pkt_len_bytes[tx_pkt_curr_wr_ptr] = txpkt->p_rawlen;
	   				// tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr] = tx_pkt_curr_calculated_start_mem_addr;
	   					// already being allocated in the ifelse conditions above

	   				// updating current used and available tx_pkt memory 
	   				tx_pkt_current_available_memory = tx_pkt_current_available_memory - txpkt->p_rawlen;
	   				// tx_pkt_current_used_memory = tx_pkt_current_used_memory + txpkt->p_rawlen;

	   				// if we are dubugging on hardware or doing simulation, then we update the debug csr register
	   				if (DEBUG || SIMULATION_TESTING) {
	   					// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
	   					__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), tx_pkt_current_available_memory, memorder);
	   				}

	   				// now we transmit this tx_pkt, where we will write the start address, tx_pkt len, 

	   				// alotting prev_wr_ptr just before the wr_ptr is incremebted
	   				tx_pkt_prev_wr_ptr = tx_pkt_curr_wr_ptr;

	   				tx_udp_v2(txpkt, DESTIP, SRC_UDP_PORT, DEST_UDP_PORT, tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);

	   				// transmit the txpkt (write the tx_pkt to the mem and populate desc_table_tx at wr_ptr)
	   				// tx_pkt_v3(txpkt, tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);
	   					// this function is inside tx_udp_v2
	   						// txpkt->p_length value will be correct now since we aren't using tx_pkt_v3() directly, we are using it through tx_udp_v2() 

	   				write_led(2);

	   				*/

	   				
	   			// else ending here	
	   			}
	   		} 
	   		else {

	   			// led = 4 means there is a warning
	   			if(SIMULATION_TESTING)
		   			write_led(4);

	   			if (DEBUG) {

					// printing error message
					printf("\n There is not enough memory available for this tx_pkt, tx_pkt_current_available_memory = %d \n", *tx_pkt_current_available_memory);

					// delay introduced for testing on hardware

					// wait 2 seconds
					// sleep(2000000);
						// no need to sleep for 2 sec here

		   		}

		   		// not enough space to send txpkt
				return 0;


	   		}
	   	}
	   	// else if (tx_pkts_status_desc_table_tx_obj.tx_pkt_active[tx_pkt_curr_wr_ptr] == 1) {
	   	else {

	   		// led = 4 means there is a warning
	   		if(SIMULATION_TESTING)
   				write_led(4);

   			if (DEBUG) {

				// printing error message
				printf("\ntx_pkts_status_desc_table_tx_obj->tx_pkt_active[tx_pkt_curr_wr_ptr] == 1, wait for memory to be freed \n");
			}

			// not enough space to send txpkt
			return 0;

	   	}
   	} 
   	else {


   		if(SIMULATION_TESTING)
	   		// led = 4 means there is a warning
			write_led(4);

			if (DEBUG) {

			// printing error message
			printf("\nThe desc_table_tx is FULL \n");

			// delay introduced for testing on hardware

			// wait 2 seconds
			// sleep(2000000);
				// no need to sleep for 2 sec here
   		}

   		// not enough space to send txpkt
   		return 0;

   	}



}