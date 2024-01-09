.phony all:
all: main

main: 
	gcc -Wall diskfunctions.c diskinfo.c -o diskinfo
	gcc -Wall diskfunctions.c disklist.c -o disklist
	gcc -Wall diskfunctions.c diskget.c -o diskget
	gcc -Wall diskfunctions.c diskput.c -o diskput

.PHONY clean:
clean:
	-rm -rf *.o *.exe diskinfo
	-rm -rf *.o *.exe disklist
	-rm -rf *.o *.exe diskget
	-rm -rf *.o *.exe diskput
