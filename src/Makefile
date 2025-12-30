# Makefile for Voss Volvox Project with Conditional OpenMP Support
# This file is used to compile various programs related to molecular structure analysis

# Set vol as the default target
.DEFAULT_GOAL := vol

# Compiler and compilation flags
# Set the compiler to g++
CC = g++
# Basic optimization flag
BASE_FLAGS = -O3
# CPU-specific optimizations for current architecture
CPU_FLAGS = -march=native -mtune=native
# OpenMP flag for parallel processing
OPENMP_FLAGS = -fopenmp
# Default to base flags plus local include paths; OpenMP will be added if supported
INCLUDE_FLAGS = -Ilib
FLAGS = $(strip $(BASE_FLAGS) $(CPU_FLAGS) $(INCLUDE_FLAGS))
STD_FLAG = -std=c++17

# Setup target to configure flags based on OpenMP support
setup:
ifeq ($(shell $(CC) -dM -E - < /dev/null | grep -q __OPENMP; echo $$?),0)
	FLAGS += $(OPENMP_FLAGS)
	@echo "OpenMP support detected; using -fopenmp flag."
else
	@echo "OpenMP not supported; proceeding without -fopenmp."
endif

# Directory to store binaries
BIN_DIR = ../bin

# Ensure the bin directory exists before building anything
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Object files used in all programs
OBJS = utils-main.o utils-output.o utils-mrc.o argument_helper.o pdb_io.o
LEGACY_OBJS = utils-main-legacy.o utils-output-legacy.o utils-mrc-legacy.o

# Ensure the bin directory exists before building object files
$(OBJS) $(LEGACY_OBJS): $(BIN_DIR)

# The default target, which builds all programs
all: $(BIN_DIR) cav chan allchan allexc fsv sol tun vdw vol volnocav twovol frac cust pdbxyzr

# Clean target to remove all compiled binaries and object files
.PHONY: clean
clean:
	rm -fv *.o  *~
	@echo "Object files and temporary files have been removed."

# Distclean target for additional cleanup, including binaries
.PHONY: distclean
distclean: clean
	rm -fvr $(BIN_DIR)/
	@echo "All binaries and $(BIN_DIR)/ have been removed."

# No target to remind the user to specify a target
.PHONY: none
none:
	@echo "Please type make <target>, where <target> is one of the following:"
	@echo "cav, chan, allchan, allexc, fsv, sol, tun, vdw, vol, volnocav, twovol, frac, or cust."

# Declare vol and test as PHONY targets
.PHONY: vol test testhelp


# Compilation rules for individual object files
utils-main.o: lib/utils-main.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o utils-main.o lib/utils-main.cpp

utils-main-legacy.o: lib/utils-main-legacy.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o utils-main-legacy.o lib/utils-main-legacy.cpp

utils-output.o: utils-main.o lib/utils-output.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o utils-output.o lib/utils-output.cpp

utils-output-legacy.o: utils-main-legacy.o lib/utils-output.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o utils-output-legacy.o lib/utils-output.cpp

utils-mrc.o: lib/utils-mrc.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o utils-mrc.o lib/utils-mrc.cpp

utils-mrc-legacy.o: lib/utils-mrc.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o utils-mrc-legacy.o lib/utils-mrc.cpp

argument_helper.o: lib/argument_helper.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -c -o argument_helper.o lib/argument_helper.cpp

pdb_io.o: lib/pdb_io.cpp lib/pdb_io.h lib/atmtypenumbers_data.h
	$(CC) $(FLAGS) $(STD_FLAG) -c -o pdb_io.o lib/pdb_io.cpp

# Individual program targets
cav: $(OBJS) cavities.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Cavities.exe $(OBJS) cavities.cpp
	@chmod +x $(BIN_DIR)/Cavities.exe

chan: $(OBJS) channel.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Channel.exe $(OBJS) channel.cpp
	@chmod +x $(BIN_DIR)/Channel.exe

allchan: $(OBJS) allchannel.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/AllChannel.exe $(OBJS) allchannel.cpp
	@chmod +x $(BIN_DIR)/AllChannel.exe

allexc: $(OBJS) allchannelexc.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/AllChannelExc.exe $(OBJS) allchannelexc.cpp
	@chmod +x $(BIN_DIR)/AllChannelExc.exe

fsv: $(OBJS) fsv_calc.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/FsvCalc.exe $(OBJS) fsv_calc.cpp
	@chmod +x $(BIN_DIR)/FsvCalc.exe

sol: $(OBJS) solvent.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Solvent.exe $(OBJS) solvent.cpp
	@chmod +x $(BIN_DIR)/Solvent.exe

tun: $(OBJS) tunnel.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Tunnel.exe $(OBJS) tunnel.cpp
	@chmod +x $(BIN_DIR)/Tunnel.exe

vdw: $(OBJS) vdw.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/VDW.exe $(OBJS) vdw.cpp
	@chmod +x $(BIN_DIR)/VDW.exe

vol: $(OBJS) volume.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Volume.exe $(OBJS) volume.cpp
	@chmod +x $(BIN_DIR)/Volume.exe

volume_original: $(LEGACY_OBJS) volume_original.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Volume-legacy.exe $(LEGACY_OBJS) volume_original.cpp
	@chmod +x $(BIN_DIR)/Volume-legacy.exe

volume_reference: $(LEGACY_OBJS) volume_original.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Volume-1.0.exe $(LEGACY_OBJS) volume_original.cpp
	@chmod +x $(BIN_DIR)/Volume-1.0.exe

# Test target depends on the vol target
test: vol
	cd ../tests && ./test_volume.sh

testhelp: all
	cd $(BIN_DIR) && for i in *.exe; do echo $$i; ./$$i -q -h; sleep 0.1; done

volnocav: $(OBJS) volnocav.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/VolumeNoCav.exe $(OBJS) volnocav.cpp
	@chmod +x $(BIN_DIR)/VolumeNoCav.exe

twovol: $(OBJS) twovolnocav.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/TwoVol.exe $(OBJS) twovolnocav.cpp
	@chmod +x $(BIN_DIR)/TwoVol.exe

cust: $(OBJS) rna_protein_volume.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/Custom.exe $(OBJS) rna_protein_volume.cpp
	@chmod +x $(BIN_DIR)/Custom.exe

frac: $(OBJS) fracDim.cpp
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/FracDim.exe $(OBJS) fracDim.cpp
	@chmod +x $(BIN_DIR)/FracDim.exe

pdbxyzr: $(BIN_DIR) pdb_io.o pdb_to_xyzr.cpp lib/pdb_io.h lib/atmtypenumbers_data.h
	$(CC) $(FLAGS) $(STD_FLAG) -o $(BIN_DIR)/pdb_to_xyzr.exe pdb_io.o pdb_to_xyzr.cpp
	@chmod +x $(BIN_DIR)/pdb_to_xyzr.exe
