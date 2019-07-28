all: 		parser

clean:
	rm parser.cpp parser.hpp parser tokens.cpp

parser.cpp:	parser.y node.hpp tokens.cpp
	bison -d -o $@ -v parser.y

tokens.cpp:	tokens.l
	flex -o $@ tokens.l

parser:		tokens.cpp parser.cpp main.cpp
	g++ -o $@ *.cpp `llvm-config --libs core --ldflags --system-libs`