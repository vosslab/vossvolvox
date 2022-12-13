### Use this for parallel processing:
#FLAGS=-O3 -w -march=native -mtune=native -fopenmp
### normal single processor, works on Mac
FLAGS=-O3 -w -march=native -mtune=native
#native flag requires gcc v4.2, here is the most basic
#FLAGS=-O1 -w

CC = g++
OBJS = utils-main.o utils-output.o utils-mrc.o

all:	cav chan fsv sol tun vdw vol allchan allexc volnocav twovol 

cavity:	cav
cavities:	cav
channel:	chan
channels:	chan
fsvcalc:	fsv
solvent:	sol
tunnel:	tun
volume:	vol
objects:	utils-main.o utils-output.o utils-mrc.o

cav:	objects cavities.cpp
	$(CC) $(FLAGS) -o Cavities.exe $(OBJS) cavities.cpp
	chmod 755 Cavities.exe
	mkdir -p ../bin
	mv -v Cavities.exe ../bin

chan:	objects channel.cpp
	$(CC) $(FLAGS) -o Channel.exe $(OBJS) channel.cpp
	chmod 755 Channel.exe
	mkdir -p ../bin
	mv -v Channel.exe ../bin

allchan:	objects allchannel.cpp
	$(CC) $(FLAGS) -o AllChannel.exe $(OBJS) allchannel.cpp
	chmod 755 AllChannel.exe
	mkdir -p ../bin
	mv -v AllChannel.exe ../bin

allexc:	objects allchannelexc.cpp
	$(CC) $(FLAGS) -o AllChannelExc.exe $(OBJS) allchannelexc.cpp
	chmod 755 AllChannelExc.exe
	mkdir -p ../bin
	mv -v AllChannelExc.exe ../bin

fsv:	objects fsv_calc.cpp
	$(CC) $(FLAGS) -o FsvCalc.exe $(OBJS) fsv_calc.cpp
	chmod 755 FsvCalc.exe
	mkdir -p ../bin
	mv -v FsvCalc.exe ../bin

sol:	objects solvent.cpp
	$(CC) $(FLAGS) -o Solvent.exe $(OBJS) solvent.cpp
	chmod 755 Solvent.exe
	mkdir -p ../bin
	mv -v Solvent.exe ../bin

tun:	objects tunnel.cpp
	$(CC) $(FLAGS) -o Tunnel.exe $(OBJS) tunnel.cpp
	chmod 755 Tunnel.exe
	mkdir -p ../bin
	mv -v Tunnel.exe ../bin

vdw:	objects vdw.cpp
	$(CC) $(FLAGS) -o VDW.exe $(OBJS) vdw.cpp
	chmod 755 VDW.exe
	mkdir -p ../bin
	mv -v VDW.exe ../bin

vol:	objects volume.cpp
	$(CC) $(FLAGS) -o Volume.exe $(OBJS) volume.cpp
	chmod 755 Volume.exe
	mkdir -p ../bin
	mv -v Volume.exe ../bin

volnocav:	objects volnocav.cpp
	$(CC) $(FLAGS) -o VolumeNoCav.exe $(OBJS) volnocav.cpp
	chmod 755 VolumeNoCav.exe
	mkdir -p ../bin
	mv -v VolumeNoCav.exe ../bin

twovol:	objects twovolnocav.cpp
	$(CC) $(FLAGS) -o TwoVol.exe $(OBJS) twovolnocav.cpp
	chmod 755 TwoVol.exe
	mkdir -p ../bin
	mv -v TwoVol.exe ../bin

frac:	objects fracDim.cpp
	$(CC) $(FLAGS) -o FracDim.exe $(OBJS) fracDim.cpp
	chmod 755 FracDim.exe
	mkdir -p ../bin
	mv -v FracDim.exe ../bin

cust:	objects customvolume.cpp
	$(CC) $(FLAGS) -o Custom.exe $(OBJS) customvolume.cpp
	chmod 755 Custom.exe
	mkdir -p ../bin
	mv -v Custom.exe ../bin

utils-main.o:	utils-main.cpp
	$(CC) $(FLAGS) -c -o utils-main.o utils-main.cpp

utils-output.o:	utils-main.o utils-output.cpp
	$(CC) $(FLAGS) -c -o utils-output.o utils-output.cpp

utils-mrc.o:	utils-mrc.cpp
	$(CC) $(FLAGS) -c -o utils-mrc.o utils-mrc.cpp

clean:
	rm -fv *.o  *~

distclean:
	rm -fv *.o ../bin/*.exe *~

none:	
	echo "Please type make xxx, where xxx = cav, cha, fsv, sol, tun, vdw, or vol"
