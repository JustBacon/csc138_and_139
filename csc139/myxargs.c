#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 100
#define MAX_COMMAND_LENGTH 256

typedef struct {
    char *args[MAX_ARGS];
    int count;
} ArgsList;

ArgsList commandArgs;
int num = -1; // used for -n # command
char *replaceIcommand = NULL; // -I {} command
bool rCommand = false; // -r command
bool tCommand = false; // -t command

void errorUsage() {
    printf("Usage: myxargs [-n num] [-I replace] [-t] [-r] command\n");
    exit(EXIT_FAILURE);
}

// sanitize special characters
char *sanitizeInput(char *input) {
    static char sanitized[MAX_INPUT_SIZE];
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (strchr("; &|><*?()$", input[i]) == NULL) {
            sanitized[j++] = input[i];
        }
    }
    sanitized[j] = '\0';
    return sanitized;
}

// get input from stdin
void readInput(ArgsList *inputs) {
    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line), stdin)) {
        char *token = strtok(line, " \t\n");
        while (token != NULL) {
            char *sanitized = sanitizeInput(token);
            inputs->args[inputs->count++] = strdup(sanitized);
            token = strtok(NULL, " \t\n");
        }
    }
}

// run command with the arguments
void runCommand(ArgsList *finalCommand) {
    if (tCommand) {
        for (int i = 0; i < finalCommand->count; i++) {
            printf("%s ", finalCommand->args[i]);
        }
        printf("\n");
    }

    char command[MAX_COMMAND_LENGTH];
    strcpy(command, finalCommand->args[0]);
    for (int i = 1; i < finalCommand->count; i++) {
        strcat(command, " ");
        strcat(command, finalCommand->args[i]);
    }

    system(command);
}

// used for replacing the {} in I command
void str_replace(char *str, const char *old, const char *new) {
    char buffer[MAX_COMMAND_LENGTH];
    char *p;

    if (!(p = strstr(str, old))) {
        return;
    }

    strncpy(buffer, str, p - str);
    buffer[p - str] = '\0';
    sprintf(buffer + (p - str), "%s%s", new, p + strlen(old));
    strcpy(str, buffer);
}

// -I command
void withICommand(ArgsList *inputs) {
    if (num == -1){
        num = 1;
    }
    for (int i = 0; i < inputs->count; i += num) {
        ArgsList batch;
        batch.count = 0;

        for (int j = i; j < i + num && j < inputs->count; j++) {
            batch.args[batch.count++] = inputs->args[j];
        }

        ArgsList finalCommand;
        finalCommand.count = 0;

        for (int k = 0; k < commandArgs.count; k++) {
            char *arg = commandArgs.args[k];
            if (strstr(arg, "{}")) {
                for (int l = 0; l < batch.count; l++) {
                    finalCommand.args[finalCommand.count] = malloc(MAX_COMMAND_LENGTH);
                    strcpy(finalCommand.args[finalCommand.count], arg);
                    str_replace(finalCommand.args[finalCommand.count], "{}", batch.args[l]);
                    finalCommand.count++;
                }
            } else {
                finalCommand.args[finalCommand.count++] = strdup(arg);
            }
        }

        runCommand(&finalCommand);
    }
}

// if -I command is not passed
void withoutICommand(ArgsList *inputs) {
    if (num == -1){
        num = 1;
    }
    for (int i = 0; i < inputs->count; i += num) {
        ArgsList batch;
        batch.count = 0;

        for (int j = i; j < i + num && j < inputs->count; j++) {
            batch.args[batch.count++] = inputs->args[j];
        }

        ArgsList finalCommand;
        finalCommand.count = 0;
        for (int k = 0; k < commandArgs.count; k++) {
            finalCommand.args[finalCommand.count++] = strdup(commandArgs.args[k]);
        }
        for (int l = 0; l < batch.count; l++) {
            finalCommand.args[finalCommand.count++] = strdup(batch.args[l]);
        }

        runCommand(&finalCommand);
    }
}

// Driver
int main(int argc, char *argv[]) {
    // check number of arguments and how to use program
    if (argc < 2) {
        errorUsage();
    }

    commandArgs.count = 0;

    // go through arg list
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                num = atoi(argv[++i]);
            } else {
                errorUsage();
            }
        } else if (strcmp(argv[i], "-I") == 0) {
            if (i + 1 < argc) {
                replaceIcommand = argv[++i];
            } else {
                errorUsage();
            }
        } else if (strcmp(argv[i], "-r") == 0) {
            rCommand = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            tCommand = true;
        } else {
            commandArgs.args[commandArgs.count++] = argv[i];
        }
    }

    ArgsList inputs;
    inputs.count = 0;
    readInput(&inputs);

    if (inputs.count == 0 && rCommand) {
        return 0;
    }

    if (num != -1 && replaceIcommand != NULL) {
        printf("-n and -I are mutually exclusive\n");
        return 1;
    }

    if (replaceIcommand != NULL) {
        withICommand(&inputs);
    } else {
        withoutICommand(&inputs);
    }

    return 0;
}
