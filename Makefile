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
	g++ -c parser.cpp

tokens.o: tokens.cpp parser.o
	@echo ":: building tokens.o"
	g++ -c -I $(PWD) tokens.cpp

main.o: main.cpp
	@echo ":: building main.o"
	g++ -c $^

node.o: node.cpp
	@echo ":: building node.o"
	g++ -c $^

codegen.o: codegen.cpp
	@echo ":: building codegen.o"
	g++ -c $^

parser:		tokens.o parser.o main.o node.o codegen.o
	g++ -ggdb -std=c++2a -o $@ tokens.o parser.o main.o node.o codegen.o `llvm-config --libs --ldflags --system-libs`