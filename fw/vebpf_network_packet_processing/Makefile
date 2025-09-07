# Hello world
# SOURCE	= ./sw/z/2024_10_13_simpeLedHIGH.c

# RISCV rx pkt filtering fw source file (SIMULATION)
# SOURCE	= ./sw/z/2024_1_24_edgetestbed_a100T26_RISCV_throughput_cal_firewallTYPE1_v5_v2_onlyHdr_TYPE4_Exp_throughput_cal_sim.c

# VeBPF rx pkt filtering throughput calculation fw source file
# SOURCE	= ./sw/z/2024_1_11_edgetestbed_a100T26_VeBPF_throughput_cal_firewallTYPE1_v5_TYPE4_Exp_SYN.c

# VeBPF rx pkt filtering fw source file (SIMULATION)
SOURCE	= ./sw/z/2024_9_12_edgetestbed_a100T30_VeBPF_pgmLoaderV2_firewallTYPE1combinedSim_reducedLinker_1SIM_0DEBUG.c

# tx pkt fw source file
# SOURCE	= ./sw/z/2024_5_16_edgetestbed_a100T27_tx_pipeline_v15_SYN_reducedLinker_0SIM_1DEBUG_0UDPcksum_1IPv4Cksum.c
	
OUTPUT_NAME = firmware
CONFIG = rv32i
ABI = ilp32
LINKER = ./sw/firmware.ld
CROSS = riscv32-unknown-elf-
CROSSCFLAGS = -O3 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib
CROSSLDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M 


.PHONY: all reset_handler

all:	reset_handler $(OUTPUT_NAME).hex

icmp.o: ./sw/z/icmp.c ./sw/z/icmp.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	
pkt.o: ./sw/z/pkt.c ./sw/z/pkt.h $(OUTPUT_NAME).o
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	
arp.o: ./sw/z/arp.c ./sw/z/arp.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

ethproto.o: ./sw/z/ethproto.c ./sw/z/ethproto.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

ipcksum.o: ./sw/z/ipcksum.c ./sw/z/ipcksum.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

ipproto.o: ./sw/z/ipproto.c ./sw/z/ipproto.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	
udpproto.o: ./sw/z/udpproto.c ./sw/z/udpproto.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

reset_handler: 
	$(CROSS)gcc $(CROSSCFLAGS) -c ./sw/Reset_Handler.S -o ./sw/Reset_Handler.o



$(OUTPUT_NAME).o: $(SOURCE) ./sw/z/icmp.h ./sw/z/pkt.h ./sw/z/board.h ./sw/z/ipproto.h ./sw/z/arp.h ./sw/z/protoconst.h ./sw/z/etcnet.h ./sw/z/ethproto.h ./sw/z/ipcksum.h ./sw/z/ipproto.h ./sw/z/udpproto.h ../../riscv_subsystem/sw/utils.h
# $(OUTPUT_NAME).o: $(SOURCE) ../../riscv_subsystem/sw/utils.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<  




$(OUTPUT_NAME).elf:  $(OUTPUT_NAME).o icmp.o pkt.o arp.o ethproto.o ipcksum.o ipproto.o udpproto.o
# $(OUTPUT_NAME).elf:  $(OUTPUT_NAME).o	
	$(CROSS)gcc $(CROSSLDFLAGS) -T $(LINKER) -o $@ ./sw/Reset_Handler.o  $^   ../../riscv_subsystem/sw/muldi3.S ../../riscv_subsystem/sw/div.S -lgcc -lc -lgloss -lc

$(OUTPUT_NAME).hex: $(OUTPUT_NAME).elf
	$(CROSS)objcopy -O verilog $< /dev/stdout > $@
	
.PHONY: dump

dump: $(OUTPUT_NAME).o $(OUTPUT_NAME).elf
	$(CROSS)objdump -S $(OUTPUT_NAME).o | less > $(OUTPUT_NAME).o.dump
	$(CROSS)objdump -S $(OUTPUT_NAME).elf | less > $(OUTPUT_NAME).elf.dump
	
.PHONY: clean
clean:
	-rm *.o
	-rm ./sw/*.o
	-rm *.elf
	-rm *.hex
