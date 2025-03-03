// #include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "board.h"
#include "pkt.h"
  
//#include "pkt.c" ss
   
#include "ipproto.h"
#include "arp.h"
#include "icmp.h"
//#include "icmp.c"  // writing this solved error of cannot find icmp ping function

#include "udpproto.h"
#include "protoconst.h"
#include "etcnet.h"
#include "ethproto.h"
#include "../../../../riscv_subsystem/sw/utils.h" // include just once!
#define 	memorder 0

#define MAX_RX_PKT_MEM	210000000  // almost = 210 MB .. keeping in mind arty100t has total DDR3 mem = 256 MBs
// #define MAX_RX_PKT_MEM	32768  // 2^15  = 32 kB
// #define MAX_RX_PKT_MEM	16384  // 2^14

// #define MAX_RX_PKT_MEM	4096  // 2^12

// #define MAX_RX_PKT_MEM	1024  // 2^10
	// Due to the following condition, there wasn't enough space for a 2nd rx pkt, the rxpkt to mem writing FSM did not move:
		// (total_mem_words_used_rx_pkts_reg + 512) < rx_pkt_alloc_mem_words_size)

#define ETH_HDR_LEN		14
// #define ETH_RX_PKT1_LEN_HARDCORDED 298 

// Make this constant equal to 1 if you are generating hex file for simulation 
// #define SIMULATION_TESTING 0
	// defining this in pkt.c as well

#define LOADING_RX_PKT_DATA 1

#define NOT_LOADING_RX_PKT_DATA 0

// #define EXPECECTED_CORRECT_EBPF_RESULT 2

#define EBPF_RESULT_DONT_CARE 1
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE1 8000
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE2 18000
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE3 8000
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE4 34000

#define IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS 20 //4
#define TOTAL_MALICIOUS_RXPKTS_FOR_LATENCY_CAL 34000  // for TYPE4 firewall Experiments 18000  // for TYPE2 firewall Experiments  // 8000  // for TYPE1 firewall Experiments // 1100 // 5

// #define	UDP_DEV_PORT	6783
// #define	UDP_HOST_PORT	6784
// #include <linux/membarrier.h>
//#include "txfns.h"
// comment to recompile using MAKE :3
// need to change the board.h address values according to the edgetestbed addressing (have written these addresses down in my vsim3 folder)

//extern void	tx_busy(NET_PACKET *); 
// a
// Acknowledge all interrupts, and shut all interrupt sources off
// #define	CLEARPIC	0x7fff7fff  // 1111111111111110111111111111111 // 1 1111 1111 1111 1101 1111 1111 1111
// // Turn off the master interrupt generation
// #define	DISABLEINTS	0x80000000  // 1000 0000 0000 0000 0000 0000 0000 0000

// #define	REPEATING_TIMER	0x80000000

// unsigned	heartbeats = 0, lasthello;

//////////////// notes for this c file: //////////////////////
// Chaning if condition that checks if expected resilt != 1 (localparam VEBPF_RESULT_DONT_CARE_KEEP_PROCESSING = 1;) 
//, also not checking drop packet result fpr this c file since DONT care result or DROP packet result is the same for this experiment.. That might not be the
// case for later experiments though .. drop packet result value check is -> or != localparam VEBPF_RESULT_DROP_PKT = 254; ..
// will leave out condition for error cx we are checking error bit, this was the error result value or != localparam VEBPF_RESULT_ERROR = 253;
//////////////////////////////////////////////////////////////

NET_PACKET	*waiting_pkt = NULL;


void	tx_busy(NET_PACKET *txpkt) {
	if (waiting_pkt == NULL) {
		// printf("TX-BUSY\n");
		waiting_pkt = txpkt;
	} else if (txpkt != waiting_pkt) {
		//txstr("Busy collision--deleting waiting packet\n");
		free_pkt(waiting_pkt);
		waiting_pkt = txpkt;
	}
}

// Hang the process until the following is done
	// waiting for sw4 to become 1
	// waiting for sw4 to go back to 0  
void hang_process_loop () {

	int sw_value;

	// waiting for sw4 to become 1 
    while(1) {
    
    	sw_value = read_sw();

	    if (sw_value == 8) {

	    	break;

	    }
	
	}

	// waiting for sw4 to go back to 0 
    while(1) {
    
    	sw_value = read_sw();

	    if (sw_value == 0) {

	    	write_led(0);
	    	break;

	    }
	
	}

}

// MAKE SIMULATION_TESTING and DEBUG define preprocessor directives 1 & 0 respectively in board.h 
int	main(int argc, char **argv) {

	// ***vip!!!*** set this constant to 1 for simulation => #define SIMULATION_TESTING 0 //1 //0

	
	NET_PACKET	*rcvd;
	int 		led2 = 2;
	
	// unsigned 		led3 = 4;	// means 0b100
	int 		led3 = 4;	// should have been int since atomic load for led write function requires 
		
	int 		led4 = 8;	// means 0b1000
	int			sw3 = 4;
	int 		sw3_value = 0;
	unsigned 	rx_pkt_eth_type;
	unsigned	rx_pkt_ip_sub_protocol;
	unsigned	rx_pkt_ip_src; 
	unsigned	rx_pkt_ip_dst; 
	unsigned	rx_pkt_udp_port_dst; 
	unsigned	rx_pkt_udp_port_src;

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	net_csr1;

	NET_PACKET	*rx_pkt_1;
	unsigned rxpkt_vebpf_dest, rxpkt_vebpf_valid, rxpkt_vebpf_error, rxpkt_vebpf_empty, rxpkt_vebpf_avail;

	// counting malicious rxpkts identified by firewall rules 
	unsigned count_malicious_rxpkts = 0;

	int start_timer = 0; //read_timer();
	int total_time_taken;
	int sw_value = 0;
	int reception_started_flag = 0;
	// Allocating starting memory address and total memory to network subsystem for rxpkts below:

	/* allocate a buffer with room to add 0-15 bytes to ensure 16-alignment */
    void* ptr = (void*)malloc(MAX_RX_PKT_MEM + 15); // do free this pointer when mem no longer required (have a condition in place to do that)

    /* round up to multiple of 16: add 15 and then round down by masking */
    void *rx_pkt_start_mem_addr = (void*)(((long)ptr+15) & ~0x0F);
	    // had made an error here while using the "->" notation I assumed it would be giving the struct pointer member's address
	    // but the "->" operator dereferences it. See https://www.geeksforgeeks.org/arrow-operator-in-c-c-with-examples/
	    // So I need to add an "&"
	    	// I think its due to the fact that I didn't declare csr1-4 as ponters in the struct

    // write the 4 byte memory ptr address of this allocated memory (starting heap address cx its dynamic allocation) to network subsystem
    __atomic_store_n((unsigned*)&(_net_csrs->csr3), rx_pkt_start_mem_addr, memorder);  // rx_pkt_start_mem_addr ptr value is 32 bit for rv32

    // write the allocated memory size for rx pkts to the network subsystem alloc size register
    __atomic_store_n((unsigned*)&(_net_csrs->csr4), MAX_RX_PKT_MEM, memorder);

    // memory allocation for rxpkts to network subsystem has been completed

    // wait for sw-4 to go to 1 to start this program below
    if (!SIMULATION_TESTING) {
	
	    // waiting for sw_value to become 8
	    while(1) {
	    
		    write_led(1);
	    	sw_value = read_sw();

	    	if (!(SIMULATION_TESTING)) 
	    		printf("\nTurn sw4 HIGH to start reading received rxpkts in the arty100T FPGA\n");

	    	// wait 2 seconds
	    	sleep(2000000);
	    		// puting sleep here so that we can connect the serial monitor and read the printed data
	    		// otherwise it is not possible to connect the serial monitor if the host pc UART port is 
	    		// busy reading the print statements without delay

		    if (sw_value == 8) {

		    	write_led(0);
		    	break;

		    }
		
		}

	} 

	// forever loop, keep reading rxpkts 
	while(1) {

		// write_led(11); 


		if (!reception_started_flag) {

			// reset timer to current clk until the IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS + 1 rxpkt is received
			start_timer = read_timer();

		}

		// Keep checking if a rxpkt is available 
		while(1) {

			// loading csr1 register from network subsystem
	    	net_csr1 = __atomic_load_n((unsigned*)&(_net_csrs->csr1), memorder);

	    	rxpkt_vebpf_empty = NET_RX_FIFO_EMPTY(net_csr1);
	    	rxpkt_vebpf_avail = NET_RX_PKT_AVAIL(net_csr1);
	    	rxpkt_vebpf_valid = NET_RX_PKT_VeBPF_VALID(net_csr1);

	    	if ((rxpkt_vebpf_valid == 1)) {
		    	if((rxpkt_vebpf_empty == 0) && (rxpkt_vebpf_avail)) {

		    		// rx pkt is available, break this while loop
		    		break;  
		    	}
	    	}
	    }

	    // reading rxpkt
	    	// this version of the function only reads the header of rxpkt
	    rx_pkt_1 = rx_pkt_v6_only_UDP_hdr(&rxpkt_vebpf_dest, &rxpkt_vebpf_valid, &rxpkt_vebpf_error, LOADING_RX_PKT_DATA);

	     // check what the is the rx pkt ethernet type
		rx_pkt_eth_type = ethpkt_ethtype_v2(rx_pkt_1);	

		switch(rx_pkt_eth_type) { 

			case ETHERTYPE_IP: {

					// strip off the eth header from the rx pkt
					rx_ethpkt_v2(rx_pkt_1);	
					
					rx_pkt_ip_src = ippkt_src(rx_pkt_1);
					
					// EBPF_RULE1_SRC_IP mentioned in etcnet.h				
					// if (rx_pkt_ip_src == EBPF_RULE1_SRC_IP) {  // for sim						
					if (rx_pkt_ip_src == EBPF_RULE1_SRC_IP_SYN) {						

						// Malicious firewall rule1 rxpkt received
						// write_led(1);

						// add to total malicious rxpkts received
				    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

				    	
					// } else if (rx_pkt_ip_src == EBPF_RULE2_SRC_IP) {
					} else if (rx_pkt_ip_src == EBPF_RULE2_SRC_IP_SYN) {

						// Malicious firewall rule2 rxpkt received
						// write_led(2);

						// add to total malicious rxpkts received
				    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

					// }  else if (rx_pkt_ip_src == EBPF_RULE3_SRC_IP) {
					}  else if (rx_pkt_ip_src == EBPF_RULE3_SRC_IP_SYN) {

						// Malicious firewall rule3 rxpkt received
						// write_led(3);

						// add to total malicious rxpkts received
				    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

					// } else if (rx_pkt_ip_src == EBPF_RULE4_SRC_IP) {
					} else if (rx_pkt_ip_src == EBPF_RULE4_SRC_IP_SYN) {

						// Malicious firewall rule4 rxpkt received
						// write_led(4);

						// add to total malicious rxpkts received
				    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

					// in else condition check UDP DEST PORT 	
					} else {

						// sub protocol of the ip pkt
						rx_pkt_ip_sub_protocol = ippkt_subproto(rx_pkt_1);

						// strip off the ip header
						rx_ippkt(rx_pkt_1);

						switch(rx_pkt_ip_sub_protocol) {

							case IPPROTO_UDP: {

								rx_pkt_udp_port_dst = udp_dport(rx_pkt_1);

								if (rx_pkt_udp_port_dst == EBPF_RULE5_DEST_UDP_PORT) {

									// Malicious firewall rule5 rxpkt received
									// write_led(5);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE6_DEST_UDP_PORT) {

									// Malicious firewall rule6 rxpkt received
									// write_led(6);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE7_DEST_UDP_PORT) {

									// Malicious firewall rule7 rxpkt received
									// write_led(7);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE8_DEST_UDP_PORT) {

									// Malicious firewall rule8 rxpkt received
									// write_led(8);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE9_DEST_UDP_PORT) {

									// Malicious firewall rule9 rxpkt received
									// write_led(7);  // led 9 only used for incorrect/non-malicious rxpkts

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE10_DEST_UDP_PORT) {

									// Malicious firewall rule10 rxpkt received
									// write_led(10);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE11_DEST_UDP_PORT) {

									// Malicious firewall rule11 rxpkt received
									// write_led(2);  // led 11 only used for showing polling

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE12_DEST_UDP_PORT) {

									// Malicious firewall rule12 rxpkt received
									// write_led(12);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE13_DEST_UDP_PORT) {

									// Malicious firewall rule13 rxpkt received
									// write_led(13);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE14_DEST_UDP_PORT) {

									// Malicious firewall rule14 rxpkt received
									// write_led(14);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE15_DEST_UDP_PORT) {

									// Malicious firewall rule15 rxpkt received
									// write_led(15);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE16_DEST_UDP_PORT) {

									// Malicious firewall rule16 rxpkt received
									// write_led(1);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else if (rx_pkt_udp_port_dst == EBPF_RULE17_DEST_UDP_PORT) {

									// Malicious firewall rule17 rxpkt received
									// write_led(2);

									// add to total malicious rxpkts received
							    	count_malicious_rxpkts = count_malicious_rxpkts + 1;

								} else {

									// Incorrect rxpkt received
									// write_led(9);

									if (!SIMULATION_TESTING)
										printf("\n\nled = 9 NON MALICIOUS rxpkt and Rxpkt RISCV filtering result DEST = 1 Total count_malicious_rxpkts = %d\n\n",  count_malicious_rxpkts);

								}

								// case break statement 
								break;

							}

							default: {

								// Incorrect rxpkt received
								// write_led(9);
								if (!SIMULATION_TESTING)
									printf("\n\nled = 9 NON MALICIOUS rxpkt and Rxpkt RISCV filtering result DEST = 1 Total count_malicious_rxpkts = %d\n\n",  count_malicious_rxpkts);

								// case break statement
								break;
							
							}

						}

					}


				// case break 
				break;

			}

			default: {

				// Incorrect rxpkt received
				// write_led(9);
				if (!SIMULATION_TESTING)
					printf("\n\nled = 9 NON MALICIOUS rxpkt and Rxpkt RISCV filtering result DEST = 1 Total count_malicious_rxpkts = %d\n\n",  count_malicious_rxpkts);

				// case break 
				break;
			}

		}


    	// if rxpkt is NULL then don't print the UDP data and just wait for the sw4 to be toggled 
    	if (rx_pkt_1 == NULL) {
	   
	    	write_led(0);

		    if (rx_pkt_1->p_usage_count) {
		    	
		    	free_pkt(rx_pkt_1);

    	    	// check if rxpkt was freed or not
    	    	if (rx_pkt_1->p_usage_count) {

    		    	free_pkt(rx_pkt_1);
    		    	// write_led(14);
    		    	
    		    }


		    }

	    } else {

	    	// free the rxpkt
	    	free_pkt(rx_pkt_1);

	    	// check if rxpkt was freed or not
	    	if (rx_pkt_1->p_usage_count) {

		    	free_pkt(rx_pkt_1);
		    	// write_led(0);
		    	
		    }

		}

		if (count_malicious_rxpkts >= IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS) {
					    
	    	// reception of IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS malicious rxpkt has started
	    	reception_started_flag = 1;
    	
		}

		if (count_malicious_rxpkts >= IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS + TOTAL_MALICIOUS_RXPKTS_FOR_LATENCY_CAL) {

			// start_timer started at IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS and ended at TOTAL_MALICIOUS_RXPKTS_FOR_LATENCY_CAL
			total_time_taken = read_timer() - start_timer;

			count_malicious_rxpkts = 0;
			reception_started_flag = 0;

			if (!SIMULATION_TESTING) {
				printf("RISCV based firewall experiment filtered total rxpkts = %d and total time it took = %d us", TOTAL_MALICIOUS_RXPKTS_FOR_LATENCY_CAL, total_time_taken);
				printf("RISCV based firewall experiment filtered total rxpkts = %d and total time it took = %d us", TOTAL_MALICIOUS_RXPKTS_FOR_LATENCY_CAL, total_time_taken);
			}

			write_led(0xf);

			// break;

			// Hang the process until the following is done
				// waiting for sw4 to become 1
				// waiting for sw4 to go back to 0  
			if (!SIMULATION_TESTING)
				hang_process_loop();
		}

	}

}	

