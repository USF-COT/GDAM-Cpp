# Makefile for Glider Database Alternative with Mongo (GDAM) Daemon
# By: Michael Lindemuth

OUTPUT=GDAM

CC=gcc
LD=g++
CSRCS = $(wildcard *.c) 
CPPSRCS += $(wildcard *.cpp)
COBJS = $(patsubst %.c,objs/%.o,$(CSRCS))
CPPOBJS = $(patsubst %.cpp,objs/%.o,$(CPPSRCS))

INC_DIRS = 
LDFLAGS =  
LIBS = -lm -lmongoclient -lboost_system -lboost_thread -lboost_filesystem -lboost_program_options 


$(OUTPUT):$(COBJS) $(CPPOBJS)
	$(LD) -O3 $^ $(INC_DIRS) $(LDFLAGS) $(LIBS) -o $@; 

objs/%.o:%.c 
	$(CC) -O3 -c $(CFLAGS) $(INC_DIRS) $< -o $@; 

objs/%.o:%.cpp
	g++ -O3 -c $(CFLAGS) $(INC_DIRS) $< -o $@;
