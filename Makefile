all:	check
	gcc -pedantic-errors -o arcstress arcstress.c

clean:
	rm -f arcstress

check:
	cstyle arcstress.c
