include Makefile.inc

SOURCES=$(wildcard *.c)
OUTPUT_FILE=./../md5

all: 
	$(GCC) $(GCCFLAGS) $(SOURCES) -o $(OUTPUT_FILE)

clean:
	rm -f $(OUTPUT_FILE)

.PHONY: all clean
