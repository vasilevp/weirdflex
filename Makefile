all: 		parser

clean:
	rm parser.cpp parser.hpp parser parser.exe tokens.cpp tokens.o parser.o main.o node.o codegen.o

parser.o:	parser.y node.hpp tokens.cpp
	bison -d -o $@ -v parser.y
	g++ -c parser.cpp

tokens.o:	tokens.l
	flex -o tokens.cpp tokens.l
	g++ -c tokens.cpp

main.o: main.cpp
	g++ -c $^

node.o: node.cpp
	g++ -c $^

codegen.o: codegen.cpp
	g++ -c $^

parser:		tokens.o parser.o main.o node.o codegen.o
	g++ -ggdb -std=c++2a -o $@ *.o `llvm-config --libs --ldflags --system-libs`