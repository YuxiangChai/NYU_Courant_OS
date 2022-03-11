linker: main.o tokenizer.o parser.o
	g++ main.o tokenizer.o parser.o -o linker

main.o: main.cpp header.h
	g++ -c main.cpp -o main.o

tokenizer.o: tokenizer.cpp header.h
	g++ -c tokenizer.cpp -o tokenizer.o

parser.o: parser.cpp header.h
	g++ -c parser.cpp -o parser.o

clean:
	rm -f linker main.o tokenizer.o parser.o