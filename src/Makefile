include Makefile.inc

SOURCES=$(wildcard *.c)
OUTPUT_FILE=./../md5

all: md5 vista

vista:
	cd vista; make all

md5:
	$(GCC) $(GCCFLAGS) $(SOURCES) -o $(OUTPUT_FILE)

clean:
	rm -f $(OUTPUT_FILE)

.PHONY: all clean md5
