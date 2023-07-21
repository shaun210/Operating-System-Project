#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 7;

//PCB
typedef struct PCB {
	int PID;
	int size;
	struct PCB *next;
	int pagetable[200]; 
	int currentLineNumberInFile; //a pointer to know which line we are in cuurent file
	int currentNumberOfFileLineInFrame;// use to know how many lines have been loaded from file to frame. Used for pageFault
	int offset; // use to know when we are done with a frame
	int pageTableIndex; // The index for the page table. This is used to know in which frame we need to execute the lines for the current program.
	int sizeOfFile; //use to keep track of the size of each file for each pcb
	int time_slice;
} PCBs;

// elements of the LRU queue
struct LRU_node {
	struct LRU_node *next;
	int element;
};

PCBs *first, *rear; //first is the head of PCB queue and rear is the last node

struct LRU_node *head_LRU, *tail_LRU;

int help();
int quit();
int badcommand();
int set(char* var, char* value);
int print(char* var);
int run2(char* script);
int badcommandFileDoesNotExist();
int badcommandTooManyTokens();
int badcommandTwoFilesSameName();
int badcommandProgramsDoNotFit();
int echo(char* word);
int my_ls();
int countNumberOfFiles();
int run(char* script, int start);
void enqueue(PCBs *pcb);
void enqueue_front(PCBs *pcb);
void enqueue_middle(PCBs* pcb);
void dequeue();
int* countNoOfLines(int numberOfPrograms, char** arrayOfFilesNames);
int countNumberOfFilesForBackingStorage();
void enqueue_LRU(struct LRU_node *node);
void dequeue_LRU_rear();
void print_LRU_queue();
void enqueue_LRU_front(struct LRU_node *node);
int peek_LRU_rear();
void empty_LRU_queue();
void dequeue_first_occurence_LRU(struct LRU_node *node);
int look_for_occurence_LRU(struct LRU_node *node);
int resetmem();






// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	int i;

	if (args_size < 1 ){ // i changed this
		return badcommand();
	}
	if (args_size > MAX_ARGS_SIZE){ // not exceed 7 arguments
		return badcommandTooManyTokens();
	}


	for ( i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if(strcmp(command_args[0], "resetmem")==0){
		if (args_size != 1) return badcommand();
		return resetmem();
	}

	if (strcmp(command_args[0], "help")==0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit")==0) {
		//quit
		if (args_size != 1) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set")==0) {
		if (args_size < 3) return badcommand(); // set must have at least one STRING

		int i;
		char temp[1001] =""; // a temporary char array was created to store all the STRING tokens.
		char* space =" "; // a string that represents whitespaces		

		for (i = 2; i < args_size; i++){ 
			strcat(temp, command_args[i]);
			if(i+1 != args_size) {
				strcat(temp,space); //add space if we are not at last token
			}
		}
		return set(command_args[1], temp);
	
	} else if (strcmp(command_args[0], "print")==0) {
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} else if (strcmp(command_args[0], "exec") == 0) {
		if (args_size < 3 || args_size > 5)  return badcommand();

		int numberOfPrograms = args_size - 2;

		//check if the files have valid name
		for(int i = 1; i<= numberOfPrograms; i++){
			FILE *tempP = fopen(command_args[i],"rt");  // the program is in a file
			if(tempP == NULL){
				return badcommandFileDoesNotExist();
			}
			fclose(tempP);
		}
		
		//this is to deal with the case where we run two or more consecutives exec command
		int number = 0;
		number = countNumberOfFilesForBackingStorage();
		if (number > 0)
		{
			system("rm -rf backing_storage"); //clean directory
			mkdir("backing_storage", S_IRWXU); // create a new directory
		}	

		//saves the file name in an array
		char *arrayOfFilesNames[numberOfPrograms];
		for(int i = 1; i<= numberOfPrograms; i++){
			arrayOfFilesNames[i-1] = command_args[i];
		}	
		
		//get the number of lines in each prog. these are stored in an array
		int *arrNumberOfLines;
		arrNumberOfLines = countNoOfLines(numberOfPrograms, arrayOfFilesNames); 

		//copy each file to backing_storage directory
		for (int i = 0; i < numberOfPrograms; i++) {
			char *fileName = command_args[i+1];
			char cp[] ="cp ";
			char backingStorage[] =" backing_storage";
			char command[strlen(fileName) +strlen(cp)+ strlen(backingStorage)];
			sprintf(command, "%s%s%s",cp, fileName, backingStorage);
			system(command);
		}
		
		if (strcmp(command_args[args_size - 1], "RR") == 0) {
			
			mem_init_frame_Store(); // initialize framestore

			PCBs *arrayPCB[numberOfPrograms]; 
			int z = 0;
			
			for (int i = 1; i <= numberOfPrograms; i++) {

				int frame = i-1; // frame number. start with 0 then 1 then 2
				int lineInFrame = frame*3; // the position of the pointer in frame store
				int tempC = 0; // use to put 2 frame. Used as pageTable array index for this loop only
				int numberOfLineInProgram = arrNumberOfLines[i-1];
				
				//initialize each PCB	
				arrayPCB[i-1] = (PCBs*) malloc(sizeof(PCBs));
				arrayPCB[i-1]->PID = i;		
				arrayPCB[i-1]->currentLineNumberInFile = 0;
				arrayPCB[i-1]->currentNumberOfFileLineInFrame = 0;
				arrayPCB[i-1]->offset = 0;
				arrayPCB[i-1]->pageTableIndex = 0;
				arrayPCB[i-1]->sizeOfFile = numberOfLineInProgram;
				arrayPCB[i-1]->time_slice = 2;
				enqueue(arrayPCB[i-1]); //add to ready queue

				//add 2 frame in frame store for each program at the start of execution. If a file cannot fit 2 frames, then stop
				while (numberOfLineInProgram - arrayPCB[i-1]->currentLineNumberInFile > 0 && tempC < 2) {
					arrayPCB[i-1]->pagetable[tempC] = frame;
					int offsetInFrame = 0; //add 3 line to each frame

					//add to LRU queue
					struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));

					node->element = arrayPCB[i-1]->pagetable[tempC]; // add current process  frame number as element

					if (head_LRU == NULL){
						enqueue_LRU(node);
					}
					else {
						enqueue_LRU_front(node);
					}
							
					//either fill frame or fill frame with maximum number of lines of code possible if code does not have 3 lines available
					while(offsetInFrame < 3 && numberOfLineInProgram - arrayPCB[i-1]->currentLineNumberInFile > 0){
						
						setFrameMemory(arrayOfFilesNames[i-1], arrayPCB[i-1]->currentLineNumberInFile, lineInFrame);
						offsetInFrame++; // move pointer in current frame
						arrayPCB[i-1]->currentLineNumberInFile++; //currentLineNumberInFile will be 6 initially for each program if 2 page gets loaded
						lineInFrame++;
					}

					//Fill a frame if a page does not have 3 lines
					while (offsetInFrame < 3){		
						//arrayPCB[i-1]->currentNumberOfFileLineInFrame++ // do I need this?
						set_frame_memory("void", lineInFrame);
						lineInFrame++;
						offsetInFrame++;
						
					}
		
					//jump from 1 frame to another, for same program. So if there are 2 program , jump 1 frame. If there are 3 programs, jump 2 frames
					if (numberOfLineInProgram - arrayPCB[i-1]->currentLineNumberInFile > 0) { 

						lineInFrame += ((numberOfPrograms-1) * 3); //must add this 1 so that we jump to required line in framestore
						frame += numberOfPrograms; //change frame
					}
					tempC++;

				}

				
			}

			int numberOfScripts = args_size - 2; //-2 because of exec and policy

			//execute processes
			int errCode = 0;
			while (first != NULL) {

				if (first->offset >2){			
					first->offset = 0;
				}

				//takes care of the case where we interrupt a program from running twice when there is a page fault
				if (first->time_slice < 1){ //0 or -1
						first->time_slice = 2;
				}

				

				// if we there is no page fault, we update the LRU_queue
				if (first->currentNumberOfFileLineInFrame != first->currentLineNumberInFile) {

					//add to LRU queue
					struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
					node->element = first->pagetable[first->pageTableIndex]; // add current process  frame number as element

					//remove the previous occurence of the current process if it is the head
					if (head_LRU  != NULL && node->element == head_LRU->element) {
						free(node);
					}
					else {
						if (head_LRU == NULL){
							enqueue_LRU(node);
						}
						else{
							enqueue_LRU_front(node);
							
							int occurence = look_for_occurence_LRU(node); // we cannot have two identical frame number in the LRU queue
							if (occurence == 1) { //meaning that we need to remove a node

								dequeue_first_occurence_LRU(node);
							}
							
						}
					}
				
				}
				


							
				


				for (int count = 0; count < 2; count++) { 	
		
					if(first->time_slice <0) break;

					//page fault handler //
					//currurrentNumberOfFileLineInFrame is an integer that represents how many instruction from a file have been loaded in framestore
					if (first->currentNumberOfFileLineInFrame == first->currentLineNumberInFile) {

						if (first->sizeOfFile == first->currentNumberOfFileLineInFrame) {
							break;
						}

						char *fileName = arrayOfFilesNames[first->PID -1];				
						int frameNumber = look_for_empty_frame(); //frame number will represent the frame. Else, -1

						//if frameNumber == -1, then there are no empty frames
						// if != -1, we add to that frame
						if (frameNumber != -1) {

							//change frame of current process
							first->pageTableIndex++;
							first->pagetable[first->pageTableIndex] = frameNumber; //update page table
							int frameIndex = (frameNumber*3); //the line in the frame store where we add the instruction

							// //make change to LRU_queue
							struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
							node->element = first->pagetable[first->pageTableIndex];

							if (head_LRU == NULL){
								enqueue_LRU(node);
							}
							else{
								enqueue_LRU_front(node);
								int occurence = look_for_occurence_LRU(node);
								if (occurence == 1) { //meaning that we need to remove a node
									//printf("5\n");
									dequeue_first_occurence_LRU(node);
								}
							}
							
							//we set 3 lines of current process into a frame, even if there is less than 3 lines of code available
							for (int j = frameIndex; j< (frameIndex+3); j ++) {

								// in the case where we do not have enough lines in file to fill a frame. we fill it with void
								if (first->sizeOfFile == first->currentLineNumberInFile){
				
									for (int i = j; i < (frameIndex + 3); i++)
									{
										set_frame_memory("void", i);
									}
									break;
								}

								int value = first->currentLineNumberInFile;
								setFrameMemory(fileName, value, j);
								first->currentLineNumberInFile++;
								
							}

						}
						else {
							// remove the tail of LRU queue. Then add to framestore directly
							printf("Page fault! Victim page contents:\n");
							int rear_index = peek_LRU_rear();// we take the index at end of LRU queue

							//change frame of current process
							first->pageTableIndex++;
							first->pagetable[first->pageTableIndex] = rear_index; //update page table

							struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
							node->element = first->pagetable[first->pageTableIndex];

							if (head_LRU == NULL){
								enqueue_LRU(node);
							}
							else{
								enqueue_LRU_front(node);
								int occurence = look_for_occurence_LRU(node);
								if (occurence == 1) { //meaning that we need to remove a node
									//printf("5\n");
									dequeue_first_occurence_LRU(node);
								}
							}

							//use index to know which frame to remove
							int frame_address = (rear_index * 3) ;

							// print out the page that are removed
							for(int address = frame_address; address < (frame_address +3); address ++) { //iterate through next 3 address in framestore
								char *lineBeRemoved = mem_get_value_frame_store(address);
								printf("%s\n", lineBeRemoved);
							}

							for (int k = frame_address; k < (frame_address +3); k++){
								// in the case where we do not have enough lines in file to fill a frame. we fill it with void
								if (first->sizeOfFile == first->currentLineNumberInFile){
				
									for (int i = k; i < (frame_address + 3); i++)
									{
										set_frame_memory("void", i);
									}
									break;
								}
								int value = first->currentLineNumberInFile;
								setFrameMemory(fileName, value, k);
								first->currentLineNumberInFile++;

								
							}
							printf("End of victim page contents.\n");


						}


						break;

					}

					first->time_slice--; // used in page fault
					if(first->time_slice <0) break;

					//change frame of current program
					if (first->offset >2){
						first->pageTableIndex++;
						first->offset = 0;


						//add to LRU queue
						struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
						node->element = first->pagetable[first->pageTableIndex];

						if (head_LRU  != NULL && node->element == head_LRU->element) {
							free(node);
						}
						else {
							if (head_LRU == NULL){
								enqueue_LRU(node);
							}
							else{
								//printf("3\n");
								int temp_node_element = node->element;
								enqueue_LRU_front(node);
								int occurence = look_for_occurence_LRU(node);
								if (occurence == 1) { //meaning that we need to remove a node
									//printf("4\n");
									dequeue_first_occurence_LRU(node);
								}
						

							}
						}
					}

					int index_of_frameStore = (first->pagetable[first->pageTableIndex] *3) + first->offset; //pointer in frame store to get line of code
					char *line = mem_get_value_frame_store(index_of_frameStore);
					
					errCode = run2(line);

					first->currentNumberOfFileLineInFrame++;
					first->offset++;

					//check if a program has reached the end
					if (first->sizeOfFile == first->currentNumberOfFileLineInFrame){
						break;
					}
			
				}
			
				if (first == NULL) { //if queue empty, break
					
					break;
				}

				//dequeue program if we have finished executing it
				if (first->sizeOfFile == first->currentNumberOfFileLineInFrame){
					dequeue();
					continue;
				}

				// store all of node first into variables. we cannot use temp nodes for pointers
				PCBs* ne = first->next;
				int pid = first->PID;
				int currentOffset = first->offset;
				int currentFileLine = first->currentLineNumberInFile;
				int currentFrameLine = first->currentNumberOfFileLineInFrame;
				int pageTableIndex1 = first->pageTableIndex;
				int sizeFile = first->sizeOfFile;
				int slice = first->time_slice;
		
				int sizeOfPageTab = sizeof(first->pagetable) / sizeof(int);
				// initialize temporary variable
				PCBs* temp = (PCBs*) malloc(sizeof(PCBs));				

				temp->PID = pid;
				temp->offset = currentOffset;
				temp->currentLineNumberInFile = currentFileLine;
				temp->currentNumberOfFileLineInFrame = currentFrameLine;
				temp->pageTableIndex = pageTableIndex1;
				temp->sizeOfFile = sizeFile;
				temp->time_slice = slice;
				//copy page table
				memcpy(temp->pagetable, first->pagetable, sizeof(first->pagetable));

				//remove head and add it at the end
				dequeue();
				enqueue(temp);

			}
			empty_LRU_queue();
			
			mem_init_variable_Store(); // added this so that we don't have to call resetmem each time after an exec command.
			return errCode;

		} 

		// if we did not mention policy
		else {
			return badcommand(); 
		}


	} 
	else if (strcmp(command_args[0], "run")==0) {
		if (args_size != 2) return badcommand();

		int numberOfPrograms = args_size - 1;

		// //check if the files have valid name
		for(int i = 1; i<= numberOfPrograms; i++){
			FILE *tempP = fopen(command_args[i],"rt");  // the program is in a file
			if(tempP == NULL){
				
				return badcommandFileDoesNotExist();
			}
			fclose(tempP);
		}
		
		//this is to deal with the case where we run two or more consecutives exec command
		int number = 0;
		number = countNumberOfFilesForBackingStorage();
		if (number > 0)
		{
			system("rm -rf backing_storage"); //clean directory
			mkdir("backing_storage", S_IRWXU); // create a new directory
		}	

		//saves the file name in an array
		char *arrayOfFilesNames[numberOfPrograms];
		for(int i = 1; i<= numberOfPrograms; i++){
			arrayOfFilesNames[i-1] = command_args[i];
		}	
		
		//get the number of lines in each prog. these are stored in an array
		int *arrNumberOfLines;
		arrNumberOfLines = countNoOfLines(numberOfPrograms, arrayOfFilesNames); 

		//copy each file to backing_storage directory
		for (int i = 0; i < numberOfPrograms; i++) {

			char *fileName = command_args[i+1];
			char cp[] ="cp ";
			char backingStorage[] =" backing_storage";

			char command[strlen(fileName) +strlen(cp)+ strlen(backingStorage)]; // issues a command to system to copy file
			sprintf(command, "%s%s%s",cp, fileName, backingStorage);
			system(command);
		}
			
			mem_init_frame_Store(); // initialize framestore

			PCBs *arrayPCB[numberOfPrograms]; 
			
			// initialize the field of each PCB.
			for (int i = 1; i <= numberOfPrograms; i++) {
				int frame = i-1; // frame number. start with 0 then 1 then 2
				int lineInFrame = frame*3; // the position of the pointer in frame store
				int tempC = 0; // use to put 2 frame. Also used as pageTable array index
				int numberOfLineInProgram = arrNumberOfLines[i-1];
				
				//initialize each PCB	
				arrayPCB[i-1] = (PCBs*) malloc(sizeof(PCBs));
				arrayPCB[i-1]->PID = i;		
				arrayPCB[i-1]->currentLineNumberInFile = 0;
				arrayPCB[i-1]->currentNumberOfFileLineInFrame = 0;
				arrayPCB[i-1]->offset = 0;
				arrayPCB[i-1]->pageTableIndex = 0;
				arrayPCB[i-1]->sizeOfFile = numberOfLineInProgram;
				arrayPCB[i-1]->time_slice = 2;
				enqueue(arrayPCB[i-1]); //add to ready queue

				//add 2 frame in frame store for each program at the start of execution. must take care of edge case too
				while (numberOfLineInProgram - arrayPCB[i-1]->currentLineNumberInFile > 0 && tempC < 2) {
					arrayPCB[i-1]->pagetable[tempC] = frame;
					int offsetInFrame = 0; //add 3 line to each frame

					//add to LRU queue
					struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));

					node->element = arrayPCB[i-1]->pagetable[tempC]; // add current process  frame number as element

					if (head_LRU == NULL){
						enqueue_LRU(node);
					}
					else {
						enqueue_LRU_front(node);
					}

					while(offsetInFrame < 3 && numberOfLineInProgram - arrayPCB[i-1]->currentLineNumberInFile > 0){			
						setFrameMemory(arrayOfFilesNames[i-1], arrayPCB[i-1]->currentLineNumberInFile, lineInFrame);
						offsetInFrame++; // move pointer in current frame
						arrayPCB[i-1]->currentLineNumberInFile++; //currentLineNumberInFile will be 6 initially for each program if 2 page gets loaded
						lineInFrame++;

					}

					//Fill a frame if a page does not have 3 lines
					while (offsetInFrame < 3){		
						set_frame_memory("void", lineInFrame);
						lineInFrame++;
						offsetInFrame++;	

					}
		
					//jump from 1 frame to another, for same program. So if there are 2 program , jump 1 frame. If there are 3 programs, jump 2 frames
					if (numberOfLineInProgram - arrayPCB[i-1]->currentLineNumberInFile > 0) { 
						lineInFrame += ((numberOfPrograms-1) * 3); //must add this 1 so that we jump to required line in framestore
						frame += numberOfPrograms; //change frame

					}
					tempC++;

				}

				
			}

			int numberOfScripts = args_size - 1; //-2 because of exec and policy

			//execute processes
			int errCode = 0;
			while (first != NULL) {

				//offset is used to keep count of number of lines that has been executed in each frame
				if (first->offset >2){			
					first->offset = 0;
				}

				// if we there is no page fault, we update the LRU_queue
				if (first->currentNumberOfFileLineInFrame != first->currentLineNumberInFile) {

					//add to LRU queue
					struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
					node->element = first->pagetable[first->pageTableIndex]; // add current process  frame number as element

					//remove the previous occurence of the current process if it is the head
					if (head_LRU  != NULL && node->element == head_LRU->element) {
						free(node);
					}
					else {
						if (head_LRU == NULL){
							enqueue_LRU(node);
						}
						else{
							int temp_node_element = node->element;
							enqueue_LRU_front(node);
							
							int occurence = look_for_occurence_LRU(node); // we cannot have two identical frame number in the LRU queue
							if (occurence == 1) { //meaning that we need to remove a node

								dequeue_first_occurence_LRU(node);
							}
							
						}
					}
				
				}
							
				//takes care of the case where we interrupt a program from running twice when there is a page fault
				if (first->time_slice < 1){ //0 or -1
						first->time_slice = 2;
				}


				for (int count = 0; count < 2; count++) { 	
		
					if(first->time_slice <0) break;

					//page fault handler //
					//currentNumberOfFileLineInFrame is an integer that represents how many instruction from a file have been loaded in framestore
					if (first->currentNumberOfFileLineInFrame == first->currentLineNumberInFile) {

						if (first->sizeOfFile == first->currentNumberOfFileLineInFrame) {
							break;
						}

						char *fileName = arrayOfFilesNames[first->PID -1];				
						int frameNumber = look_for_empty_frame(); //frame number will represent the frame. Else, -1

						//if frameNumber == -1, then there are no empty frames
						// if != -1, we add to that frame
						if (frameNumber != -1) {

							//change frame of current process
							first->pageTableIndex++;
							first->pagetable[first->pageTableIndex] = frameNumber; //update page table
							int frameIndex = (frameNumber*3); //the line in the frame store where we add the instruction

							// //make change to LRU_queue
							struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
							node->element = first->pagetable[first->pageTableIndex];

							if (head_LRU == NULL){
								enqueue_LRU(node);
							}
							else{
								enqueue_LRU_front(node);
								int occurence = look_for_occurence_LRU(node);
								if (occurence == 1) { //meaning that we need to remove a node
									//printf("5\n");
									dequeue_first_occurence_LRU(node);
								}
							}
							
							//we set 3 lines of current process into a frame, even if there is less than 3 lines of code available
							for (int j = frameIndex; j< (frameIndex+3); j ++) {

								// in the case where we do not have enough lines in file to fill a frame. we fill it with void
								if (first->sizeOfFile == first->currentLineNumberInFile){
				
									for (int i = j; i < (frameIndex + 3); i++)
									{
										set_frame_memory("void", i);
									}
									break;
								}

								int value = first->currentLineNumberInFile;
								setFrameMemory(fileName, value, j);
								first->currentLineNumberInFile++;
								
							}

						}
						else {
							// remove the tail of LRU queue. Then add to framestore directly
							printf("Page fault! Victim page contents:\n");
							int rear_index = peek_LRU_rear();// we take the index at end of LRU queue

							//change frame of current process
							first->pageTableIndex++;
							first->pagetable[first->pageTableIndex] = rear_index; //update page table

							struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
							node->element = first->pagetable[first->pageTableIndex];

							if (head_LRU == NULL){
								enqueue_LRU(node);
							}
							else{
								enqueue_LRU_front(node);
								int occurence = look_for_occurence_LRU(node);
								if (occurence == 1) { //meaning that we need to remove a node
									//printf("5\n");
									dequeue_first_occurence_LRU(node);
								}
							}

							//use index to know which frame to remove
							int frame_address = (rear_index * 3) ;

							// print out the page that are removed
							for(int address = frame_address; address < (frame_address +3); address ++) { //iterate through next 3 address in framestore
								char *lineBeRemoved = mem_get_value_frame_store(address);
								printf("%s\n", lineBeRemoved);
							}

							for (int k = frame_address; k < (frame_address +3); k++){
								// in the case where we do not have enough lines in file to fill a frame. we fill it with void
								if (first->sizeOfFile == first->currentLineNumberInFile){
				
									for (int i = k; i < (frame_address + 3); i++)
									{
										set_frame_memory("void", i);
									}
									break;
								}
								int value = first->currentLineNumberInFile;
								setFrameMemory(fileName, value, k);
								first->currentLineNumberInFile++;

								
							}
							printf("End of victim page contents.\n");


						}


						break;

					}

					first->time_slice--; // used in page fault
					if(first->time_slice <0) break;

					//change frame of current program
					if (first->offset >2){
						first->pageTableIndex++;
						first->offset = 0;


						//add to LRU queue
						struct LRU_node *node = (struct LRU_node*) (malloc(sizeof(struct LRU_node)));
						node->element = first->pagetable[first->pageTableIndex];

						if (head_LRU  != NULL && node->element == head_LRU->element) {
							free(node);
						}
						else {
							if (head_LRU == NULL){
								enqueue_LRU(node);
							}
							else{
								//printf("3\n");
								int temp_node_element = node->element;
								enqueue_LRU_front(node);
								int occurence = look_for_occurence_LRU(node);
								if (occurence == 1) { //meaning that we need to remove a node
									//printf("4\n");
									dequeue_first_occurence_LRU(node);
								}
						

							}
						}
					}

					int index_of_frameStore = (first->pagetable[first->pageTableIndex] *3) + first->offset; //pointer in frame store to get line of code
					char *line = mem_get_value_frame_store(index_of_frameStore);
					
					errCode = run2(line);

					first->currentNumberOfFileLineInFrame++;
					first->offset++;

					//check if a program has reached the end
					if (first->sizeOfFile == first->currentNumberOfFileLineInFrame){
						break;
					}
			
				}
				if (first == NULL) { //if queue empty, break
					
					break;
				}

				//dequeue program if we have finished executing it
				if (first->sizeOfFile == first->currentNumberOfFileLineInFrame){
					dequeue();
					continue;
				}

				// store all of node first into variables. we cannot use temp nodes for pointers
				PCBs* ne = first->next;
				int pid = first->PID;
				int currentOffset = first->offset;
				int currentFileLine = first->currentLineNumberInFile;
				int currentFrameLine = first->currentNumberOfFileLineInFrame;
				int pageTableIndex1 = first->pageTableIndex;
				int sizeFile = first->sizeOfFile;
				int slice = first->time_slice;
				
				int sizeOfPageTab = sizeof(first->pagetable) / sizeof(int);
				// initialize temporary variable
				PCBs* temp = (PCBs*) malloc(sizeof(PCBs));				

				temp->PID = pid;
				temp->offset = currentOffset;
				temp->currentLineNumberInFile = currentFileLine;
				temp->currentNumberOfFileLineInFrame = currentFrameLine;
				temp->pageTableIndex = pageTableIndex1;
				temp->sizeOfFile = sizeFile;
				temp->time_slice = slice;

				//copy page table
				memcpy(temp->pagetable, first->pagetable, sizeof(first->pagetable));

				//remove head and add it at the end
				dequeue();
				enqueue(temp);

			}
			empty_LRU_queue();
			
			mem_init_variable_Store(); // added this so that we don't have to call resetmem each time after an exec command.
			
			return errCode;

	
	} else if (strcmp(command_args[0], "echo")==0) { //added the echo command
		if (args_size != 2) return badcommand();
		return echo(command_args[1]);

	 } else if (strcmp(command_args[0], "my_ls")==0) {
		 if (args_size != 1) return badcommand();
		 
		 return my_ls();;
	} else return badcommand();
}

int help(){

	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
	return 0;
}

int resetmem(){
	mem_init_variable_Store();
	return 0;
}

//my_ls omits the special files starting with '.' as mentionned on ed
int my_ls(){ // cheek if the . files are allowed

	int numberOfFiles = countNumberOfFiles(); // this method is used to know how big of an array we need to create to store each file name in the directory
	char tmp [numberOfFiles]; // an array to store file names
	struct dirent *ptr;
	DIR *direction = opendir("."); //opens the current directory file
	char fileArray[numberOfFiles][100]; // create a 2d array with the assumption that the file names won't be larger than 100 characters
	int countForFiles =0;

	if (direction == NULL) {
        printf("Could not open current directory" );
        return 0;
    }
	
	while((ptr = readdir(direction)) != NULL) {
		if (ptr->d_name[0] != '.') { //omit the special files starting with '.'
			strcpy(fileArray[countForFiles], ptr->d_name);
			countForFiles ++;
		}
	}

	// used bubble sort to sort the file name in order. Compares character based on ASCII ordering
	for (int k = 0; k < numberOfFiles; k++) {
		for(int z = 0; z <numberOfFiles-k-1; z++) {
			if (strcmp(fileArray[z], fileArray[z+1] )> 0)
			{
				strcpy(tmp, fileArray[z]);
				strcpy(fileArray[z], fileArray[z+1]);
				strcpy(fileArray[z+1], tmp);
			}			
		}
	}

	//print the files in order
  	for(int i=0; i<numberOfFiles ;i++) {
    	printf("%s \n", fileArray[i]);
  	}

	closedir(direction);
	
	return 0;
}


int echo(char* word) {
	char temp[100] ="";
	char echoArr[100] ="";
	strcpy(temp,word);
	
	if(temp[0] == '$') { //if '$

		int j;
		int length_charArr = (sizeof(temp) / sizeof(char));

		for(j = 1; j < length_charArr; j ++) {
			echoArr[j-1] = temp[j]; //discard the '$' char
		}
		char* temp = mem_get_value(strdup(echoArr)); //get the value assossiated to the variable that we want to echo from memory.
		if (strcmp(temp, "Variable does not exist") ==0) {
			printf("\n"); //means that there is no such variable in memory. we display an empty line.
		}
		else {
			printf("%s\n", temp); 
		}
	}

	else { // if there is no '$' at beginning of STRING, then we just print the STRING
		printf("%s\n", temp);
	}
	return 0;
}

int quit(){
	printf("%s\n", "Bye!");
	system("rm -rf backing_storage"); //added this to close baclong storage
	exit(0);
}

int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return 3;
}

int badcommandTooManyTokens(){
	printf("%s\n", "Bad command: Too many tokens");
	return 1;
}
int badcommandTwoFilesSameName(){
	printf("Bad command: same file name\n");
	return 1;
}
int badcommandProgramsDoNotFit(){
	printf("Bad command: programs do not fit in memory\n");
	return 1;
}

int set(char* var, char* value){

	char *link = "="; // value stored at pointer link is "="
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);
	mem_set_value(var, value);

	return 0;

}

int print(char* var){
	printf("%s\n", mem_get_value(var)); 
	return 0;
}

//takes argument lineInFile which represents our pointer in a given program of name fileName
//lineInFrame represents where we are in the frame
void setFrameMemory(char* fileName, int lineInFile, int lineInFrame){

	char line[1000];
	char path[16+strlen(fileName)];
	strcpy(path, "backing_storage/");
	strcat(path, fileName);
	FILE *f = fopen(path, "r");

	// if for example we want line two, function will stop when lineInFile == current_line
	lineInFile += 1;
	int current_line = 1;
	bool keepReading = true;

	if(f == NULL){
		return;
	}

	do
	{
		memset(line, 0, sizeof(line));
		fgets(line, 999, f);

		if (feof(f)){
			
			break;
		}
		else if (current_line == lineInFile){
			keepReading = false;

		}
		
		current_line++;
		
		
	} 
	while(keepReading);
	// printf("lineInFrame: %d\n", lineInFrame);
	// printf("line :%s", line);
	set_frame_memory(line, lineInFrame);

	fclose(f);

	//set_frame_memory(line, )

}

//method to run scripts
int run(char* script, int start){
	char line[1000];

	FILE *p = fopen(script,"rt");  // the program is in a file
	if(p == NULL){
		//fclose(p);
		return badcommandFileDoesNotExist();
	}
	
	fgets(line,999,p);
	int count = start; // count number of words per script
	//we then load line by line in memory
	while(1){
		mem_set_script_in_memory(line, (count-1));

		memset(line, 0, sizeof(line)); // reset array line[] to zero

		if(feof(p)){
			break;
		}
		fgets(line,999,p);
		count++; // check if count will give right number
		if(count>999){
			fclose(p);
			return badcommandProgramsDoNotFit();
		}

	}
	fclose(p);
	return (count + 1); // count + 1 because we want to come to next memory address when iterating for a new program
}

//run parseInput and return an integer value
int run2(char* line){
	int errCode = 0;

	errCode = parseInput(line);

	return errCode;
}

// this method counts the number of file in the current directory
int countNumberOfFiles(){
	struct dirent *ptr;
	DIR *direction = opendir(".");
	int count = 0;

	if (direction == NULL) {
        printf("Could not open current directory" );
        return 0;
    }

	while((ptr = readdir(direction)) != NULL){
		if (ptr->d_name[0] != '.') { 
			count ++;		
		}	
	}

	closedir(direction);
	
	return count;
}

// this method counts the number of file in the backing_storage directory
int countNumberOfFilesForBackingStorage() {
	struct dirent *ptr;
	DIR *direction = opendir("backing_storage");
	int count = 0;

	if (direction == NULL) {
        printf("Could not open current directory" );
        return 0;
    }

	while((ptr = readdir(direction)) != NULL){
		if (ptr->d_name[0] != '.') { 
			count ++;		
		}	
	}

	closedir(direction);
	
	return count;
}

// counts number of lines in a file.
// Returns an array with size [number of files] and each element contains number of line for each file
int* countNoOfLines(int numberOfPrograms, char **arrayOfFilesNames){
	int *tempArr = malloc(numberOfPrograms);

	//count number of lines in a file. store in array[numberOfPrograms]
	for(int i = 1; i<= numberOfPrograms; i++){
		FILE *tempP = fopen(arrayOfFilesNames[i-1],"r"); 
		int count = 0;
		if(getc(tempP) == EOF){
			tempArr[i-1] = count;
			continue;
		}
		for (char j = getc(tempP); j != EOF; j = getc(tempP)){
			if (j == '\n') // Increment count if this character is newline
            count += 1;
		}
		tempArr[i-1] = count+1;
    	fclose(tempP);

	}

	return tempArr;
}

// queue

//add element to end of queue
void enqueue(PCBs *pcb){
	PCBs* current = pcb;
	if (rear != NULL){
		rear->next = current;
	}
	else{	
		first = current;

	}
	rear = current;
	rear->next = NULL;

}



//enqueue in front of queue (unconventional)
void enqueue_front(PCBs *pcb){
	PCBs* current = pcb;
	current->next = first;
	
	first = current;
	// printf("sizefirst=%d\n", first->size);
}

//at element in the queue, i.e not at the front nor at the tail
void enqueue_middle(PCBs* pcb){
	PCBs* current = pcb;
	current->next = first->next;
	first->next = current;
}

//remove head of queue
void dequeue() {
	if(first != NULL){

		// if first == rear, we have to remove both first and rear
		if (first == rear)
		{
			rear = NULL;
			free(rear);
		}

		//remove the head
		PCBs * current = first; 
		first = first->next;
		free(current);
	}
}



//queue for LRU

//add element to the back of the queue
void enqueue_LRU(struct LRU_node *node) {
	struct LRU_node *current = node;

	if(tail_LRU != NULL){
		tail_LRU->next = current;
	}
	else{
		head_LRU = current;
	}
	tail_LRU = current;
	tail_LRU->next = NULL;

}

// add element to the front of the queue
void enqueue_LRU_front(struct LRU_node *node){
	struct LRU_node *current = node;
	current->next = head_LRU;

	head_LRU = current;
}

// returns the element at the rear of queue
int peek_LRU_rear(){

	if(head_LRU != NULL){

		struct LRU_node  *current = head_LRU;
		while(current != NULL){
			
			if (current->next == NULL){
				return current->element;
			}
			current = current->next;
		}
	
	}

}

// remove the tail of the queue
void dequeue_LRU_rear() {

	if(head_LRU != NULL){
		struct LRU_node  *current = head_LRU;
		while(current != NULL){

			if (current->next == tail_LRU){

				tail_LRU = current;

				tail_LRU->next = NULL;
				
				free(tail_LRU->next);
				return;
			}
			current = current->next;
		}

	}
}

void dequeue_first_occurence_LRU(struct LRU_node *node) {
	if(head_LRU != NULL) {
		struct LRU_node  *current = head_LRU;

		while(current != NULL){

			if (current->next == tail_LRU && tail_LRU->element == node->element){

				tail_LRU = current;	

				tail_LRU->next = NULL;
					
				free(tail_LRU->next);
				return;
			}

			else if (current->next->element == node->element) {
				struct LRU_node *temp = current->next;
				current->next = current->next->next;
				temp = NULL;
				free(temp);
				return;

			}
			else{

				current = current->next;
			}
		}
		
	}
}


// if two node have same frame number in queue, remove the second one
// return 1 if true
// return 0 if false
int look_for_occurence_LRU(struct LRU_node *node){
	int count = 0;
	if(head_LRU != NULL) {
		struct LRU_node  *current = head_LRU;	
		while (current != NULL){
			if (current->element == node->element){
				count++;
			}
			if(count == 2){
				return 1;
			}
			current = current->next;
		}
	}
	return 0;
}

//prints the queue
void print_LRU_queue(){
	if (head_LRU != NULL){

		struct LRU_node  *current = head_LRU;
		printf("LRU queue:");
		while(current != NULL){
			printf("%d ",current->element);
			current = current->next;
		}
		printf("\n");
	}
}

// empty and free memory allocated, at the end of the execution
void empty_LRU_queue() {
	if (head_LRU != NULL){

		struct LRU_node  *current = head_LRU;
		while (current != NULL){
			if (head_LRU == tail_LRU) {
				tail_LRU = NULL;
				free(tail_LRU);
			}
			struct LRU_node *temp = head_LRU;
			head_LRU = head_LRU->next;
			current = current->next;
			free(temp);
			
		}
	}
}