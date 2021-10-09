# Simple-Shell-in-C
Simple shell in linux, supporting Ctrl-z and fg but not 'cd' command
Authored by YoelAbk6 for the fourth exercise in OP course

==Description==

This exercise is a simulation of a simple shell in Linux.
The user can enter commands, including arguments, and execute them using the program.

The program runs in a loop that ends when the user will input 'done'.
If ctrl-z is entered, the current son process suspends, if there is not one running - nothing happens. When a process is suspended and 'fg' is sent from the user, the last suspended process is back to the foreground and the main process will wait for it to finish or be paused again. Again, if there is no suspended process, nothing happens.

statistics will be printed at the program end.

functions:

contains 7 main functions:

1. countArguments - Receives a sentence, count the number of words in it and returns it

2. parseString- Receives a sentence and a two-dimensional char array. Splitting the words between every blank space and assigns them into the two dim array.
   Text between quotes will be considered as one word and will be entered into one cell of the array.
   The quotes won't be copied.

3. exeCommand - Receives a command to execute as an array** and execute it using execvp.
   The command 'cd' which is not supported yet, is checked in this function, so "command not supported (YET)" is
   printed if entered.

4. separatePipes - Receives a pointer to the commands list head address and a command line as a char*, separates the different commands between the pipes
   using parseString and strtok with "|" as delimiter and adds them to the list. Returns the number of pipes.

5. pipeForkAndExe - The functions receive a list of commands with the number of pipes between them. The function then executes each command, redirecting its    input/output from/to the STD or the prev/next process corresponding the number of pipes


6. isFg - Receives the list of commands and the first command length and checks if the command is 'fg'. If it is, sends SIGCONT using kill to the current paused_pid and set the father process to wait using waitpid with 'WUNTRACED'.

7. sigHandler - Receives a signal number as an int. If the signal is SIGCHLD, terminates zombie processes using waitpid with WNOHANG. If the signal is SIGTSTP, save the current process pid to paused_pid and raises SIGTSTP. 


==Program Files==

ex4.c - containing the main and all the functions
README.txt

==How to compile?==

compile: gcc ex4.c -o ex4
run: ./ex4


==Input:==

Users input - a string representing a command (legal or illegal)

==Output:==

ex4 - Prints the system output for the commands executed, or a corresponding ERR message when a command doesn't exist or supported (like 'cd'). Also prints statistics when the program ends (Total num of commands, total num of pipes).
