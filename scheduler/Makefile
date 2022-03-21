sche: main.o helper.o header.h
	g++ main.o helper.o -o sche -ggdb

main.o: main.cpp header.h
	g++ -c main.cpp -o main.o -ggdb

helper.o: helper.cpp header.h
	g++ -c helper.cpp -o helper.o -ggdb

clean:
	rm -f sche main.o helper.o