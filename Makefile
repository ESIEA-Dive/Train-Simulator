all: simulateur.c 
	gcc -o simulateur simulateur.c -l pthread

clean:
	rm simulateur
