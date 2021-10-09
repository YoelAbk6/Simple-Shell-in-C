/*EX4
 * Simple shell in linux written in c,supports ctrl-z and fg. Doesn't support 'cd' command.
 * run till 'done' is the input - prints the command and pipes number at the end
 */

#include <stdio.h>

#include <unistd.h>

#include <pwd.h>

#include <string.h>

#include <stdlib.h>

#include <wait.h>

#include <signal.h>

#define SENTENCE_LEN 512
#define PATH_MAX 512

struct Commands {
    char ** separatedArgs;
    struct Commands * next;
};

void printPrompt();

char * getUserName();

int countArguments(const char * sentence);

void parseString(const char sentence[], char ** parsedStr, struct Commands * head);

void freeArr(struct Commands * head);

void exeCommand(char ** command, struct Commands * head);

void printStatistics(int numOfCommands, int totalPipeNum);

int isDone(struct Commands * head, int cmdLen);

int separatePipes(struct Commands ** head, char * commandLine);

void pipeForkAndExe(struct Commands * head, int pipeNum);

void sigHandler(int sigNum);

int isFg(struct Commands * head, int cmdLen);

pid_t paused_pid, last_pid;

int main() {
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR) { // Ignores Ctrl-Z in the father process
        perror("signal ERR");
        exit(1);
    }
    if (signal(SIGCHLD, sigHandler) == SIG_ERR) { // Handles son process statues changes
        perror("signal ERR");
        exit(1);
    }
    char commandLine[SENTENCE_LEN];
    int numArguments = 0, totalPipeNum = 0, commandLen, pipeNum;
    struct Commands * head;
    while (1) {
        printPrompt();
        if (fgets(commandLine, SENTENCE_LEN, stdin) != NULL) {
            commandLen = countArguments(commandLine);
            if (commandLen > 0) {
                pipeNum = separatePipes( & head, commandLine);
                numArguments++;
                totalPipeNum += pipeNum;
                if (pipeNum == 0 && isDone(head, commandLen)) {
                    printStatistics(--numArguments, totalPipeNum);
                    exit(0);
                } else {
                    if (!isFg(head, commandLen))
                        pipeForkAndExe(head, pipeNum);
                }
            }
        }
    }
}

//Prints prompt in the following format user@current dir>, if getUserName or getcwd fails -
// print NULL in their place
void printPrompt() {
    char * userName = getUserName();
    char currentDir[PATH_MAX];

    getcwd(currentDir, sizeof(currentDir));
    printf("%s@%s>", userName, currentDir);
}

//Returns the username as a char*, if getpwuid fails - return NULL
char * getUserName() {
    uid_t uid = geteuid();
    struct passwd * pw = getpwuid(uid);
    if (pw)
        return pw -> pw_name;
    else {
        return NULL;
    }
}

//receives a sentence and returns the number of words in it, ignoring blank spaces
int countArguments(const char * sentence) {
    int i = 0, wordCounter = 0;
    while (sentence[i] != '\n' && sentence[i] != '\0') {
        if (sentence[i] != ' ' && (sentence[i + 1] == ' ' || sentence[i + 1] == '\n' || sentence[i + 1] == '\0'))
            wordCounter++;
        i++;
    }
    return wordCounter;
}

/*receives a sentence as a char[] and a char**
 *assign dynamically the sentence words into the char**.
 * text between single or double quotes is considered as one string, and quotes are ignored
 */
void parseString(const char sentence[], char ** parsedStr, struct Commands * head) {
    char tmpWord[SENTENCE_LEN];
    int tmpIndex = 0, parsedIndex = 0;
    int quot = 0;
    for (int i = 0; i <= strlen(sentence); i++) {
        if (sentence[i] == '"' || sentence[i] == '\'') {
            quot ^= 1; //XOR - toggle between true and false
            i++;
        }
        if (sentence[i] != '\n' && sentence[i] != '\0' && (quot || sentence[i] != ' ')) {
            tmpWord[tmpIndex] = sentence[i];
            tmpIndex++;
        }
            //End of word
        else if (((sentence[i] == ' ' && !quot) || sentence[i] == '\n' || sentence[i] == '\0') && tmpIndex > 0) {
            tmpWord[tmpIndex] = '\0';
            parsedStr[parsedIndex] = (char * ) malloc((strlen(tmpWord)) + 1);
            if (parsedStr[parsedIndex] == NULL) {
                fprintf(stderr, "malloc failed");
                freeArr(head);
                exit(1);
            }
            strcpy(parsedStr[parsedIndex], tmpWord);
            parsedIndex++;
            tmpIndex = 0;
        }
    }
}

//Frees the list of commands
void freeArr(struct Commands * head) {
    struct Commands * curr = head;
    while (curr) {
        head = head -> next;
        int i = 0;
        while (curr -> separatedArgs[i])
            free(curr -> separatedArgs[i++]);
        free(curr -> separatedArgs);
        free(curr);
        curr = head;
    }
}

// Receives a command to execute as an array** and execute it using execvp
void exeCommand(char ** command, struct Commands * head) {
    if (strcmp(command[0], "cd") == 0) {
        fprintf(stderr, "command not supported (YET)\n");
        freeArr(head);
        exit(0);
    } else if (execvp(command[0], command) == -1) {
        perror("command not found");
        freeArr(head);
        exit(1);
    }
}

//Prints the total num of commands and the total number of pipes
void printStatistics(int numOfCommands, int totalPipeNum) {
    printf("Number of commands: %d\n", numOfCommands);
    printf("Number of pipes: %d\n", totalPipeNum);
    printf("See you Next time !\n");
}

//Checks if 'done' is the input
int isDone(struct Commands * head, int cmdLen) {
    if (strcmp(head -> separatedArgs[0], "done") == 0 && cmdLen == 1) {
        freeArr(head);
        return 1;
    }
    return 0;
}

/**Checks if 'fg' is the input, if it is, sends SIGCONT to paused_pid
,sets the father to wait and returns 1. If not - returns 0.
 **/
int isFg(struct Commands * head, int cmdLen) {
    if (strcmp(head -> separatedArgs[0], "fg") == 0 && cmdLen == 1) {
        if (kill(paused_pid, SIGCONT) == -1) {
            printf("%d\n", paused_pid);
            perror("ERR");
        } else waitpid(paused_pid, NULL, WUNTRACED);
        return 1;
    }
    return 0;
}

/*Receives a pointer to the commands list head address and a command line as a char*, separates the different commands between the pipes
 * using parseString, returns the number of pipes.
 */
int separatePipes(struct Commands ** head, char * commandLine) {
    char * command = strtok(commandLine, "|");
    int i = 0, commandLen;
    * head = NULL;
    struct Commands * curr = NULL;
    while (command != NULL) {
        commandLen = countArguments(command);
        if ( * head == NULL) {
            * head = (struct Commands * ) malloc(sizeof(struct Commands));
            ( * head) -> next = NULL;
            ( * head) -> separatedArgs = (char ** ) malloc((commandLen + 1) * sizeof(char * ));
            if (( * head) -> separatedArgs == NULL) {
                fprintf(stderr, "malloc failed");
                freeArr( * head);
                exit(1);
            }
            curr = * head;
        } else {
            curr -> next = (struct Commands * ) malloc(sizeof(struct Commands));
            curr = curr -> next;
            curr -> next = NULL;
            curr -> separatedArgs = (char ** ) malloc((commandLen + 1) * sizeof(char * ));
            if (curr -> separatedArgs == NULL) {
                fprintf(stderr, "malloc failed");
                freeArr( * head);
                exit(1);
            }
        }
        curr -> separatedArgs[commandLen] = NULL;
        parseString(command, curr -> separatedArgs, * head);
        command = strtok(NULL, "|");
        i++;
    }
    return (--i);
}

/*The functions receive a list of commands with the number of pipes between them.
 * The function then execute each command, redirecting its input/output from/to the STD or the prev/next process corresponding
 * the number of pipes
 */
void pipeForkAndExe(struct Commands * head, int pipeNum) {
    int pipeFD[2];
    pid_t pid;
    int FDIn = STDIN_FILENO;
    struct Commands * curr = head;
    while (pipeNum >= 0) {
        if (pipe(pipeFD) == -1) {
            perror("cannot open pipe");
            freeArr(head);
            exit(1);
        }
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            freeArr(head);
            exit(1);
        } else if (pid == 0) {
            if (signal(SIGTSTP, sigHandler) == SIG_ERR) {
                perror("signal ERR");
                freeArr(head);
                exit(1);
            }
            if (dup2(FDIn, STDIN_FILENO) == -1) {
                perror("dup 2 failed");
                freeArr(head);
                exit(1);
            }
            if (pipeNum - 1 >= 0) {
                if (dup2(pipeFD[1], STDOUT_FILENO) == -1) {
                    perror("dup 2 failed");
                    freeArr(head);
                    exit(1);
                }
            }
            close(pipeFD[0]);
            exeCommand(curr -> separatedArgs, head);
        } else {
            last_pid = pid;
            waitpid(pid, NULL, WUNTRACED);
            close(pipeFD[1]);
            FDIn = pipeFD[0];
            pipeNum--;
            curr = curr -> next;
        }
    }
    freeArr(head);
}

//Receives a signal number and acts in corresponding with it
void sigHandler(int sigNum) {
    if (sigNum == SIGCHLD)
        waitpid(-1, NULL, WNOHANG); // terminates zombie processes
    if (sigNum == SIGTSTP) { //Catches the SIGTSTP the first time in order to save the pid and then raises the SIGTSTP
        paused_pid = last_pid;
        raise(SIGTSTP);
    }
}