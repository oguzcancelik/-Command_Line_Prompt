#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80

void setup(char inputBuffer[], char *args[], int *background) {
    int i, start = -1, ct = 0;
    int length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    if (length == 0) {
        exit(0);
    }

    if ((length < 0) && (errno != EINTR)) {
        perror("error reading the command");
        exit(-1);
    }

    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]) {
            case ' ':
            case '\t':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                start = -1;
                break;
            case '\n':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL;
                break;
            default:
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&') {
                    *background = 1;
                    inputBuffer[i - 1] = '\0';
                }
        }
    }
    args[ct] = NULL;
}

int main(void) {
    pid_t childpid;
    int background, i, a, bmcounter = 0;
    char *inputBuffer[MAX_LINE];
    char *args[MAX_LINE / 2 + 1];
    char *found;
    char *pathArray[128];
    char *bookmark[32];
    const char *env = getenv("PATH");
    FILE *file;

    pathArray[0] = "/bin";
    for (a = 1; (found = strsep(&env, ":")) != NULL; a++) {
        pathArray[a] = found;
    }

    while (1) {
        background = 0;
        printf("\nmyshell: \n");
        setup(inputBuffer, args, &background);
        char *arguments[41];
        char commandPath[41];
        int commandChecker = 0, redirectChecker = 0, appendChecker = 0, stderrChecker = 0, fileNumber = 0;

        if (!strcmp(args[0], "bookmark")) {
            if (!strcmp(args[1], "-l")) {
                for (i = 0; i < bmcounter; i++) {
                    printf("%d %s\n", i, bookmark[i]);
                }
                continue;
            } else if (!strcmp(args[1], "-d")) {
                int n = atoi(args[2]);
                for (i = n; i < bmcounter - 1; i++) {
                    bookmark[i] = bookmark[i + 1];
                }
                bookmark[i] = NULL;
                bmcounter--;
                continue;
            } else if (!strcmp(args[1], "-i")) {
                int n = atoi(args[2]);
                char *temp = bookmark[n];
                char *temp2 = "";
                for (i = 0; (found = strsep(&temp, "\"")) != NULL; i++) {
                    asprintf(&temp2, "%s%s", temp2, found);
                }
                for (i = 0; (found = strsep(&temp2, " ")) != NULL; i++) {
                    args[i] = found;
                }
                args[i] = NULL;
            } else {
                asprintf(&bookmark[bmcounter], "%s", args[1]);
                for (i = 2; args[i] != NULL; i++) {
                    asprintf(&bookmark[bmcounter], "%s %s", bookmark[bmcounter], args[i]);
                }
                printf("\n%d %s\n", bmcounter, bookmark[bmcounter]);
                bmcounter++;
                continue;
            }
        } else if (!strcmp(args[0], "codesearch")) {
            char *temp = "";
            if (!strcmp(args[1], "-r")) {
                args[0] = "grep";
                args[1] = "-nr";
                for (i = 3; args[i] != NULL; i++) {
                    asprintf(&args[2], "%s %s", args[2], args[i]);
                }
                for (i = 0; (found = strsep(&args[2], "\"")) != NULL; i++) {
                    asprintf(&temp, "%s%s", temp, found);
                }
                args[2] = temp;
                args[3] = "--include=*.[c,C,h,H]";
                args[4] = NULL;
            } else {
                args[0] = "grep";
                for (i = 2; args[i] != NULL; i++) {
                    asprintf(&args[1], "%s %s", args[1], args[i]);
                }
                for (i = 0; (found = strsep(&args[1], "\"")) != NULL; i++) {
                    asprintf(&temp, "%s%s", temp, found);
                }
                args[1] = temp;
                args[2] = "-nr";
                args[3] = "--include=*.[c,C,h,H]";
                args[4] = "--exclude-dir=*";
                args[5] = NULL;
            }
        } else if (!strcmp(args[0], "set")) {
            if (args[2] != NULL && !strcmp(args[2], "=")) {
                if (!setenv(args[1], args[3], 1)) {
                } else {
                    fprintf(stderr, "\nsetenv() failed.\n");
                }
            } else if (args[2] == NULL) {
                for (i = 2; (found = strsep(&args[1], "=")) != NULL; i++) {
                    asprintf(&args[i], "%s", found);
                }
                if (i == 4) {
                    if (!setenv(args[2], args[3], 1)) {
                    } else {
                        fprintf(stderr, "\nsetenv() failed.\n");
                    }
                } else {
                    fprintf(stderr, "\nWrong input type: \nUsage: set varname = somevalue OR set varname=somevalue\n");
                }
            } else {
                fprintf(stderr, "\nWrong input! \nUsage: set varname = somevalue OR set varname=somevalue\n");
            }
            continue;
        } else if (!strcmp(args[0], "print")) {
            if (args[1] == NULL) {
                args[0] = "printenv";
            } else {
                char *temp = "";
                for (i = 0; (found = strsep(&args[1], "<,>")) != NULL; i++) {
                    asprintf(&temp, "%s%s", temp, found);
                }
                args[1] = temp;
                env = getenv(args[1]);
                if (env != NULL) {
                    printf("%s\n", env);
                } else {
                    printf("\n%s does not exist.", args[1]);
                }
                continue;
            }
        } else if (!strcmp(args[0], "exit")) {
            printf("\nThere may be background processes still running. \nTerminate those processes to exit.\n");
            while (wait(NULL) > 0);
            printf("\nProgram terminated.\n\n");
            break;
        } else {
            for (i = 0; args[i] != NULL; i++) {
                if (!strcmp(args[i], ">") || !strcmp(args[i], ">>") || !strcmp(args[i], "2>")) {
                    if (args[i + 1] == NULL) {
                        fprintf(stderr, "\nWrong input!");
                        continue;
                    }
                    args[i + 2] = NULL;
                    redirectChecker = 1;
                    fileNumber = i + 1;
                    if (!strcmp(args[i], ">>")) {
                        appendChecker = 1;
                    } else if (!strcmp(args[i], "2>")) {
                        stderrChecker = 1;
                    }
                }
            }
        }
        for (i = 0; i < a; i++) {
            strcpy(commandPath, pathArray[i]);
            strcat(commandPath, "/");
            strcat(commandPath, args[0]);
            file = fopen(commandPath, "r");
            if (file) {
                fclose(file);
                commandChecker = 1;
                break;
            }
        }
        if (!commandChecker) {
            fprintf(stderr, "\nError: Command not found.\n");
            continue;
        }
        arguments[0] = commandPath;
        for (i = 1; args[i] != NULL && strcmp(args[i], "&"); i++) {
            arguments[i] = args[i];
        }
        for (i; i < 32; i++) {
            arguments[i] = NULL;
        }
        childpid = fork();
        if (childpid == -1) {
            perror("Failed to fork");
            return 1;
        } else if (childpid == 0) {
            if (redirectChecker) {
                int redirectFile;
                if (appendChecker) {
                    redirectFile = open(args[fileNumber], O_CREAT | O_RDWR | O_APPEND,
                                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                } else {
                    redirectFile = open(args[fileNumber], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                }

                dup2(redirectFile, 1);

                if (stderrChecker) {
                    dup2(redirectFile, 2);
                }
                close(redirectFile);
            }
            execv(commandPath, arguments);
        }
        if (!background && childpid > 0) {
            while (wait(NULL) > 0);
        }
    }
}
