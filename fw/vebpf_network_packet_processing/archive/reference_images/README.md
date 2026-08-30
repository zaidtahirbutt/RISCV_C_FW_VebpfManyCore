# Reference firmware images

These are prebuilt firmware images committed before this repository gained a
reproducible build. They are **not** used by any current flow — simulation and
synthesis now build their own via `make APP=<name>` and reference the result
through a `MEM_INIT` block in the consuming project's `system.tml`.

They are kept because of an unresolved reproducibility gap, documented here
rather than quietly dropped:

| Image | Rebuilds byte-identically from current source? |
|---|---|
| `..._0SIM_0DEBUG_SYN.hex` | **Yes** — `make APP=vebpf_firewall_syn` reproduces it exactly (verified by md5) |
| `..._1SIM_0DEBUG.hex` | **No** — `make APP=vebpf_firewall_sim` produces a different image |
| `..._1SIM_0DEBUG_SIM.hex` | **No** — the `$readmemh`-format conversion of the above |
| `2024_1_24_..._throughput_cal_SYN.hex` | not checked |

The SYN pair reproducing exactly while the SIM pair does not, using the same
toolchain, linker script and flags, means the toolchain is not the variable —
the committed SIM images predate some change to their source or build inputs
that was never recorded. The gap has no practical effect today: simulation
with freshly-built firmware passes and yields `sim_time_ns = 3131136.001`,
identical to every run made against the committed image.

Delete these once the discrepancy is either explained or judged irrelevant.
