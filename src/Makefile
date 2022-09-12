include Makefile.inc

VISTA_SOURCES=vista.c
APP_SOURCES=app.c
SHARED_SOURCES=resource_manager.c
APP_OUTPUT_FILE=./../md5
VISTA_OUTPUT_FILE=./../vista
SLAVE_SOURCES=slave.c
SLAVE_OUTPUT_FILE=slave.o

all: vista app

vista:
	$(GCC) $(GCCFLAGS) $(VISTA_SOURCES) $(SHARED_SOURCES) -o $(VISTA_OUTPUT_FILE)

slave:
	$(GCC) $(GCCFLAGS) -c $(SLAVE_SOURCES) -o $(SLAVE_OUTPUT_FILE)

app: slave
	$(GCC) $(GCCFLAGS) $(APP_SOURCES) $(SHARED_SOURCES) $(SLAVE_OUTPUT_FILE) -o $(APP_OUTPUT_FILE)

clean:
	rm -f $(VISTA_OUTPUT_FILE) $(APP_OUTPUT_FILE) $(SLAVE_OUTPUT_FILE)

.PHONY: all clean app vista slave
