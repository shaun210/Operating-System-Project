Author: Shaun Soobagrah
McGill ID: 260919063

//For assignment 1
I used the starter code.

1. enhance set command:
I edited part of the interpreter method in the interpreter class.
If the user types a set command without a STRING value, it is considered a bad command.

2. echo command:
All the changes to the code have been done in the interpreter class

3. batch mode
If there are empty lines between commands in the text files, it will print unknown command. But after executing the last command,
it will return to interactive mode.

4. my_ls
Oll the changed were made in the interpreter class where I added a new command. 
Omits the special files starting with '.'.
Compares character based on ASCII ordering. 

5. one liner
All the changes have been made in the parseInput method in the shell class.
I assumed that the last command won't end with ';'
There must be a white space after ';' for the method to work.


//For assignment 2

I used my code for A1 as starter code

1.2.1:

Most of the modification have been done in the interpreter class and also, some method from shellmemory class were called.
This part was also coded in a way where is forms the basis for running multiple program. But since for the run command there is only 1 program as argument, 
the for-loop will run only once.
run2 is a method that runs 1 line of code and return an integer.
The method run returns an integer, which is used to determine how many lines of code there are in a program.
I created a struct which represents the PCBs.
The queue uses pointers to link entries.

1.2.2

Most of the modification have been done in the interpreter class.
Mostly like 1.2.1, execpt now we consider policy.

1.2.3

In this queue, you can add at the front and in between two entries. The methods are at the end of the interpreter C file.

1.2.4

I commented the code

//assignment3//

I used my code as starter code

Apart from adding the frame store and variable store, the rest of the modification to the code was done in the interpreter class.

Note that I assumed that:
Let's say that a process is cut short due to page fault, and it still has a 1 line to execute in its "time slice". But now it will be placed to the back of the queue. The next time that we have to execute that process, only 1 line will be executed, that is because it had 1 line to be executed in its "time slice"

For the exec command, after adding 2 page for each program(if necessary), there is a pointer that points to the current line in the file that has been added last.
And there is another pointer, currentNumberOfFileLineInFrame, to know how many lines from each files have been added to the frame. Those are used for page fault. They
are struct fields of the struct that defines each PCB. The rest has been commented in the code.
If a frame cannot be filled due to lack of lines, i.e, the file runs out of line, "void" are added to that frame for the remaining frame lines.
In my frameStore, the program are added not continuously. i.e, program 1 will first be added at frame 0 and frame 3 if we are executing 3 program for example.

There is also the LRU_queue which is a queue data structure that keeps track of which frame has been used last. Again, the steps have been commented.
In the LRU_queue, the least recently used frame number is always to the right(rear of queue)

The run command is nearly similar to the exec command.

