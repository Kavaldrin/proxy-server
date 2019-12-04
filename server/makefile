CC = g++
DEBUG = -g
SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)
CPPFLAGSC = -Wall -c $(DEBUG) -MMD -std=c++17 -lboost_regex
CPPFLAGS = -Wall $(DEBUG) -MF -MM -std=c++17 -lboost_regex
LIBS = $(wildcard *.h)
EXE = PiSC
DEPS := $(OBJS:.o=.d)
K = default

all: $(SRC) $(EXE)

-include $(DEPS)

$(EXE): $(OBJS)
	$(CC) $(CPPFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CPPFLAGSC) $< -o $@

.PHONY: clean cleanall valgrind gdb run tar generator

clean:
	rm *.o *.d
cleanall:
	rm *.o *.d $(EXE)
valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./$(EXE)
gdb:
	gdb $(EXE)
run: 
	./$(EXE)
tar:
	tar czf $(EXE).tar.gz $(SRC) $(LIBS)
generator:
	echo 'class $(K);\n' > $(K).h
	echo '#ifndef _$K_H_' >> $(K).h
	echo '#define _$K_H_' >> $(K).h
	echo '\n\n\n' >> $(K).h
	echo 'class $(K){' >> $(K).h
	echo '\n\n\n\n\n\n' >> $(K).h
	echo '};' >> $(K).h
	echo '\n\n\n' >> $(K).h
	echo '#endif' >> $(K).h
	echo '\n' >> $(K).h
	echo '#include "$(K).h"' > $(K).cpp
