all: run

cust_group: main.c
	gcc -o cust_group main.c -lpthread

run: cust_group
	./cust_group
