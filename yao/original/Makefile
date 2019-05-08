CXX_CFLAGS = -I$(HOME)/local/include  -L$(HOME)/local/lib -Wno-deprecated -D__STDC_LIMIT_MACROS -DFREE_XOR -DRAND_SEED -DGRR -DNDEBUG   -std=c++0x -g #-fsanitize=address         
CXX        = g++
MPI_CXX    = mpicxx

LIBS       = -lpbc -lgmp -lcrypto 
HEADERS    = Algebra.h Bytes.h Circuit.h Env.h GarbledCct.h NetIO.h Prng.h ClawFree.h 
OBJS       = Algebra.o Bytes.o Circuit.o Env.o GarbledCct.o NetIO.o Prng.o Aes.o key_expansion.o interpreter.o
MAINS      = main.cpp YaoBase.cpp  BetterYao.cpp 

all : $(MAINS) $(OBJS)
	#$(CXX) -o battleship $(CXX_CFLAGS)  interpretermain.cpp  interpreter.cpp
	$(CXX) -o evl -DEVL_CODE $(CXX_CFLAGS) $^ $(LIBS)
	$(CXX) -o gen -DGEN_CODE $(CXX_CFLAGS) $^ $(LIBS)

sim : $(MAINS) $(OBJS)
	$(CXX) -o sim $(CXX_CFLAGS) $^ $(LIBS)

test-circuit: test-circuit.cpp $(OBJS)
	$(CXX) $(CXX_CFLAGS) -o test-circuit $(CXX_LFLAGS) $^ $(LIBS)

server : ipserver.cpp Bytes.o Env.o NetIO.o
	$(CXX) -pthread -o server $(CXX_CFLAGS) $(CXX_LFLAGS) $^ $(LIBS)

GarbledCct.o : Algebra.h Bytes.h Circuit.h Env.h Prng.h GarbledCct.h GarbledCct.cpp
	$(CXX) -msse2 $(CXX_CFLAGS) -c GarbledCct.cpp

Env.o : Algebra.h Bytes.h ClawFree.h Circuit.h NetIO.h Env.h Env.cpp 
	$(CXX) $(CXX_CFLAGS) -c Env.cpp

NetIO.o : Bytes.h NetIO.h NetIO.cpp
	$(CXX) $(CXX_CFLAGS) -c NetIO.cpp

Algebra.o: Bytes.h Prng.h Algebra.h Algebra.cpp
	$(CXX) $(CXX_CFLAGS) -c Algebra.cpp

Circuit.o : Bytes.h Circuit.h Circuit.cpp
	$(CXX) $(CXX_CFLAGS) -c Circuit.cpp

Prng.o: Bytes.h Prng.h Prng.cpp
	$(CXX) $(CXX_CFLAGS) -c Prng.cpp

interpreter.o: interpreter.h interpreter.cpp
	$(CXX) $(CXX_CFLAGS) -c interpreter.cpp

Aes.o: Bytes.h Aes.cpp
	$(CXX) $(CXX_CFLAGS) -c Aes.cpp


#Aes.o: Bytes.h Aes.cpp
#	$(CXX) -O3 -c -maes -msse4.2 Aes.cpp -DAESNI

key_expansion.o: key_expansion.s
	$(CXX) -O3 -c key_expansion.galois.s -o key_expansion.o

Bytes.o : Bytes.h Bytes.cpp
	$(CXX) $(CXX_CFLAGS) -c Bytes.cpp

clean :
	rm -f *.o gen evl sim test-circuit server
