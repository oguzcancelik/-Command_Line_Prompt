#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80

/* Setup function that provided by teacher assistant.
   It seperates input to array elements. */
void setup(char inputBuffer[], char *args[], int *background)
{
    int length, i, start, ct;
    ct = 0;

    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    start = -1;
    if (length == 0)
    {
        exit(0);
    }

    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1);
    }

    for (i = 0; i < length; i++)
    {
        switch (inputBuffer[i])
        {
        case ' ':
        case '\t':
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            start = -1;
            break;
        case '\n':
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL;
            break;
        default:
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }
    }
    args[ct] = NULL;
}

//End of setup function

// Main function
int main(void)
{
    //initializing variables

    pid_t childpid;
    int background, i, a, bmcounter = 0;
    char *inputBuffer[MAX_LINE];
    char *args[MAX_LINE / 2 + 1];
    char *found;
    char *pathArray[128];
    char *bookmark[32];
    const char *env = getenv("PATH"); //reading PATH variable
    FILE *file;

    /*  Seperating PATH variable to pathArray array.
        It stores the command paths.*/
    pathArray[0] = "/bin";
    for (a = 1; (found = strsep(&env, ":")) != NULL; a++)
    {
        pathArray[a] = found;
    }

    while (1)
    {
        background = 0;
        //  Get input from the user.
        printf("\nmyshell: \n");
        setup(inputBuffer, args, &background);
        char *arguments[41];
        char commandPath[41];
        int commandChecker = 0, redirectChecker = 0, appendChecker = 0, stderrChecker = 0, fileNumber = 0;

        // If input starts with "bookmark", do bookmark tasks.
        if (!strcmp(args[0], "bookmark"))
        {
            if (!strcmp(args[1], "-l")) // List bookmarks
            {
                for (i = 0; i < bmcounter; i++)
                {
                    printf("%d %s\n", i, bookmark[i]);
                }
                continue;
            }
            else if (!strcmp(args[1], "-d")) // Detele elements from bookmarks
            {
                int n = atoi(args[2]);
                for (i = n; i < bmcounter - 1; i++)
                {
                    bookmark[i] = bookmark[i + 1];
                }
                bookmark[i] = NULL;
                bmcounter--;
                continue;
            }
            else if (!strcmp(args[1], "-i")) //Execute bookmarks
            {
                int n = atoi(args[2]);
                char *temp = bookmark[n];
                char *temp2 = "";

                for (i = 0; (found = strsep(&temp, "\"")) != NULL; i++)
                {
                    asprintf(&temp2, "%s%s", temp2, found);
                }
                for (i = 0; (found = strsep(&temp2, " ")) != NULL; i++)
                {
                    args[i] = found;
                }
                args[i] = NULL;
            }
            else // Add new bookmarks
            {
                asprintf(&bookmark[bmcounter], "%s", args[1]);
                for (i = 2; args[i] != NULL; i++)
                {
                    asprintf(&bookmark[bmcounter], "%s %s", bookmark[bmcounter], args[i]);
                }
                printf("\n%d %s\n", bmcounter, bookmark[bmcounter]);
                bmcounter++;
                continue;
            }
        }
        // If input starts with "codesearch", do codesearch tasks.
        else if (!strcmp(args[0], "codesearch"))
        {
            char *temp = "";
            if (!strcmp(args[1], "-r")) // Search recursively
            {
                args[0] = "grep"; //Using "grep" command for searching
                args[1] = "-nr";  // grep options
                for (i = 3; args[i] != NULL; i++)
                {
                    asprintf(&args[2], "%s %s", args[2], args[i]);
                }
                for (i = 0; (found = strsep(&args[2], "\"")) != NULL; i++)
                {
                    asprintf(&temp, "%s%s", temp, found);
                }
                args[2] = temp;
                args[3] = "--include=*.[c,C,h,H]"; // Including  c,C,h,H files
                args[4] = NULL;
            }
            else
            {
                args[0] = "grep"; //Using "grep" command for searching
                for (i = 2; args[i] != NULL; i++)
                {
                    asprintf(&args[1], "%s %s", args[1], args[i]);
                }
                for (i = 0; (found = strsep(&args[1], "\"")) != NULL; i++)
                {
                    asprintf(&temp, "%s%s", temp, found);
                }
                args[1] = temp;
                args[2] = "-nr";                   // grep options
                args[3] = "--include=*.[c,C,h,H]"; // Including  c,C,h,H files
                args[4] = "--exclude-dir=*";       // Excluding subdirectories
                args[5] = NULL;
            }
        }
        // If input starts with "set", do set tasks.
        else if (!strcmp(args[0], "set"))
        {
            if (args[2] != NULL && !strcmp(args[2], "="))
            {
                if (!setenv(args[1], args[3], 1)) // Set environment variable
                {
                }
                else
                {
                    fprintf(stderr, "\nsetenv() failed.\n");
                }
            }
            else if (args[2] == NULL)
            {
                for (i = 2; (found = strsep(&args[1], "=")) != NULL; i++)
                {
                    asprintf(&args[i], "%s", found);
                }

                if (i == 4)
                {
                    if (!setenv(args[2], args[3], 1))
                    {
                    }
                    else
                    {
                        fprintf(stderr, "\nsetenv() failed.\n");
                    }
                }
                else
                {
                    fprintf(stderr, "\nWrong input type: \nUsage: set varname = somevalue OR set varname=somevalue\n");
                }
            }
            else
            {
                fprintf(stderr, "\nWrong input! \nUsage: set varname = somevalue OR set varname=somevalue\n");
            }
            continue;
        }
        // If input starts with "print", do print tasks.
        else if (!strcmp(args[0], "print"))
        {
            if (args[1] == NULL)
            {
                args[0] = "printenv"; // Print all environment variables
            }
            else
            {
                char *temp = "";
                for (i = 0; (found = strsep(&args[1], "<,>")) != NULL; i++)
                {
                    asprintf(&temp, "%s%s", temp, found);
                }
                args[1] = temp;
                env = getenv(args[1]);
                if (env != NULL)
                {
                    printf("%s\n", env); // Print environment variable
                }
                else
                {
                    printf("\n%s does not exist.", args[1]);
                }
                continue;
            }
        }
        // If input starts with "print", do print tasks.
        else if (!strcmp(args[0], "exit"))
        {
            printf("\nThere may be background processes still running. \nTerminate those processes to exit.\n");

            while (wait(NULL) > 0) // Wait for background processes
                ;

            printf("\nProgram terminated.\n\n");
            break;
        }
        else // Detect if there are any redirection taks
        {
            for (i = 0; args[i] != NULL; i++)
            {
                if (!strcmp(args[i], ">") || !strcmp(args[i], ">>") || !strcmp(args[i], "2>"))
                {
                    if (args[i + 1] == NULL)
                    {
                        fprintf(stderr, "\nWrong input!");
                        continue;
                    }
                    args[i + 2] = NULL;
                    redirectChecker = 1;
                    fileNumber = i + 1;
                    if (!strcmp(args[i], ">>"))
                    {
                        appendChecker = 1;
                    }
                    else if (!strcmp(args[i], "2>"))
                    {
                        stderrChecker = 1;
                    }
                }
            }
        }
        // Detecting the path of the given command
        for (i = 0; i < a; i++)
        {
            strcpy(commandPath, pathArray[i]);
            strcat(commandPath, "/");
            strcat(commandPath, args[0]);
            file = fopen(commandPath, "r"); // If the file exist, use this path to exec
            if (file)
            {
                fclose(file);
                commandChecker = 1;
                break;
            }
        }

        // If given command does not exist, print error
        if (!commandChecker)
        {
            fprintf(stderr, "\nError: Command not found.\n");
            continue;
        }

        arguments[0] = commandPath;
        for (i = 1; args[i] != NULL && strcmp(args[i], "&"); i++)
        {
            arguments[i] = args[i];
        }
        for (i; i < 32; i++)
        {
            arguments[i] = NULL;
        }

        childpid = fork(); // fork
        if (childpid == -1)
        {
            perror("Failed to fork");
            return 1;
        }
        else if (childpid == 0)
        {
            if (redirectChecker) // Redirection option
            {
                int redirectFile;
                if (appendChecker) // Append option
                {
                    redirectFile = open(args[fileNumber], O_CREAT | O_RDWR | O_APPEND,
                                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                }
                else
                {
                    redirectFile = open(args[fileNumber], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                }

                dup2(redirectFile, 1);

                if (stderrChecker) // Stderr option
                {
                    dup2(redirectFile, 2);
                }
                close(redirectFile);
            }
            execv(commandPath, arguments); // Executing given command
        }

        if (!background && childpid > 0) // It the process is foreground, wait for it
        {
            while (wait(NULL) > 0)
                ;
        }
    }
}

//End of main function