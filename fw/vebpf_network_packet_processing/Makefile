# RISC-V control-plane firmware for the VeBPF many-core system.
#
#   make APP=<name>      build one application
#   make list-apps       show every available APP name and its source file
#   make clean           remove build/ only (never touches tracked sources)
#
# Every build writes all four consumable formats in ONE invocation, under
# build/$(APP)/, so they can never disagree about which compile they came
# from:
#
#   build/$(APP)/firmware.hex         objcopy -O verilog  -> UART upload (load.py)
#   build/$(APP)/firmware.sim.hex     one word per line   -> Verilog $readmemh
#                                     (cocotb simulation AND synthesis BRAM preload)
#   build/$(APP)/firmware.vivado.hex  one byte per line   -> Vivado-specific loader
#   build/$(APP)/firmware.elf/.map    for objdump/addr2line and link inspection
#
# `firmware.sim.hex` is the file `system.tml`'s MEM_INIT points at; see
# disl-core's memory-init mechanism.

APP ?= vebpf_firewall_syn

SRC_DIR = ./sw/z

# ---------------------------------------------------------------------------
# Named apps -> source files. Selecting a build is `make APP=<name>`, never
# editing which line is commented out.
#
# Every app listed here is verified to compile AND link cleanly. Nine older
# sources (simple_icmp_loop, gettysburg, job, pingtest, test.c, test2.c,
# test3-5.c) are deliberately NOT listed: they were written against an older
# pkt.c and no longer link against the current one, which calls write_led()
# and tx_busy() that only the newer apps define (pingtest.c additionally
# fails to compile). They are kept under archive/ for provenance rather than
# advertised as buildable apps.
# ---------------------------------------------------------------------------
APP_SRC_vebpf_firewall_syn            = $(SRC_DIR)/2024_9_12_edgetestbed_a100T30_VeBPF_pgmLoaderV2_firewallTYPE1combinedSim_reducedLinker_0SIM_0DEBUG_SYN.c
APP_SRC_vebpf_firewall_sim            = $(SRC_DIR)/2024_9_12_edgetestbed_a100T30_VeBPF_pgmLoaderV2_firewallTYPE1combinedSim_reducedLinker_1SIM_0DEBUG.c
APP_SRC_riscv_firewall_throughput_syn = $(SRC_DIR)/2024_1_24_edgetestbed_a100T26_RISCV_throughput_cal_firewallTYPE1_v5_v2_onlyHdr_TYPE4_Exp_throughput_cal_SYN.c
APP_SRC_riscv_firewall_throughput_sim = $(SRC_DIR)/2024_1_24_edgetestbed_a100T26_RISCV_throughput_cal_firewallTYPE1_v5_v2_onlyHdr_TYPE4_Exp_throughput_cal_SIM.c
APP_SRC_vebpf_throughput_cal          = $(SRC_DIR)/2024_1_11_edgetestbed_a100T26_VeBPF_throughput_cal_firewallTYPE1_v5_TYPE4_Exp_SYN.c
APP_SRC_tx_pipeline                   = $(SRC_DIR)/2024_5_16_edgetestbed_a100T27_tx_pipeline_v15_SYN_reducedLinker_0SIM_1DEBUG_0UDPcksum_1IPv4Cksum_SYN.c
APP_SRC_led_hello_world               = $(SRC_DIR)/2024_10_13_simpeLedHIGH.c
APP_SRC_simple_pingtest_loop          = $(SRC_DIR)/simple_pingtest_loop.c
APP_SRC_simple_udp_loop               = $(SRC_DIR)/simple_udp_loop.c

# `SOURCE=<path>` still works as a raw escape hatch for a one-off file.
SOURCE ?= $(APP_SRC_$(APP))

BUILD_DIR   = build/$(APP)
OUT         = $(BUILD_DIR)/firmware

CONFIG      = rv32i
ABI         = ilp32
LINKER      = ./sw/firmware.ld
CROSS       = riscv32-unknown-elf-
CROSSCFLAGS = -O3 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib
CROSSLDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M

# Shared protocol-stack objects linked into every app, in the exact order the
# original Makefile used -- link order affects code layout, so do not reorder.
LIBOBJS = $(BUILD_DIR)/icmp.o $(BUILD_DIR)/pkt.o $(BUILD_DIR)/arp.o \
          $(BUILD_DIR)/ethproto.o $(BUILD_DIR)/ipcksum.o \
          $(BUILD_DIR)/ipproto.o $(BUILD_DIR)/udpproto.o

# The linker script references ./sw/Reset_Handler.o by that exact path, so it
# is built in place rather than under build/.
RESET_HANDLER = ./sw/Reset_Handler.o

APP_HEADERS = $(SRC_DIR)/icmp.h $(SRC_DIR)/pkt.h $(SRC_DIR)/board.h \
              $(SRC_DIR)/ipproto.h $(SRC_DIR)/arp.h $(SRC_DIR)/protoconst.h \
              $(SRC_DIR)/etcnet.h $(SRC_DIR)/ethproto.h $(SRC_DIR)/ipcksum.h \
              $(SRC_DIR)/udpproto.h ../../riscv_subsystem/sw/utils.h

.PHONY: all
all: check-app $(OUT).hex $(OUT).sim.hex $(OUT).vivado.hex
	@echo ""
	@echo "APP=$(APP) built from $(SOURCE)"
	@echo "  $(OUT).hex          (UART upload / load.py)"
	@echo "  $(OUT).sim.hex      (\$$readmemh: cocotb sim + synthesis BRAM preload)"
	@echo "  $(OUT).vivado.hex   (Vivado byte-order loader)"
	@echo "  $(OUT).elf/.map     (objdump / addr2line)"

.PHONY: check-app
check-app:
	@if [ -z "$(SOURCE)" ]; then \
		echo "error: unknown APP '$(APP)'. Run 'make list-apps' for valid names."; exit 1; \
	fi
	@if [ ! -f "$(SOURCE)" ]; then \
		echo "error: APP '$(APP)' maps to '$(SOURCE)', which does not exist."; exit 1; \
	fi

.PHONY: list-apps
list-apps:
	@echo "Available APP names (default: vebpf_firewall_syn):"
	@$(foreach v,$(sort $(filter APP_SRC_%,$(.VARIABLES))), \
		printf "  %-34s %s\n" "$(patsubst APP_SRC_%,%,$(v))" "$($(v))";)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(RESET_HANDLER): ./sw/Reset_Handler.S
	$(CROSS)gcc $(CROSSCFLAGS) -c ./sw/Reset_Handler.S -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h | $(BUILD_DIR)
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@ $<

$(OUT).o: $(SOURCE) $(APP_HEADERS) | $(BUILD_DIR)
	$(CROSS)gcc $(CROSSCFLAGS) -c -o $@ $<

# pkt.c depended on firmware.o in the original Makefile; preserved so the
# build graph is unchanged.
$(BUILD_DIR)/pkt.o: $(OUT).o

$(OUT).elf: $(OUT).o $(LIBOBJS) $(RESET_HANDLER)
	$(CROSS)gcc $(CROSSLDFLAGS) -T $(LINKER) -o $@ $(RESET_HANDLER) \
		$(OUT).o $(LIBOBJS) \
		../../riscv_subsystem/sw/muldi3.S ../../riscv_subsystem/sw/div.S \
		-lgcc -lc -lgloss -lc > $(OUT).map

$(OUT).hex: $(OUT).elf
	$(CROSS)objcopy -O verilog $< /dev/stdout > $@

$(OUT).sim.hex: $(OUT).hex
	python3 tools/hexconv.py $< $@ --mode sim

$(OUT).vivado.hex: $(OUT).hex
	python3 tools/hexconv.py $< $@ --mode vivado

.PHONY: dump
dump: $(OUT).elf
	$(CROSS)objdump -S $(OUT).o   > $(OUT).o.dump
	$(CROSS)objdump -S $(OUT).elf > $(OUT).elf.dump

.PHONY: clean
clean:
	-rm -rf build
	-rm -f ./sw/*.o

.PHONY: clean-app
clean-app:
	-rm -rf $(BUILD_DIR)
