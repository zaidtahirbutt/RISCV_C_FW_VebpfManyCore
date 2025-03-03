////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	pkt.h
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	Describes a packet that can be received, shared, processed,
//		and transmitted again
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
#ifndef	PKT_H
#define	PKT_H
#define DESC_TABLE_TX_DEPTH 4 

typedef struct {
	int	p_usage_count, p_rawlen, p_length;
	char	*p_raw,	// Points to the beginning of raw packet memory
		*p_user;   // Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
} NET_PACKET;

typedef struct {

	// I can declare a int pointer and then malloc it with size of DESC_TABLE_TX_DEPTH
	// and then declare a pointer to this struct and access access those pointer int elements
	// using something like this: pkt->p_user[0] = (sport >> 8) & 0x0ff;
	// as we are doing for NET_PACKET *pkt; and then doing the above stuff..
		// I think I can do the same if I used NET_PACKET pkt; and did .. pkt.p_user[0] = (sport >> 8) & 0x0ff;
			/*A dot in C++ can be used to access an element contained within a structure, 
			whereas an arrow is used to access an element contained within a pointer to a structure.*/
	// but unlike the pkt we have a fixed DESC_TABLE_TX_DEPTH while the pkt needs variable length that can be changed using 
	// malloc and then parsing through that memory using dereferencing etc... hence we will use static arrays.. not static as in keyword
	// but static as in fixed length

	// this will keep track of tx_pkt status in desc_table_tx in network subsystem 
		// tx_pkt_active[i] = 0 is init value
		// tx_pkt_active[i] = 1 means the RISC-V just wrote the  tx_pkt desc_table_tx entry index "i" metadata to network subsystem
		// tx_pkt_active[i] = 2 means the network subsystem rd_ptr has passed by the desc_table_tx entry index "i" and has transmitted that tx_pkt out

	unsigned  tx_pkt_active[DESC_TABLE_TX_DEPTH];
	unsigned  tx_pkt_len_bytes[DESC_TABLE_TX_DEPTH];
	void	 *tx_pkt_current_start_mem_address[DESC_TABLE_TX_DEPTH];

	// another way was to just declare pointers and do malloc to allocate space to each stuct member element and then parse through the array using square bracket indexing

}  TX_PKTS_DESC_TABLE_TX_STATUS;

extern void* 	AlignToFourByteMemAddress(void *input_pointer); 

extern void 	update_tx_pkt_avail_memory(unsigned *tx_pkt_prev_rd_ptr, unsigned *tx_pkt_curr_rd_ptr, unsigned *tx_pkt_current_available_memory, TX_PKTS_DESC_TABLE_TX_STATUS *tx_pkts_status_desc_table_tx_obj);

extern int     tx_pkt_memory_availability_calculation(NET_PACKET *txpkt, unsigned *tx_pkt_curr_wr_ptr, unsigned *tx_pkt_prev_wr_ptr, unsigned *tx_pkt_curr_rd_ptr, 
												unsigned *tx_pkt_prev_rd_ptr, unsigned *tx_pkt_current_available_memory, TX_PKTS_DESC_TABLE_TX_STATUS *tx_pkts_status_desc_table_tx_obj,
												unsigned tx_pkt_upper_limit_mem_addr, unsigned tx_pkt_start_mem_addr);


extern	NET_PACKET	 *rx_pkt(void);
extern	NET_PACKET	 *rx_pkt2(void);  // new function for the updated HDL
extern	NET_PACKET	 *rx_pkt3(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error);  // new function for the updated HDL
extern	NET_PACKET	 *rx_pkt4_VeBPF_Demo(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error);  // new function for the updated HDL
extern	NET_PACKET	 *rx_pkt4_VeBPF_Demo_debug_v1(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error);  // new function for testing for bugs
extern	NET_PACKET	 *rx_pkt_v5(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error, unsigned flag_load_rxpkt_and_VeBPF_result);  // new function for testing for bugs
extern	NET_PACKET	 *rx_pkt_v6(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error, unsigned flag_load_rxpkt_and_VeBPF_result); 
extern	NET_PACKET	 *rx_pkt_v6_only_UDP_hdr(unsigned* vebpf_dest,unsigned* vebpf_valid, unsigned* vebpf_error, unsigned flag_load_rxpkt_and_VeBPF_result); 


// reduced ver of v6, just increments the rdptr
extern	void	     *rx_pkt_v6_reduced(void); 


extern	void		pkt_reset(NET_PACKET *pkt);
extern	void		tx_pkt(NET_PACKET *pkt);
extern	void		tx_pkt_v2(NET_PACKET *pkt);
extern	void		tx_pkt_v3(NET_PACKET *pkt, void *tx_pkt_current_start_mem_address);
extern	NET_PACKET	*new_pkt(unsigned msglen);
extern	void		free_pkt(NET_PACKET *pkt);
extern	void		dump_raw(NET_PACKET *pkt);
//extern void			tx_busy(NET_PACKET *);  //s

#endif
