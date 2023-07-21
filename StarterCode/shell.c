
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/stat.h>

#include "interpreter.h"
#include "shellmemory.h"


int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {

	printf("%s\n", "Shell version 1.1 Created January 2022");
	help();

	char prompt = '$';  				// Shell prompt
	char userInput[MAX_USER_INPUT];		// user's input stored here
	int errorCode = 0;					// zero means no error, default
	const char *name = "backing_storage";

	
    FILE* f = fopen("backing_storage", "r");
    if (f!= NULL) {
		printf("there is a previous backing storage\n");
		system("rm -rf backing_storage"); //clean directory
	}

	// create a new directory
	mkdir(name, S_IRWXU);

	//init user input
	for (int i=0; i<MAX_USER_INPUT; i++)
		userInput[i] = '\0';
	
	//init shell memory
	mem_init(); 

	//init variable store
	mem_init_variable_Store();

	//init frame store
	mem_init_frame_Store();

	while(1) {							
		printf("%c ",prompt);
		fgets(userInput, MAX_USER_INPUT-1, stdin);

		if (userInput[0] =='\0') {	//this identifies when the file in batch mode has reached its last character which is not null.		 	
			FILE *stop = freopen("/dev/tty", "r", stdin); // this is a function that returns to the terminal that started the program
			fgets(userInput, MAX_USER_INPUT-1, stdin);	// this is added so that a null line is not printed
		}
		
		errorCode = parseInput(userInput);
		if (errorCode == -1) exit(99);	// ignore all other errors
		memset(userInput, 0, sizeof(userInput));

	}

	return 0;

}
// Extract words from the input then call interpreter
int parseInput(char ui[]) {
 
	char tmp[200];
	char *words[100];							
	int a,b;							
	int w=0; // wordID

	for(a=0; ui[a]==' ' && a<1000; a++) { //it stops when ui[a] points to a non-whitespace	
	}	
	// skip white spaces
	
	while(ui[a] != '\0' && a<1000) {
		for(b=0; ui[a]!='\0' && ui[a]!=' ' && ui[a] !=';' && a<1000; a++, b++){
			tmp[b] = ui[a];						// extract a word 
		}
		tmp[b] = '\0'; //end a string
		
		words[w] = strdup(tmp); //each word will be added as an element of array word

		w++; // represents the number of words
		
		if(ui[a] == '\0'){
			break;
		}

		//when ui[a] == ";", we send each command at a time, with the amount of word it contains.
		// also, this part has been coded with the assumption that the last command in a one-liner will not have a ";". So,
		// the last command will be used as return outside the loop
		if(ui[a] == ';'){ //reset
			int result = interpreter(words, w);
			memset(words, 0,w); // places a 0 in the first w elements of words, thus resetting it
			w = 0;
			a++;// this is use so take care of the whitespace between commands. for e.g :set a 123; print a
		}

		a++; // increment a here to move to next word
	}
	
	return interpreter(words, w);
}
