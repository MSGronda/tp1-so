all: src

src:
	cd src; make all

clean:
	cd src; make clean
	cd src; cd vista; make clean

.PHONY: all src clean
