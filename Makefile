GXX_OPTS=-ggdb -O0 -std=c++17 -I `llvm-config --includedir` #-D_DEBUG=1

all: 		parser

clean:
	rm -f parser.cpp parser.hpp parser tokens.cpp *.o parser.output a.out *.exe

tokens.cpp: tokens.l lexutils.hpp
	@echo ":: generating tokens.cpp"
	flex -o tokens.cpp tokens.l

parser.cpp: parser.y
	@echo ":: generating parser.cpp, parser.hpp"
	bison -d -o parser.cpp -v -Wall parser.y

parser.o:	parser.y node.hpp tokens.cpp parser.cpp
	@echo ":: building parser.o"
	g++ ${GXX_OPTS} -c parser.cpp

tokens.o: tokens.cpp parser.o
	@echo ":: building tokens.o"
	g++ ${GXX_OPTS} -c tokens.cpp

main.o: main.cpp
	@echo ":: building main.o"
	g++ ${GXX_OPTS} -c main.cpp

node.o: node.cpp node.hpp
	@echo ":: building node.o"
	g++ ${GXX_OPTS} -c node.cpp

codegen.o: codegen.cpp codegen.hpp
	@echo ":: building codegen.o"
	g++ ${GXX_OPTS} -c codegen.cpp

parser:		tokens.o parser.o main.o node.o codegen.o
	@echo ":: linking parser"
	g++ ${GXX_OPTS} -o parser tokens.o parser.o main.o node.o codegen.o `llvm-config --libs --ldflags --system-libs`
