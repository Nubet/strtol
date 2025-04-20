all: main

main: main.o
	gcc -fsanitize=undefined -g -Wall -pedantic $^ -o $@

.c.o:
	gcc -fsanitize=undefined -g -Wall -pedantic -c $< -o $@

clean:
	rm -f main *.o
