all: first
first: first.c first.h
	gcc -Wall -Werror -fsanitize=address -std=c99 first.c -o first -lm
clean:
	rm -f first
