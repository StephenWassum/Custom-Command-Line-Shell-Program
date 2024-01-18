/*
    Author: Stephen Wassum
    Date: 12/2/2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h> 

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

int lastExecution = 0;


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



int myCd(char *path) {
    if (chdir(path) != 0) {
        perror("cd error");
        return -1; 
    }
    return 0; 
}



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



void myExit() {
    exit(0);
}



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



int main(int argc, char *argv[]) {
    if (argc == 2) {
        batchMode(argv[1]);
    } else {
        interactiveMode();
    }
    return 0;
}
