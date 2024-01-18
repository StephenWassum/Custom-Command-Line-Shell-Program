# My Shell Project by Stephen Wassum

## Running the Program

To run this program open an terminal at the location of mysh.c and other files within this folder.
If the program is contained in a tar file, open this first. Once open run the make command in the terminal.
The Makefile will compile mysh.c. Then use the command ./mysh to run the program in interactive mode or use
the command ./mysh with a txt file as argument ./mysh argument.txt to open in batch mode.

## mysh.c

**File Overview:**

This program implements a simple command-line shell, similar to bash or zsh. This program, mysh, runs in two 
modes, interactive and batch, both read sequences of commands. This program runs in interactive mode when 
given no arguments. In this case, mysh reads commands from standard input. In batch mode, this program
opens a specified file and interprets the lines of that file as commands. 

## Project Implementation

### Main function

The main function simply determines whether 1 or no arguments have been passed in through the terminal and
runs the program in either interactive or batch mode respectivley.

```c
int main(int argc, char *argv[]) {
    if (argc == 2) {
        batchMode(argv[1]);
    } else {
        interactiveMode();
    }
    return 0;
}


```

### Interactive mode

Interactive mode runs if no commands are given in the terminal. Interactive mode writes the prompt to std output
and reads commands given by the user in std input. Interactive mode uses a while loop that can only be ended
by the user to detect commands. It reads these commands and tokenizes them performing an additional check to see
if the command is empty. If so, it continues to the next iteration of the loop. Interactive mode then
performs several operations. The first is to check whether a wildcard expansion is specified in the token. 
If so the  token is flagged and wildcard expansion is performed. Next, the code checks for redirection tokens 
and flags them if encountered. Then, their are checks to determin whether the command token is a built in 
command, if so the built in functions of mysh are called. Finally, after all those checks, the execute command
function is called with checks for redirection and an arg list that has been edited if based on whether
wildcard or redirection tokens were encountered.

```c
int interactiveMode(){
    char command[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    char *token;
    int i;

    printf("Welcome to my shell!\n");
    fflush(stdout);

    while (1) {
        printf("mysh> ");
        fflush(stdout);

        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            if (feof(stdin)) {
                printf("\nExit\n");
                break;
            } else {
                perror("fgets");
                continue;
            }
        }

        size_t length = strlen(command);
        if (length > 0 && command[length - 1] == '\n') {
            command[length - 1] = '\0';
        }

        if (strncmp(command, "then ", 5) == 0) {
            if (lastExecution != 0) continue;
            memmove(command, command + 5, strlen(command) - 4);
        } else if (strncmp(command, "else ", 5) == 0) {
            if (lastExecution == 0) continue;
            memmove(command, command + 5, strlen(command) - 4);
        }

        i = 0;
        token = strtok(command, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " "); 
        }
        args[i] = NULL; 

        if (args[0] == NULL) {
            continue; 
        }

        char *argsWildcard[MAX_ARGS];
        int argsWildcardCount = 0;
        int redirectionFound = 0; 

        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "<") == 0 || strcmp(args[j], ">") == 0) {
                redirectionFound = 1; 
                argsWildcard[argsWildcardCount++] = strdup(args[j]);
            } else if (redirectionFound) {
                argsWildcard[argsWildcardCount++] = strdup(args[j]);
                redirectionFound = 0; 
            } else {
                expandWildcard(args[j], argsWildcard, &argsWildcardCount);
            }
        }
        argsWildcard[argsWildcardCount] = NULL; 

        int inputRedirectionFlag = 0, outputRedirectionFlag = 0;
        char *inputFile = NULL, *outputFile = NULL;

        char *argsRedirection[MAX_ARGS]; 
        int argsRedirectionCount = 0;

        for (int j = 0; argsWildcard[j] != NULL; j++) {
            if (strcmp(argsWildcard[j], "<") == 0 && argsWildcard[j + 1] != NULL) {
                inputRedirectionFlag = 1;
                inputFile = argsWildcard[j + 1];
                j++; 
        } else if (strcmp(argsWildcard[j], ">") == 0 && argsWildcard[j + 1] != NULL) {
                outputRedirectionFlag = 1;
                outputFile = argsWildcard[j + 1];
                j++; 
        } else {
                argsRedirection[argsRedirectionCount++] = argsWildcard[j];
            }
        }
        argsRedirection[argsRedirectionCount] = NULL; 

        if (strcmp(argsRedirection[0], "cd") == 0) {
            lastExecution = myCd
        (args[1]);
        } else if (strcmp(argsRedirection[0], "pwd") == 0) {
            lastExecution = myPwd
        ();
        } else if (strcmp(argsRedirection[0], "which") == 0) {
                lastExecution = myWhich(args[1]);
        } else if (strcmp(argsRedirection[0], "exit") == 0) {
            myExit();
        } else{


        lastExecution = executeCommand(argsRedirection, inputRedirectionFlag, inputFile, outputRedirectionFlag, outputFile);
        }
    }

    return 0;
}

```

### Batch mode

Batch mode functions essentially the same as interactive mode except it reads its commands from lines within
the file that was passed in as an argument.

```c
void batchMode(char *filename) {
    char command[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    char *token;
    int i;

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    while (fgets(command, MAX_COMMAND_LENGTH, file)) {

        
        size_t length = strlen(command);

        if (length == 0 || (length == 1 && command[0] == '\n')) {
            continue;  
        }

        if (command[length - 1] == '\n') {
            command[length - 1] = '\0';
        }

        if (length > 0 && command[length - 1] == '\n') {
            command[length - 1] = '\0';
        }

        if (strncmp(command, "then ", 5) == 0) {
            if (lastExecution != 0) continue;
            memmove(command, command + 5, strlen(command) - 4);
        } else if (strncmp(command, "else ", 5) == 0) {
            if (lastExecution == 0) continue;
            memmove(command, command + 5, strlen(command) - 4);
        }

        i = 0;
        token = strtok(command, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " "); 
        }
        args[i] = NULL; 

        if (args[0] == NULL) {
            continue; 
        }

        char *argsWildcard[MAX_ARGS];
        int argsWildcardCount = 0;
        int redirectionFound = 0; 

        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "<") == 0 || strcmp(args[j], ">") == 0) {
                redirectionFound = 1; 
                argsWildcard[argsWildcardCount++] = strdup(args[j]);
            } else if (redirectionFound) {
                argsWildcard[argsWildcardCount++] = strdup(args[j]);
                redirectionFound = 0; 
            } else {
                expandWildcard(args[j], argsWildcard, &argsWildcardCount);
            }
        }
        argsWildcard[argsWildcardCount] = NULL;  

        int inputRedirectionFlag = 0, outputRedirectionFlag = 0;
        char *inputFile = NULL, *outputFile = NULL;

        char *argsRedirection[MAX_ARGS]; 
        int argsRedirectionCount = 0;

        for (int j = 0; argsWildcard[j] != NULL; j++) {
            if (strcmp(argsWildcard[j], "<") == 0 && argsWildcard[j + 1] != NULL) {
                inputRedirectionFlag = 1;
                inputFile = argsWildcard[j + 1];
                j++; 
        } else if (strcmp(argsWildcard[j], ">") == 0 && argsWildcard[j + 1] != NULL) {
                outputRedirectionFlag = 1;
                outputFile = argsWildcard[j + 1];
                j++; 
        } else {

                argsRedirection[argsRedirectionCount++] = argsWildcard[j];
            }
        }
        argsRedirection[argsRedirectionCount] = NULL; 

        if (strcmp(argsRedirection[0], "cd") == 0) {
            lastExecution = myCd
        (args[1]);
        } else if (strcmp(argsRedirection[0], "pwd") == 0) {
            lastExecution = myPwd
        ();
        } else if (strcmp(argsRedirection[0], "which") == 0) {
                lastExecution = myWhich(args[1]);
        } else if (strcmp(argsRedirection[0], "exit") == 0) {
            myExit();
        } else{


        lastExecution = executeCommand(argsRedirection, inputRedirectionFlag, inputFile, outputRedirectionFlag, outputFile);
        }
      

    }

    fclose(file);
}

```

### Wildcard Expansion

The wildcard expansion consists of three functions. The main expansion function reads over the files in a directory
and checks if their prefixs and suffixs matches the command arg. If they are these files are added to the argument 
list. Wildcard expanison uses two helper functions, a function to split tokens and a wildcard matcher. The first 
splits tokens where a wildcard * character is found. The second uses the prior split to check if files in the directory
also have the prefixes and suffixes determined by the split.

```c
void expandWildcard(char *token, char *argsWildcard[], int *argCount) {


    if (strchr(token, '*') == NULL) {
        argsWildcard[*argCount] = strdup(token); 
        (*argCount)++;
        return;
    }

    DIR *dir;
    struct dirent *entry;
    char *directory, *prefix, *suffix;
    splitToken(token, &directory, &prefix, &suffix); 

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");

        free(directory);
        free(prefix);
        free(suffix);

        return;
    }

    int wildcardMatches = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (matchWildcard(entry->d_name, prefix, suffix)) {
            wildcardMatches = 1;
            char *currentDirectory;

            if (strcmp(directory, ".") == 0) {
                currentDirectory = strdup(entry->d_name);
            } else {
                int pathLen = snprintf(NULL, 0, "%s/%s", directory, entry->d_name) + 1;
                currentDirectory = malloc(pathLen);

                if (currentDirectory == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }

                snprintf(currentDirectory, pathLen, "%s/%s", directory, entry->d_name);
            }

            argsWildcard[*argCount] = currentDirectory;
            (*argCount)++;
        }
    }

    if (!wildcardMatches) {
        argsWildcard[*argCount] = strdup(token);
        (*argCount)++;
    }

    closedir(dir);


    free(directory);
    free(prefix);
    free(suffix);
}

```

```c

void splitToken(const char *token, char **directory, char **prefix, char **suffix) {
    char *wildcard = strchr(token, '*');
    if (!wildcard) {
        *directory = strdup(".");
        *prefix = strdup(token);
        *suffix = strdup("");
        return;
    }

    char *slash = strrchr(token, '/');
    if (slash && slash < wildcard) {
        int directoryLen = slash - token + 1;
        *directory = strndup(token, directoryLen);
        *prefix = strndup(slash + 1, wildcard - slash - 1);
    } else {
        *directory = strdup("."); 
        *prefix = strndup(token, wildcard - token);
    }

    *suffix = strdup(wildcard + 1);
}
```

```c
int matchWildcard(const char *filename, const char *prefix, const char *suffix) {

    if (prefix[0] == '\0' && filename[0] == '.') {
        return 0;
    }

    size_t prefixLen = strlen(prefix);
    size_t suffixLen = strlen(suffix);
    size_t fileLen = strlen(filename);

    if (strncmp(filename, prefix, prefixLen) != 0) {
        return 0;
    }

    if (suffixLen > 0 && (fileLen < suffixLen || strcmp(filename + fileLen - suffixLen, suffix) != 0)) {
        return 0;
    }

    return 1;
}

```


### Built in Commands

mysh has 4 built in commands. These commands are: cd, pwd, which, and exit. cd uses chdir() to change its own 
directory and prints an error messge if given the wrong directory. pwd prints the current working directory
to standard output using getcwd() function. The Which program takes an argument and prints the path
that mysh would use for a command. If given a built in command nothing is printed, or a non exitent command
nothing is printed. Finally, exit simply terminates a program.

```c
int myCd(char *path) {
    if (chdir(path) != 0) {
        perror("cd error");
        return -1; 
    }
    return 0; 
}

```

```c

int myPwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0; 
    } else {
        perror("pwd error");
        return -1; 
    }
}
```

```c
int myWhich(const char *command) {
    char *location = findPath(command);

    if (location != NULL) {
        printf("%s\n", location);
        return 0;
    } else {
        printf("%s not found\n", command);
        return -1;
    }
}

```

```c

void myExit() {
    exit(0);
}
```

### Command Path Function

This helper function resolves the the full path of a command. It first checks if a user has specified a path
and uses that, otherwiase it searches the following directories for the command and access it if found: 
{"/usr/local/bin", "/usr/bin", "/bin"}.

```c
char *findPath(const char *command) {

    if (strchr(command, '/') != NULL) {
        return strdup(command);
    }

    static const char *searchLocation[] = {"/usr/local/bin", "/usr/bin", "/bin"};
    static char location[1024];

    for (int i = 0; i < 3; ++i) {
        snprintf(location, sizeof(location), "%s/%s", searchLocation[i], command);
        if (access(location, X_OK) == 0) {
            return location;
        }
    }
    return NULL; 
}

```

### Execute Command

The execute command function first performs two checks to determine whether a pipe or redirection token
has been specified. If so the functions execute piped command, or redirection are returned. Otherwise
a fork is executed. In the child process, the command path function is called to resolve the path
and execv is called. The parent function waits for the child process to finsh then returns
the exit status of the child process.


```c
int executeCommand(char *args[], int inputRedirectionFlag, char *inputFile, int outputRedirectionFlag, char *outputFile) {

    int i = 0;
    int pipePlacement = -1;
    char *args1[256];  
    char *args2[256];  

    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            pipePlacement = i;
            break;
        }
        i++;
    }

    if (pipePlacement != -1) {
        memcpy(args1, args, pipePlacement * sizeof(char *));
        args1[pipePlacement] = NULL;
        memcpy(args2, &args[pipePlacement + 1], (256 - pipePlacement - 1) * sizeof(char *));

        return executePipedCommand(args1, args2, inputRedirectionFlag, inputFile, outputRedirectionFlag, outputFile);

    } else {

        if (inputRedirectionFlag || outputRedirectionFlag) {
            return redirection(inputRedirectionFlag, inputFile, outputRedirectionFlag, outputFile, args);

        } else {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                char *commandPath = findPath(args[0]);
                if (commandPath == NULL) {
                    fprintf(stderr, "Command not found: %s\n", args[0]);
                    exit(EXIT_FAILURE);
                }

                execv(commandPath, args);
                perror("execv");
                exit(EXIT_FAILURE);
            
            }  else {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status); 
                }

                return -1; 
            }
        }
    }
    return -1;
}

```

### Redirection
The redirection function utilizes a fork, and then checks for input or output redirection flags in the child process.
The child process then utilizes dup2 so that the program knows to read or output to the specified file. Finally, the
commmand path function is called and execv is called. The Parent process waits for the termination fo the child 
process.

```c
int redirection(int inputRedirectionFlag, char* inputFile, int outputRedirectionFlag, char* outputFile, char* args[]) {

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {  
        if (inputRedirectionFlag) {
            
            if (access(inputFile, R_OK) == -1) {
                perror("access input file");
                exit(EXIT_FAILURE);
            }
            
            int inputDescriptor = open(inputFile, O_RDONLY);
            if (inputDescriptor == -1) {
                perror("open input file");
                exit(1);
            }
            dup2(inputDescriptor, 0); 
            close(inputDescriptor); 
        }

        if (outputRedirectionFlag) {
            int outputDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0640);
            if (outputDescriptor == -1) {
                perror("open output file");
                exit(1);
            }
            dup2(outputDescriptor, 1);  
            close(outputDescriptor);
        }

        char *commandPath = findPath(args[0]);
        if (commandPath == NULL) {
            fprintf(stderr, "Command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }

        execv(commandPath, args);
        perror("execv");  
        exit(EXIT_FAILURE);

    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
}

```

### Execute Piped Command

This function executes a piped command. It creates a fork and runs two child processes, for each command
specified. 

```c
int executePipedCommand(char *args1[], char *args2[], int inputRedirectionFlag, char *inputFile, int outputRedirectionFlag, char *outputFile) {
    
    int pipeDescriptor[2];

    if (pipe(pipeDescriptor) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {  
        if (!inputRedirectionFlag || strcmp(args2[0], "bar") != 0) {
            close(pipeDescriptor[0]); 
            dup2(pipeDescriptor[1], STDOUT_FILENO); 
            close(pipeDescriptor[1]);
        } else {
            close(pipeDescriptor[0]); 
            close(pipeDescriptor[1]);
        }
    
        char *commandPath = findPath(args1[0]);
        execv(commandPath, args1);
        perror("execv");
        exit(1);

    } else {
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("fork");
            exit(1);
        }

        if (pid2 == 0) {  
            if (inputRedirectionFlag && strcmp(args2[0], "bar") == 0) {
                int inputDescriptor = open(inputFile, O_RDONLY);
                if (inputDescriptor == -1) {
                    perror("open input file");
                    exit(1);
                }
                dup2(inputDescriptor, STDIN_FILENO);
                close(inputDescriptor);
            } else {
                close(pipeDescriptor[1]);  
                dup2(pipeDescriptor[0], STDIN_FILENO); 
                close(pipeDescriptor[0]);
            }

            if (outputRedirectionFlag) {
                int outputDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0640);
                if (outputDescriptor == -1) {
                    perror("open output file");
                    exit(1);
                }
                dup2(outputDescriptor, STDOUT_FILENO);
                close(outputDescriptor);
            }

            char *commandPath = findPath(args2[0]);
            execv(commandPath, args2);
            perror("execv");
            exit(1);
        } else {
            close(pipeDescriptor[0]);
            close(pipeDescriptor[1]);

            int status1, status2;
            waitpid(pid, &status1, 0);
            waitpid(pid2, &status2, 0);

            if (WIFEXITED(status2)) {
                return WEXITSTATUS(status2);
            }
        }  
    }
    return -1; 
}
```


## Testing Strategy

*Run mysh in the terminal by typing ./mysh (without any args to start) in the directory where ./mysh is located.

### Basic Command Testing

*First to test that shell properly handles basic commnands, we'd type in the following basic commands individually in the terminal.*

-ls
Output: Makefile  mysh  myshbeta.c  mysh.c  myshnotes.c  README.txt  testbatch.txt  testdirectory  testfile.txt

-echo Whats up?
Output: Whats up?

-date
Output: Sun Dec  3 09:12:53 PM EST 2023


*Also we'd test basic edge cases.*

-test whether you can run mysh without entering a command and not have it crash.
Output: should not crash program, restart input loop

-echo "THIS IS A VERY LONG TEST: Next we test whether the built in commands work properly, the following tests should ensure that the build in commands work,Next we test whether the built in commands work properly, the following tests should ensure that the build in commands work, Next we test whether the built in commands work properly, the following tests should ensure that the build in commands work, Next we test whether the built in commands work properly, the following tests should ensure that the build in commands work, Next we test whether the built in commands work properly, the following tests should ensure that the build in commands work"
Output: should display whole string

### Built in commands

*Next we test whether the built in commands work properly, the following tests should ensure that the build in commands work*

-cd testdirectory
Output: got to testdirectory

-ls 
Output: subdirtest.txt

-pwd
Output: /common/home/stw69/Documents/CS214 Systems Programming/MyShell/testdirectory

-cd ..
Output: return to previous directory

-ls
Output: Makefile  mysh  myshbeta.c  mysh.c  myshnotes.c  README.txt  testbatch.txt  testdirectory  testfile.txt

-pwd
Output:/common/home/stw69/Documents/CS214 Systems Programming/MyShell

-which ls
Output: /usr/bin/ls

-which cd
Output: cd not found

-exit
Output: succesful exit


### Test Wildcard Expansion

*Test wildcard expanison, test if its included in arg list as is if doesnt find a match, test for subdirectories, test extreme prefix*
*and suffix cases.

-ls *
Output:
Makefile  mysh  myshbeta.c  mysh.c  myshnotes.c  README.txt  testbatch.txt  testfile.txt

testdirectory:
subdirtest.txt

-ls *.txt
Output: README.txt  testbatch.txt  testfile.txt

-ls testdirectory/*.txt
Output: Testdirectory//subdirtest.txt

-ls *testfile.txt
Output: testfile.txt

-ls testfile.txt*
Output: testfile.txt

-ls tes*le.txt
Output: testfile.txt


### Test Redirection

*These tests will test if redirection is working with basic commands and with both input and output.

cat < testfile.txt
Output: display contents of testfile.txt in terminal

echo "Redirection Test" > redirectiontest.txt
Output: Creat file redirectiontest.txt with contents "Redirection Test"

sort < testfile.txt > redirectiontest.txt
Output: sort the the contents of testfile.txt and overwrite and place the results in redirectiontest.txt

### Test Pipes

*These commands will test if the pipe function is properly implemented. It will test pipes as well as pipes with redirection.*

-ls | sort
Output: sorted list of files in the directory

-cat testfile.txt | grep Stephen
Output: Display Stephen if found

ls -l | sort -k5 -n > sortfilenames.txt
Output:Sort files in Myshell by size and output them to sortfilesnames

### Test Conditionals

*Test statements with else or then*

-echo Whats up?
-then echo Test Succesful
-echo Whats up?
-then echo Test Failed
-cd testwillfail
-then echo Test Failed

### Test Porgram exit

*To test if the program exits properly type the following commands*

Remove Created files

-rm redirectiontest.txt
-rm sortfilenames.txt

-ps(only 1 mysh should be running else you'd have to exit every case)
-exit
-ps(no mysh should be running)

### Test Batch mode

*To test batch mode a file is included in the directory called testbatch.txt, contained in the file are*
*the various tests seen above that need to be manually typed when using interactive mode. Running this*
*file in batch mode should output the same results. To run in batch mode type ./mysh but included an *
*argument afterwards, so in this case ./mysh batchtest.txt*

