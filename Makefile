all: readln cliente cloudshell

cloudshell: cloudshell.c readln.o
	gcc cloudshell.c readln.o -o cloudshell

cliente: cliente.c readln.o
	gcc cliente.c readln.o -o cliente

readln: readln.c
	gcc readln.c -c
