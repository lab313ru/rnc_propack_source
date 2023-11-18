CFLAGS ?= -O3 -flto -Wall

all: rnc
.PHONY: all

rnc: main.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f rnc
.PHONY: clean
