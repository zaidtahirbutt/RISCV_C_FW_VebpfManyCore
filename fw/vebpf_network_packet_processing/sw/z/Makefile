# I think is the edgetestbed MAKEFILE
#SOURCE	= test.c

#Z addon
#SOURCE	= test2.c
#SOURCE	= test3.c
#SOURCE	= test2_LED_forSim.c
#SOURCE	= z/test2_LED_forSim.c
SOURCE	= simple_icmp_loop.c

#SOURCE	= test4.c
#SOURCE	= test5.c
#SOURCE	= test3_UART_GPIO.c

OUTPUT_NAME = firmware
CONFIG = rv32i
ABI = ilp32
LINKER = firmware.ld
#CROSS = riscv64-linux-gnu-

#Z addon
CROSS = riscv64-unknown-elf-

#This might convert the 64bit version to rv32i I guess
CROSSCFLAGS = -O3 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib


CROSSLDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M

UART_PORT = ttyUSB1


.PHONY: all
all:	$(OUTPUT_NAME).hex




icmp.o: icmp.c icmp.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	
pkt.o: pkt.c pkt.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $^ #$<
	
arp.o: arp.c arp.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

ethproto.o: ethproto.c ethproto.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

ipcksum.o: ipcksum.c ipcksum.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

ipproto.o: ipproto.c ipproto.h
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

#udpproto.o: udpproto.c udpproto.h
#	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	
#txfns.o: txfns.c txfns.h
#	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	
$(SOURCE).o: $(SOURCE) icmp.h pkt.h board.h ipproto.h arp.h protoconst.h etcnet.h ethproto.h ipcksum.h ipproto.h 
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $^ #$<

#$^ means all pre-reqs $< only first pre-req

#$(OUTPUT_NAME).o: $(SOURCE) 
	#$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<
	#$(CROSS)gcc $(CROSSCFLAGS) -c -o $@  $<

#$< refers to first dependancy

$(OUTPUT_NAME).elf: $(SOURCE).o icmp.o pkt.o arp.o ethproto.o ipcksum.o ipproto.o
	$(CROSS)gcc $(CROSSLDFLAGS) -T $(LINKER) -o $@   Reset_Handler.S $^ #$< icmp.o pkt.o arp.o ethproto.o ipcksum.o ipproto.o 
	
    
$(OUTPUT_NAME).hex: $(OUTPUT_NAME).elf 
	$(CROSS)objcopy -O verilog $< /dev/stdout > $@


.PHONY: dump
dump: $(OUTPUT_NAME).o $(OUTPUT_NAME).elf
	$(CROSS)objdump -S $(OUTPUT_NAME).o | less > $(OUTPUT_NAME).o.dump
	$(CROSS)objdump -S $(OUTPUT_NAME).elf | less > $(OUTPUT_NAME).elf.dump
	
.PHONY: load
load: $(OUTPUT_NAME).hex
	python3 load.py $(UART_PORT)
	
.PHONY: clean
clean:
	rm *.o
	rm *.elf
	rm *.hex
	rm *.dump
