# Author: Jared Stark;   Created: Mon Aug 16 11:28:20 PDT 2004
# Description: Makefile for building a cbp submission.

CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

objects = cbp_inst.o main.o op_state.o predictor.o tread.o
objects_comp = cbp_inst.o main.o op_state.o predictor_comp.o tread.o

ifeq ($(OS),Windows_NT)
predictor : $(objects)
	$(CXX) -o $@.exe $(objects)
	
predictor_comp : $(objects_comp)
	$(CXX) -o $@.exe $(objects_comp)
else
predictor : $(objects)
	$(CXX) -o $@ $(objects)
predictor_comp : $(objects_comp)
	$(CXX) -o $@ $(objects_comp)
endif


cbp_inst.o : cbp_inst.h cbp_assert.h cbp_fatal.h cond_pred.h finite_stack.h indirect_pred.h stride_pred.h value_cache.h
main.o : tread.h cbp_inst.h predictor.h op_state.h
op_state.o : op_state.h
predictor.o : predictor.h op_state.h tread.h cbp_inst.h
predictor_comp.o : predictor.h op_state.h tread.h cbp_inst.h
tread.o : tread.h cbp_inst.h op_state.h

all: predictor predictor_comp

.PHONY : clean
clean :
	rm -f predictor predictor_comp $(objects) $(objects_comp)

