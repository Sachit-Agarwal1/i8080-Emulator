emulator.exe: Core.o main.o
		gcc Core.o main.o -o emulator -g
main.o : main.c
		gcc -c main.c -g
Core.o : Core.c program1
		gcc -c Core.c -g
program1: progMaker.py
		py progMaker.py
clean: 
		del Core.o main.o program1