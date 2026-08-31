# Build configuration for the 'riscv_firewall_throughput_sim' app.
# Overridable on the command line: make APP=riscv_firewall_throughput_sim DEBUG=1
#
# Simulation build. Source is byte-identical to the _syn app; these flags are the ONLY difference.
SIMULATION_TESTING ?= 1
DEBUG              ?= 0
