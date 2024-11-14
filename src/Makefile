# Makefile for Voss Volvox Project with Conditional OpenMP Support
# This file is used to compile various programs related to molecular structure analysis

# Compiler and compilation flags
CC = g++                    # Set the compiler to g++
BASE_FLAGS = -O3 -w         # Basic optimization and quiet mode flags
CPU_FLAGS = -march=native -mtune=native  # CPU-specific optimizations for current architecture
OPENMP_FLAGS = -fopenmp     # OpenMP flag for parallel processing
FLAGS = $(BASE_FLAGS) $(CPU_FLAGS)       # Default to base flags; OpenMP will be added if supported

# Directory to store binaries
BIN_DIR = ../bin

# Check if the compiler supports OpenMP
ifeq ($(shell $(CC) -dM -E - < /dev/null | grep -q __OPENMP; echo $$?),0)
	FLAGS += $(OPENMP_FLAGS)
	@echo "OpenMP support detected; using -fopenmp flag."
else
	@echo "OpenMP not supported; proceeding without -fopenmp."
endif

# Object files used in all programs
OBJS = utils-main.o utils-output.o utils-mrc.o

# The default target, which builds all programs
all: cav chan allchan allexc fsv sol tun vdw vol volnocav twovol frac cust

# Clean target to remove all compiled binaries and object files
.PHONY: clean
clean:
	rm -fv *.o  *~
	@echo "Object files and temporary files have been removed."

# Distclean target for additional cleanup, including binaries
.PHONY: distclean
distclean: clean
	rm -fv $(BIN_DIR)/*.exe
	@echo "All binaries in $(BIN_DIR)/ have been removed."

# No target to remind the user to specify a target
.PHONY: none
none:
	@echo "Please type make <target>, where <target> is one of the following:"
	@echo "cav, chan, allchan, allexc, fsv, sol, tun, vdw, vol, volnocav, twovol, frac, or cust."

# Compilation rules for individual object files
utils-main.o: utils-main.cpp
	$(CC) $(FLAGS) -c -o $(BIN_DIR)/utils-main.o utils-main.cpp

utils-output.o: utils-main.o utils-output.cpp
	$(CC) $(FLAGS) -c -o $(BIN_DIR)/utils-output.o utils-output.cpp

utils-mrc.o: utils-mrc.cpp
	$(CC) $(FLAGS) -c -o $(BIN_DIR)/utils-mrc.o utils-mrc.cpp

# Individual program targets
cav: $(OBJS) cavities.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/Cavities.exe $(OBJS) cavities.cpp
	@chmod +x $(BIN_DIR)/Cavities.exe

chan: $(OBJS) channel.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/Channel.exe $(OBJS) channel.cpp
	@chmod +x $(BIN_DIR)/Channel.exe

allchan: $(OBJS) allchannel.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/AllChannel.exe $(OBJS) allchannel.cpp
	@chmod +x $(BIN_DIR)/AllChannel.exe

allexc: $(OBJS) allchannelexc.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/AllChannelExc.exe $(OBJS) allchannelexc.cpp
	@chmod +x $(BIN_DIR)/AllChannelExc.exe

fsv: $(OBJS) fsv_calc.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/FsvCalc.exe $(OBJS) fsv_calc.cpp
	@chmod +x $(BIN_DIR)/FsvCalc.exe

sol: $(OBJS) solvent.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/Solvent.exe $(OBJS) solvent.cpp
	@chmod +x $(BIN_DIR)/Solvent.exe

tun: $(OBJS) tunnel.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/Tunnel.exe $(OBJS) tunnel.cpp
	@chmod +x $(BIN_DIR)/Tunnel.exe

vdw: $(OBJS) vdw.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/VDW.exe $(OBJS) vdw.cpp
	@chmod +x $(BIN_DIR)/VDW.exe

vol: $(OBJS) volume.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/Volume.exe $(OBJS) volume.cpp
	@chmod +x $(BIN_DIR)/Volume.exe

volnocav: $(OBJS) volnocav.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/VolumeNoCav.exe $(OBJS) volnocav.cpp
	@chmod +x $(BIN_DIR)/VolumeNoCav.exe

twovol: $(OBJS) twovolnocav.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/TwoVol.exe $(OBJS) twovolnocav.cpp
	@chmod +x $(BIN_DIR)/TwoVol.exe

frac: $(OBJS) fracDim.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/FracDim.exe $(OBJS) fracDim.cpp
	@chmod +x $(BIN_DIR)/FracDim.exe

cust: $(OBJS) customvolume.cpp
	$(CC) $(FLAGS) -o $(BIN_DIR)/Custom.exe $(OBJS) customvolume.cpp
	@chmod +x $(BIN_DIR)/Custom.exe

