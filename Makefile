all: src

src:
	cd src; make all

clean:
	cd src; make clean

.PHONY: all src clean
