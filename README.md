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

## Build configuration: `SIMULATION_TESTING` and `DEBUG`

Two compile-time flags decide what a build actually does:

| Flag | Effect |
|---|---|
| `SIMULATION_TESTING` | `1` for simulation, `0` for synthesis. When `1`, error `printf()` output is suppressed and errors are signalled only by setting error LED = 5. |
| `DEBUG` | `1` compiles in `printf()` error reporting **and** the LED signalling. Set this on a **synthesis** build to watch Ethernet packet processing over the debug UART on real hardware. |

Each app declares its own values in `apps/<name>/app.mk`, and any build can
override them:

```console
make APP=vebpf_firewall_syn                  # the app's own defaults
make APP=vebpf_firewall_syn DEBUG=1          # + packet debug output over UART
make APP=vebpf_firewall_sim SIMULATION_TESTING=0
```

`make list-apps` prints each app's current values. Changing a flag forces a
full rebuild of that app, so a build can never mix objects compiled with
different flags.

> **Why this is a build flag and not an edit.** Both flags used to be plain
> `#define`s in `lib/board.h`, edited by hand before each build. That meant
> every app shared one setting, and which setting a given hex file had been
> built with survived only in that file's name (`..._1SIM_0DEBUG.c`).
> `riscv_firewall_throughput_sim` and `riscv_firewall_throughput_syn` have
> **byte-identical sources** — these two flags were the only thing that ever
> distinguished them — so under a single shared setting they built identically
> and their names meant nothing. `lib/board.h` still holds the fallback values,
> now behind `#ifndef` guards.

## Applications

All nine compile and link cleanly. `SIM`/`DBG` are the app's default flag
values (see above).

| App | SIM | DBG | What it does |
|---|:---:|:---:|---|
| `vebpf_firewall_syn` | 0 | 0 | VeBPF many-core firewall control plane, **hardware** build (the tested-on-hardware app) |
| `vebpf_firewall_sim` | 1 | 0 | Same, simulation build |
| `riscv_firewall_throughput_syn` | 0 | 0 | RISC-V-only firewall — the baseline the VeBPF many-core is measured against |
| `riscv_firewall_throughput_sim` | 1 | 0 | Same source, simulation flags |
| `vebpf_throughput_cal` | 0 | 0 | VeBPF throughput measurement harness |
| `tx_pipeline` | 0 | 1 | Transmit-pipeline exercise (IPv4 checksum path), packet debug enabled |
| `led_hello_world` | 0 | 0 | Minimal GPIO smoke test — start here to prove your toolchain and board work |
| `simple_pingtest_loop` | 0 | 0 | ICMP echo loop |
| `simple_udp_loop` | 0 | 0 | UDP echo loop |

## Layout

```
fw/vebpf_network_packet_processing/
├── Makefile              APP=<name> selection; emits every format in one build
├── apps/<name>/main.c    one directory per firmware application
│   └── app.mk            that app's SIMULATION_TESTING / DEBUG defaults
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
