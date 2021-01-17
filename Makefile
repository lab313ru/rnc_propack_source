CFLAGS ?= -O3 -flto

all: main.c
	$(CC) $(CFLAGS) main.c -o rnc64
	$(CC) $(CFLAGS) -m32 main.c -o rnc32
.PHONY: all

clean:
	rm -f rnc64 rnc32
.PHONY: clean
