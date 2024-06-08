all: mysim.c
	gcc -g -std=gnu11 -Werror -Wall mysim.c -o mysim

run: mysim
	./mysim --free-list=explicit --fit=first examples/1.in

clean: mysim
	rm -rf ./mysim
