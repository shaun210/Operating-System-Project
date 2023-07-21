#include<stdlib.h>
#include<string.h>
#include<stdio.h>


struct memory_struct{
	char *var;
	char *value;
};


// create a new struck for OS shell memory to store programs
// the OS shell memory is for loading the scripts. It loads all the files into OS shell memory continuously
//assignment 2
struct OS_memory{
	char *line;
};

struct frame_store {
	char *line;
};

struct variable_store {
	char *variable_name;
	char *value;
};

//int frame_store_size = 6;
int variable_store_size = VARMEMSIZE;
int frame_store_size = FRAMESIZE;

struct OS_memory OSmemory[1000]; // this is for loading entire script
struct memory_struct shellmemory[1000];
struct frame_store frame_store[FRAMESIZE]; 
struct variable_store variableStore[VARMEMSIZE]; // change this . change init loop also


// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}


char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}


// Shell memory functions
void mem_init(){
	printf("Frame Store Size = %d; Variable Store Size = %d\n", frame_store_size,variable_store_size );
	int i;
	for (i=0; i<1000; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
// e.g set a 2
void mem_set_value(char *var_in, char *value_in) {
	int i;

	for (i=0; i<variable_store_size; i++){
		if (strcmp(variableStore[i].variable_name, var_in) == 0){
			variableStore[i].value = strdup(value_in); //strdup will terminate the string with "\0"
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<variable_store_size; i++){
		if (strcmp(variableStore[i].variable_name, "none") == 0){
			variableStore[i].variable_name = strdup(var_in);
			variableStore[i].value = strdup(value_in);			
			return;
		} 

	}
	return;

}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;

	for (i=0; i<variable_store_size; i++){
		if (strcmp(variableStore[i].variable_name, var_in) == 0){

			return strdup(variableStore[i].value);
		} 
	}
	return "Variable does not exist";

}

//assignment 2  //
// for OS_shell memory

void mem_init_OS_shell(){

	int i;
	for (i=0; i<1000; i++){		
		OSmemory[i].line = "none"; 
	}
}

// method to store all the programs into memory
void mem_set_script_in_memory(char *line, int line_number){
	OSmemory[line_number].line = strdup(line);

}

// return the line at the given index in OS memory shell
char *mem_get_value_OS_shell(int index) { 
	return strdup(OSmemory[index].line);

}

//method to clear out the memory array from a given index to another
void clean_script_from_memory(int start, int end){
	for (int i = start; i < end; i++){
		memset(OSmemory[i].line, 0, strlen(OSmemory[i].line));
	}

}

//method to clear a single element from memory array
void clean_single_line_in_memory(int i) {
	memset(OSmemory[i].line, 0, strlen(OSmemory[i].line));
}





//assignent 3 // 
//for frame store

void mem_init_frame_Store(){

	int i;
	for (i=0; i<frame_store_size; i++){	 //change this to XVAL	
		frame_store[i].line = "none"; 
	}
}

void set_frame_memory(char *line, int index) {
	if (strcmp(line,"none") == 0){
		frame_store[index].line = "none";
	}
	else{
		frame_store[index].line = strdup(line);
	}
	
	
}

// returns a line from frame store using the given index
char *mem_get_value_frame_store(int index) {
	return strdup(frame_store[index].line);
}

//method to look for empty frame in framestore
//iterate till we see a "none".
// then we return the frame number
int look_for_empty_frame() {

	for (int i = 0; i < frame_store_size; i++) {
		if (strcmp(frame_store[i].line, "none") == 0){

			return (i/3);
		}
	}
	return -1;

}


void printFrameStore(){
	for (int i = 0; i < frame_store_size; i++)
	{
		printf("%s\n", frame_store[i].line);	
	}
	
}

void mem_init_variable_Store(){

	for (int i=0; i < variable_store_size; i++){	 //change this to XVAL	
		variableStore[i].value = "none"; 
		variableStore[i].variable_name ="none";
	}
}