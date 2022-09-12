include Makefile.inc

SOURCES_RESOURCE=$(wildcard resource_manager/*.c)
SOURCES_VISTA=vista.c
SOURCES_APP=app.c
SOURCES_SLAVE=slave.c

OUT_APP=./../md5
OUT_VISTA=./../vista


all: vista app

vista:
	@$(GCC) $(GCCFLAGS) $(SOURCES_VISTA) $(SOURCES_RESOURCE) -o $(OUT_VISTA)
	@echo "Successfully created vista"

app: slave
	@$(GCC) $(GCCFLAGS) $(SOURCES_APP) $(SOURCES_RESOURCE) $(SOURCES_SLAVE) -o $(OUT_APP)
	@echo "Successfully created md5"

clean:
	rm -f $(OUT_APP) $(OUT_VISTA) 

.PHONY: all clean app vista slave res_manager
