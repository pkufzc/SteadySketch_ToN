XXFLAGS = -g -lboost_program_options --std=c++17 $(USER_DEFINES)

obj := main_test
ifeq ($(OBJ_LOCAL), 1)
	obj := ../dst/$(obj)
endif

$(obj): main.cpp
	g++ $^ $(XXFLAGS) -o $@

clean: 
	rm $(obj)
