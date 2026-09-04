# CLAUDE.md — RISCV_C_FW_VebpfManyCore

Guidance for Claude Code sessions working in this repo.

## What this repo is

Bare-metal C firmware for the RISC-V soft-core that runs the **control plane**
of the VeBPF many-core packet-processing architecture. The VeBPF array handles
the data plane in hardware; this firmware arms packet reception, reads packet
descriptors and VeBPF verdicts over MMIO, and drives the experiment/debug paths.

**This repo is public.** `VebpfManyCore` and `disl-core` are currently private.
Do not add developer home-directory paths, credentials, or unreleased content.

## Build

```console
cd fw/vebpf_network_packet_processing
make list-apps                          # every app + its default flag values
make APP=vebpf_firewall_syn              # build one app
make APP=vebpf_firewall_syn DEBUG=1      # override a build flag
export PATH=/opt/riscv/bin:$PATH         # riscv32-unknown-elf-* lives here
```

One `make APP=<name>` emits **every** consumable format together, so they can
never disagree about which compile they came from:

| Output | Format | Consumer |
|---|---|---|
| `build/<app>/firmware.hex` | `objcopy -O verilog` | UART upload (`load.py`) |
| `build/<app>/firmware.sim.hex` | one 32-bit word per line | Verilog `$readmemh` — simulation **and** synthesis BRAM preload |
| `build/<app>/firmware.vivado.hex` | one byte per line, LSB first | Vivado-specific loader |
| `build/<app>/firmware.elf` / `.map` | ELF / linker map | `objdump`, `addr2line` |

`disl-core` consumes `firmware.sim.hex` via a `MEM_INIT` block in the consuming
example's `system.tml`, and builds this repo via a `[PREBUILD]` step — so
`./vebpf-mc sim` in the parent repo compiles this firmware automatically.
Nothing here needs to know about that.

## Layout

```
fw/vebpf_network_packet_processing/
├── Makefile              APP=<name> selection; emits all formats in one build
├── apps/<name>/
│   ├── main.c            the application
│   └── app.mk            that app's SIMULATION_TESTING / DEBUG defaults
├── lib/                  shared protocol stack -- GPL-3.0, see LICENSE
├── tools/hexconv.py      objcopy-verilog -> $readmemh / Vivado converter
├── sw/                   linker script + reset handler
├── archive/              superseded sources, NOT built
│   └── reference_images/ prebuilt hex from before the build was reproducible
└── build/<app>/          output (gitignored)
riscv_subsystem/          shared boot asm, memory map, address_map.h
```

Adding an app = creating `apps/<name>/main.c` + `app.mk`. There is no mapping
table to update and no `SOURCE=` line to comment or uncomment.

## The two build flags

| Flag | Effect |
|---|---|
| `SIMULATION_TESTING` | `1` for simulation, `0` for synthesis. At `1`, error `printf()` output is suppressed and errors signal only via error LED = 5 |
| `DEBUG` | `1` compiles in `printf()` error reporting **as well as** the LED. Set on a **synthesis** build to watch Ethernet packet processing over the debug UART on real hardware |

Declared per-app in `apps/<name>/app.mk`, overridable on the command line,
fallback values in `lib/board.h` behind `#ifndef` guards. A `.build_flags` stamp
forces a full rebuild when a value changes, so objects compiled with different
flags can never mix.

> **Why this matters.** These were plain hand-edited `#define`s in `lib/board.h`
> until 2026-08-31. Because every app shared one setting,
> `riscv_firewall_throughput_sim` and `riscv_firewall_throughput_syn` — whose
> sources are **byte-identical**, these two flags being the only thing that ever
> distinguished them — built identical firmware and their names meant nothing.

## Regression gates

| Gate | Expected |
|---|---|
| Reference build | `make APP=vebpf_firewall_syn` → `firmware.hex` md5 **`9e49b0a414e1b942d08fb2c4be3896d9`** (also matches the committed `archive/reference_images/..._0SIM_0DEBUG_SYN.hex`) |
| All apps | all 9 listed apps compile **and link** |
| Converter | `tools/hexconv.py <uart.hex> out --mode sim` on `archive/reference_images/..._1SIM_0DEBUG.hex` → md5 **`b05b89331d2bf298c5d9f575885dccb8`** |
| Downstream | parent repo's simulation still gives `sim_time_ns = 3131136.001` |

## Things to look out for

1. **`lib/` is GPL-3.0** (Gisselquist Technology / ZipVersa), 23 files, each with
   its own header. The repo's own work is MIT. **Firmware binaries that link
   `lib/` are subject to GPL-3.0.** `lib/board.h` and `lib/mmio.h` carry no GPL
   header and are original work; `lib/irq.c` is public domain;
   `riscv_subsystem/sw/{div,muldi3}.S` and `riscv-asm.h` are GPL-3.0 **with the
   GCC Runtime Library Exception**. `LICENSE` has the verified file-by-file
   breakdown — do not simplify it.

2. **`archive/` is not built.** Nine older apps there no longer link against the
   current `lib/pkt.c` (it calls `write_led()`/`tx_busy()`, defined only in the
   newer apps) and `pingtest.c` does not compile. Only list an app in the
   Makefile if it actually builds.

3. **`archive/reference_images/` holds a real reproducibility gap.** The `_SYN`
   image rebuilds byte-identically from current source; the `_SIM` image does
   **not**. Same toolchain, linker and flags. Four inputs are known to affect the
   bytes — `SIMULATION_TESTING`, `DEBUG`, the linker `LENGTH`, and the
   stack-pointer literal in `sw/Reset_Handler.S` (`li sp,65536`) — and a full
   sweep of all four reproduces none of them, so at least one *other* input
   changed (likely a `lib/` source or the compiler version). **Practical impact
   is nil**: nothing uses those files; simulation builds its own. Do not delete
   them without recording the outcome.

4. **`sw/firmware.ld` and `sw/Reset_Handler.S` are a matched pair**, as are the
   copies under `sw/linker_firmwareLd_resetHandler_for_256MB_DDR/`. The linker
   script sets `LENGTH`; the reset handler hardcodes the stack top as an
   immediate. Changing BRAM depth means changing **both**. Note the Makefile
   always assembles `./sw/Reset_Handler.S` regardless of which `LINKER=` you
   pass — so passing a different linker script alone changes nothing.

5. **The linker script references `./sw/Reset_Handler.o` by that exact path**, so
   that object is built in place rather than under `build/`. Do not "tidy" it
   into the build directory.

6. **`make clean` removes only `build/`** and `sw/*.o`. It used to `rm *.hex`,
   which deleted committed artifacts — do not reintroduce that.

7. **This repo is a submodule and sits on DETACHED HEAD** after
   `git submodule update`. See the parent's `CLAUDE.md` before pushing.

8. **Deferred, deliberately**: unifying the three UART *upload* scripts
   (`load.py` and friends) — they still hardcode filenames in their bodies; only
   the format *conversion* was extracted into `tools/hexconv.py`. And
   parameterizing `MEM_LEN` in the linker/reset-handler, which would remove the
   duplicate `sw/linker_firmwareLd_resetHandler_for_256MB_DDR/` directory.

9. **`lib/board.h` and `lib/mmio.h` both define peripheral addresses** and
   disagree in meaning at one: `riscv_subsystem/config/address_map.h` calls
   `0x1000001C` `TIMER`, `mmio.h` calls it `I2C`. Not currently harmful; worth
   reconciling.

## Related

- [VebpfManyCore](https://github.com/zaidtahirbutt/VebpfManyCore) — the full system (has its own `CLAUDE.md`)
- [VeBPF](https://github.com/zaidtahirbutt/VeBPF) — the eBPF CPU core
