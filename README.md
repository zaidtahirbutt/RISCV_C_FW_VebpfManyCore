# RISCV_C_FW_VebpfManyCore

C firmware for the RISC-V soft-core that runs the **control plane** of the
[VeBPF many-core](https://github.com/zaidtahirbutt/VebpfManyCore)
packet-processing architecture on FPGA-based SmartNICs and IoT devices.

The VeBPF many-core array handles the *data plane* — evaluating eBPF rules
against packet headers at line rate, in hardware. This repository is the
*management plane* that sits beside it: a small bare-metal RISC-V program that
arms packet reception, reads packet descriptors and VeBPF verdicts over MMIO,
and drives the experiment/debug paths.

## Where this fits

```
   Host ──UART──▶ eBPF rules ──▶ VeBPF many-core array   (data plane, hardware)
                                          │ verdicts
                                          ▼
   Host ──UART──▶ THIS FIRMWARE ──▶ RISC-V soft-core     (control plane, software)
```

Two independent flows consume what this repository builds:

| Flow | File used | How it gets there |
|---|---|---|
| **Simulation / synthesis** | `build/<app>/firmware.sim.hex` | Named by a `MEM_INIT` block in the consuming project's `system.tml`, resolved and loaded via Verilog `$readmemh` |
| **Real hardware** | `build/<app>/firmware.hex` | Uploaded over UART to an already-flashed board (`load.py`) |

Both come out of a **single** `make APP=<name>`, so they can never disagree
about which compile they came from.

## Prerequisites

A bare-metal RISC-V toolchain providing `riscv32-unknown-elf-gcc`
(rv32i / ilp32), plus Python 3 for the format converter. Pick one:

**Build from source** (what this project is developed against):
```console
git clone https://github.com/riscv-collab/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --with-arch=rv32i --with-abi=ilp32
make -j$(nproc)          # plain 'make', NOT 'make linux' -- this firmware is freestanding
export PATH=/opt/riscv/bin:$PATH
```

**Prebuilt (xPack), any platform** — binaries are prefixed `riscv-none-elf-`:
```console
# download from https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases
make APP=<name> CROSS=riscv-none-elf-
```

**Debian/Ubuntu** — multilib, supports rv32i/ilp32 despite the `riscv64` name:
```console
sudo apt-get install gcc-riscv64-unknown-elf
make APP=<name> CROSS=riscv64-unknown-elf-
```

Verify with `riscv32-unknown-elf-gcc --version`.

## Build

```console
cd fw/vebpf_network_packet_processing
make list-apps            # show every available app
make APP=vebpf_firewall_sim
```

Outputs land in `build/<app>/`:

| File | Format | Consumer |
|---|---|---|
| `firmware.hex` | `objcopy -O verilog` (`@addr` + little-endian bytes) | UART upload via `load.py` |
| `firmware.sim.hex` | one 32-bit word per line | Verilog `$readmemh` — cocotb simulation **and** synthesis BRAM preload |
| `firmware.vivado.hex` | one byte per line, LSB first | Vivado-specific loader |
| `firmware.elf` / `.map` | ELF / linker map | `objdump`, `addr2line`, link inspection |

`make clean` removes only `build/`. Adding a new app is creating
`apps/<name>/main.c` — there is no mapping table to update and no `SOURCE=`
line to comment or uncomment.

## Applications

All nine compile and link cleanly.

| App | What it does |
|---|---|
| `vebpf_firewall_syn` | VeBPF many-core firewall control plane, **hardware** build (the tested-on-hardware app) |
| `vebpf_firewall_sim` | Same, simulation build (extra instrumentation, `SIM=1`) |
| `riscv_firewall_throughput_syn` | RISC-V-only firewall — the baseline the VeBPF many-core is measured against |
| `riscv_firewall_throughput_sim` | Same, simulation build |
| `vebpf_throughput_cal` | VeBPF throughput measurement harness |
| `tx_pipeline` | Transmit-pipeline exercise (IPv4 checksum path) |
| `led_hello_world` | Minimal GPIO smoke test — start here to prove your toolchain and board work |
| `simple_pingtest_loop` | ICMP echo loop |
| `simple_udp_loop` | UDP echo loop |

## Layout

```
fw/vebpf_network_packet_processing/
├── Makefile              APP=<name> selection; emits every format in one build
├── apps/<name>/main.c    one directory per firmware application
├── lib/                  shared protocol stack (GPL-3.0 -- see LICENSE)
├── tools/hexconv.py      objcopy-verilog -> $readmemh / Vivado format converter
├── sw/                   linker script + reset handler
├── archive/              superseded sources, kept for provenance, NOT built
└── build/<app>/          output (gitignored)
riscv_subsystem/          shared boot asm + memory map (div.S/muldi3.S: rv32i has no M extension)
```

`archive/` holds nine older applications that were written against an earlier
revision of `lib/pkt.c` and no longer link against the current one (it calls
`write_led()` / `tx_busy()`, which only the newer apps define); `pingtest.c`
additionally fails to compile. They are kept for reference rather than
advertised as buildable.

## License

MIT for this repository's own work — **but the shared protocol stack in
`lib/` is GPL-3.0** (Gisselquist Technology / ZipVersa), so firmware binaries
that link it are subject to GPL-3.0 terms. See [`LICENSE`](LICENSE) for the
full file-by-file breakdown.

## Related

- [VebpfManyCore](https://github.com/zaidtahirbutt/VebpfManyCore) — the full many-core system
- [VeBPF](https://github.com/zaidtahirbutt/VeBPF) — the eBPF-ISA-compliant CPU core

## Citation

> Z. Tahir, S. Bandara, M. Herbordt, A. Sanaullah, U. Drepper, "VeBPF Many-Core
> Architecture for Network Functions in FPGA-based SmartNICs and IoT,"
> *IEEE High Performance Extreme Computing Conference (HPEC)*, 2024.

## Contact

Zaid Tahir — CAAD Lab, ECE, Boston University
