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

APPS_DIR = ./apps
LIB_DIR  = ./lib

# One directory per app, each with a main.c -- so selecting a build is
# `make APP=<name>`, and adding an app is creating a directory. No mapping
# table and no editing which SOURCE= line is commented out.
SOURCE ?= $(APPS_DIR)/$(APP)/main.c

BUILD_DIR   = build/$(APP)
OUT         = $(BUILD_DIR)/firmware

CONFIG      = rv32i
ABI         = ilp32
LINKER      = ./sw/firmware.ld
CROSS       = riscv32-unknown-elf-
CROSSCFLAGS = -O3 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib -I$(LIB_DIR)
CROSSLDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M

# Shared protocol-stack objects linked into every app, in the exact order the
# original Makefile used -- link order affects code layout, so do not reorder.
LIBOBJS = $(BUILD_DIR)/icmp.o $(BUILD_DIR)/pkt.o $(BUILD_DIR)/arp.o \
          $(BUILD_DIR)/ethproto.o $(BUILD_DIR)/ipcksum.o \
          $(BUILD_DIR)/ipproto.o $(BUILD_DIR)/udpproto.o

# The linker script references ./sw/Reset_Handler.o by that exact path, so it
# is built in place rather than under build/.
RESET_HANDLER = ./sw/Reset_Handler.o

APP_HEADERS = $(wildcard $(LIB_DIR)/*.h) ../../riscv_subsystem/sw/utils.h

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
	@if [ ! -f "$(SOURCE)" ]; then \
		echo "error: no app '$(APP)' ($(SOURCE) does not exist)."; \
		echo "Run 'make list-apps' for valid names."; exit 1; \
	fi

.PHONY: list-apps
list-apps:
	@echo "Available APP names (default: vebpf_firewall_syn):"
	@for d in $(APPS_DIR)/*/; do \
		[ -f "$$d/main.c" ] && printf "  %s\n" "$$(basename $$d)"; \
	done

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(RESET_HANDLER): ./sw/Reset_Handler.S
	$(CROSS)gcc $(CROSSCFLAGS) -c ./sw/Reset_Handler.S -o $@

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c $(LIB_DIR)/%.h | $(BUILD_DIR)
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
