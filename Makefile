CFLAGS ?= -O3 -flto

all: rnc64 rnc32
.PHONY: all

rnc32: CFLAGS += -m32
rnc%: main.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f rnc64 rnc32
.PHONY: clean
