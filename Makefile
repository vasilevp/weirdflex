GXX_OPTS=-ggdb -O0 -std=c++17 -I `llvm-config --includedir`

all: 		parser

clean:
	rm -f parser.cpp parser.hpp parser parser.exe tokens.cpp *.o parser.output

tokens.cpp: tokens.l
	@echo ":: generating tokens.cpp"
	flex -o tokens.cpp tokens.l

parser.cpp: parser.y
	@echo ":: generating parser.cpp, parser.hpp"
	bison -d -o parser.cpp -v parser.y

parser.o:	parser.y node.hpp tokens.cpp parser.cpp
	@echo ":: building parser.o"
	g++ ${GXX_OPTS} -c parser.cpp

tokens.o: tokens.cpp parser.o
	@echo ":: building tokens.o"
	g++ ${GXX_OPTS} -c tokens.cpp

main.o: main.cpp
	@echo ":: building main.o"
	g++ ${GXX_OPTS} -c $^

node.o: node.cpp
	@echo ":: building node.o"
	g++ ${GXX_OPTS} -c $^

codegen.o: codegen.cpp
	@echo ":: building codegen.o"
	g++ ${GXX_OPTS} -c $^

parser:		tokens.o parser.o main.o node.o codegen.o
	g++ ${GXX_OPTS} -o $@ tokens.o parser.o main.o node.o codegen.o `llvm-config --libs --ldflags --system-libs`