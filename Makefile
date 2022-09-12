all: src

src:
	@cd src; make all

clean:
	@cd src; make clean;
	rm -f respuesta.txt

.PHONY: all src clean
