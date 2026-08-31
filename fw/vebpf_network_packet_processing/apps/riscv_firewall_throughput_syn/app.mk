# Build configuration for the 'riscv_firewall_throughput_syn' app.
# Overridable on the command line: make APP=riscv_firewall_throughput_syn DEBUG=1
#
# Synthesis build. Source is byte-identical to the _sim app; these flags are the ONLY difference.
SIMULATION_TESTING ?= 0
DEBUG              ?= 0
