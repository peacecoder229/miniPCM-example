all: utils.o pci.o mmio.o msr.o pmu.o imc.o cha.o mem_mon_vm mem_mon

mem_mon_vm: IMC-raw.cpp utils.o pci.o mmio.o msr.o pmu.o imc.o cha.o iio.o
	g++ IMC-raw.cpp utils.o pci.o mmio.o msr.o pmu.o imc.o cha.o iio.o -static -o mem_mon_vm -lpthread

mem_mon: IMC-raw.cpp utils.o pci.o mmio.o msr.o pmu.o imc.o cha.o iio.o
	g++ IMC-raw.cpp utils.o pci.o mmio.o msr.o pmu.o imc.o cha.o iio.o -o mem_mon

utils.o: utils.h utils.cpp global.h types.h
	g++ -c utils.cpp -o utils.o

pci.o: pci.h pci.cpp global.h types.h
	g++ -c pci.cpp -o pci.o

msr.o: msr.h msr.cpp global.h types.h
	g++ -c msr.cpp -o msr.o

pmu.o: pmu.h pmu.cpp global.h types.h
	g++ -c pmu.cpp -o pmu.o

imc.o: imc.h imc.cpp global.h types.h
	g++ -c imc.cpp -o imc.o

cha.o: cha.h cha.cpp global.h types.h
	g++ -c cha.cpp -o cha.o

mmio.o: mmio.h mmio.cpp global.h types.h
	g++ -c mmio.cpp -o mmio.o

iio.o: iio.h iio.cpp global.h types.h
	g++ -c iio.cpp -o iio.o

debug: $(wildcard *.h) $(wildcard *.cpp)
	g++ -g -pg -o debug.x $(wildcard *.cpp)

clean:
	rm -rf *.x *.o *~ *.d *.a *.so

remake: clean all

run:
	./IMC-raw.x -e imc/config=0x000000000000f005,name=UNC_M_CAS_COUNT.WR -e imc/config=0x000000000000cf05,name=UNC_M_CAS_COUNT.RD  -e imc/config=0x0000000000000082,name=UNC_M_WPQ_OCCUPANCY_PCH0 -e imc/config=0x0000000000000080,name=UNC_M_RPQ_OCCUPANCY_PCH0 -d 1 -e cha/config=0x00c001fe00010135,config1=0x20,name=UNC_CHA_TOR_INSERTS.IA_MISS:tid=0x20

cha:
	./IMC-raw.x -e cha/config=0x00c001fe00010135,config1=0x20,name=UNC_CHA_TOR_INSERTS.IA_MISS:tid=0x20

iio:
	./IMC-raw.x -e iio/config=0x0000701000400183,name=UNC_IIO_DATA_REQ_OF_CPU.MEM_WRITE.PART0

run_debug:
	./debug.x -e imc/config=0x000000000000f005,name=UNC_M_CAS_COUNT.WR -e imc/config=0x000000000000cf05,name=UNC_M_CAS_COUNT.RD  -e imc/config=0x0000000000000082,name=UNC_M_WPQ_OCCUPANCY_PCH0 -e imc/config=0x0000000000000080,name=UNC_M_RPQ_OCCUPANCY_PCH0 -d 1 -e cha/config=0x00c001fe00010135,config1=0x20,name=UNC_CHA_TOR_INSERTS.IA_MISS:tid=0x20
	# ./debug.x -e iio/config=0x0000701000400183,name=UNC_IIO_DATA_REQ_OF_CPU.MEM_WRITE.PART0

fixed:
	./IMC-raw.x -e imc/fixed

cha-io-bw:
	./IMC-raw.x -e cha/config=0x00c8f3ff00000435,name=PCIRDCUR -e cha/config=0x00cc43ff00000435,name=ITOM, -e cha/config=0x00cd43ff00000435,name=ITOM_NEAR
iio-bw:
	./IMC-raw.x -e iio/config=0x0007001000000183,name=mem_wr -e iio/config=0x0007001000000483,name=mem_rd
cha-io-bw-iiofree:
	./IMC-raw.x -e cha/config=0x00c8f3ff00000435,name=PCIRDCUR -e cha/config=0x00cc43ff00000435,name=ITOM, -e cha/config=0x00cd43ff00000435,name=ITOM_NEAR -e iio/config=0x0007001000000183

io_rnd_lat:
	./IMC-raw.x -e iio/config=0x000400000000ffd5,name=iio_occ -e iio/config=0x00070ff0000004c2,name=iio_insrt -e iio/config=0x0000000000000001,name=iioclocks -d 1
io_mem_bw:
	./IMC-raw.x -e imc/config=0x000000000000f005,name=UNC_M_CAS_COUNT.WR -e imc/config=0x000000000000cf05,name=UNC_M_CAS_COUNT.RD  -e imc/config=0x0000000000000082,name=UNC_M_WPQ_OCCUPANCY_PCH0 -e imc/config=0x0000000000000080,name=UNC_M_RPQ_OCCUPANCY_PCH0, -e cha/config=0x00c8f3ff00000435,name=PCIRDCUR -e cha/config=0x00cc43ff00000435,name=ITOM  -d 1
cha-lat:
	./IMC-raw.x -e cha/config=0x00c817fe00000136,name=TOROCC -e cha/config=0x00c817fe00000135,name=TORINSRT -e cha/config=0x0000000000000001,name=CHACLKS -d 1
imc_stat:
	./IMC-raw.x -e imc/config=0x000000000000ff05,name=UNC_M_CAS_COUNT.ALL -e imc/config=0x0000000000000101,name=UNC_M_CLOCKTICKS  -e imc/config=0x0000000000000082,name=UNC_M_WPQ_OCCUPANCY_PCH0 -e imc/config=0x0000000000000080,name=UNC_M_RPQ_OCCUPANCY_PCH0, -e cha/config=0x00c8f3ff00000435,name=PCIRDCUR -e cha/config=0x00cc43ff00000435,name=ITOM  -d 1
