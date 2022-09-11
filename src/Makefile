include Makefile.inc

VISTA_SOURCES=vista.c
MD5_SOURCES=md5.c
SHARED_SOURCES=resource_manager.c
MD5_OUTPUT_FILE=./../md5
VISTA_OUTPUT_FILE=./../vista
SLAVE_SOURCES=slave.c
SLAVE_OUTPUT_FILE=slave.o

all: vista md5

vista:
	$(GCC) $(GCCFLAGS) $(VISTA_SOURCES) $(SHARED_SOURCES) -o $(VISTA_OUTPUT_FILE)

slave:
	$(GCC) $(GCCFLAGS) -c $(SLAVE_SOURCES) -o $(SLAVE_OUTPUT_FILE)

md5: slave
	$(GCC) $(GCCFLAGS) $(MD5_SOURCES) $(SHARED_SOURCES) $(SLAVE_OUTPUT_FILE) -o $(MD5_OUTPUT_FILE)

clean:
	rm -f $(VISTA_OUTPUT_FILE) $(MD5_OUTPUT_FILE) $(SLAVE_OUTPUT_FILE)

.PHONY: all clean md5 vista slave
