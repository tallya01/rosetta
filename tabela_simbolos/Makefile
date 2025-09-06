#Substitua o valor de CC com o nome do compilador que voce usa: gcc ou g++
CC = gcc
CFLAGS =   -g  -Wall 
LFLAGS = -lm

# 

main:    tabela_simbolos.o main.o 
	$(CC) $(CFLAGS) $(LFLAGS) tabela_simbolos.o main.o -o  main


main.o: main.c
	$(CC) $(CFLAGS) $(LFLAGS) -c main.c -o main.o
                

tabela_simbolos.o: tabela_simbolos.c
	$(CC) $(CFLAGS) -c tabela_simbolos.c -o tabela_simbolos.o

clean:
	rm -f     *.o    main

cleanObj:
	rm -f   *.o  
