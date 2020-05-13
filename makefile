all: clean run

run: run.c 
	gcc -o run run.c
	
clean:
	rm -f run *.exe *.stackdump 
