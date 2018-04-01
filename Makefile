all: main.c
	gcc -ggdb3 -O0 main.c -o rnc64
	gcc -m32 -ggdb3 -O0 main.c -o rnc32
clean:
	rm -f rnc64 rnc32
