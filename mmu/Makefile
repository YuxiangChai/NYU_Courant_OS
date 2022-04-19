mmu: main.o helper.o header.h
	g++ -g main.o helper.o -o mmu

main.o: main.cpp header.h
	g++ -g -c main.cpp -o main.o

helper.o: helper.cpp header.h
	g++ -g -c helper.cpp -o helper.o

clean:
	rm -f mmu main.o helper.o