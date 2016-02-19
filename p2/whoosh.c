/*
 * =====================================================================================
 *
 *       Filename:  shell.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/07/2016 09:25:22
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YING FANG (), fang42@wisc.edu
 *   Organization:  
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>


char ERROR_MSG[30] = "An error has occurred\n";

//for utility use
void trim(char *s) {
    int i;

    while (isspace (*s)) s++;   // skip left side white spaces
    for (i = strlen (s) - 1; (isspace (s[i])); i--) ;   // skip right side white spaces
    s[i + 1] = '\0';
}

void flush_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

//built-in commands
int my_exit() {
    exit(0);
}

void my_pwd() {
    char cwd[512];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n",cwd);
}

int my_cd(const char *path) {
    int i;
    if (path == NULL) {
        i = chdir(getenv("HOME"));
    } else {
        i = chdir(path);
    }
    return i;
}

void resetPaths(char** paths) {
    int i =0;
    while (paths[i] != NULL) {
        paths[i] = NULL;
        i++;
    }
}

//main
int main (int argc, char* argv[]){

    if (argc != 1) {
        fprintf(stderr, "%s", ERROR_MSG);
        exit(1);
    }
    char command[128];
    char* cmd;
    char* myargv[128];
    int redr;
    char* file;

    //set default path
    char *paths[128];
    paths[0] = "/bin";

    while(1) {
        printf("whoosh> ");
        fflush(stdout);
        redr = 0;

        // read in command line
        if (fgets(command, 128, stdin) == NULL) continue;
        if (*command != '\n') {
            if (command[strlen(command)-1] != '\n') {
                fprintf(stderr, "%s", ERROR_MSG);
                flush_input();
                continue;
            }
        } else {
            continue;
        }

        trim(command);
        cmd = strtok(command, " ");

        //process built-in commands
        if (!strcmp(cmd, "exit")) {
            my_exit();
        }
        else if (!strcmp(cmd, "pwd")) {
            my_pwd();
        }
        else if (!strcmp(cmd, "cd")) {
            char *path = strtok(NULL, " ");
            int i = my_cd(path);
            if (i == -1) {
                fprintf(stderr, "%s", ERROR_MSG);
            }}
        else if (!strcmp(cmd, "path")){
            resetPaths(paths);
            int i = 0;
            char* p = strtok(NULL, " ");
            while (p != NULL) {
                paths[i] = p;
                i++;
                p = strtok(NULL, " "); 
            }
        }

        //non built-in commands
        else {
        //parse myargv
            int i = 1;
            char* arg;
            do {
                arg = strtok(NULL, " ");
                if (arg != NULL && *arg == '>') {
                    redr = 1;
                    break;
                }
                myargv[i] = arg;
                i++;
            }
            while (arg != NULL);

            //set up redirection file
            if (redr == 1) {
                file = strtok(NULL, " ");
                if (file == NULL || strtok(NULL, " ") != NULL) {
                    fprintf(stderr, "%s", ERROR_MSG);
                    continue;
                }
                myargv[i] = NULL;
            }

            int child = fork();
            if (child == 0) {   //child

                myargv[0] = malloc(512*sizeof(char));
                int i = 0;
                struct stat sb;
                while (1) {
                    char* p = paths[i];
                    if (p == NULL) {
//                        fprintf(stderr, "%s", ERROR_MSG);
                        fprintf(stdout, "%s", ERROR_MSG);
                        exit(1);
                    }
                    sprintf(myargv[0], "%s/%s", p, cmd);

                    //if exists
                    if (stat(myargv[0], &sb) == 0) {
                        //if redirection, change STDOUT & STDERR
                        if (redr == 1) {
                            char outfile[512];
                            sprintf(outfile, "%s.out",file);
                            close(STDOUT_FILENO);
                            open(outfile,
                                    O_CREAT | O_TRUNC | O_WRONLY,
                                    S_IRUSR | S_IWUSR);
                            char errfile[512];
                            sprintf(errfile, "%s.err", file);
                            close(STDERR_FILENO);
                            open(errfile,
                                    O_CREAT | O_TRUNC | O_WRONLY,
                                    S_IRUSR | S_IWUSR);
                        }

                        execv(myargv[0], myargv);
                        fprintf(stderr, "%s", ERROR_MSG);
                        exit(1);
                        break;
                    }
                    myargv[0][0] = '\0';
                    i++;
                }
            } else if (child > 0) { //parent
                wait(NULL);
            } else {
                fprintf(stderr, "%s", ERROR_MSG);
            }
        }
    }

    return 0;

}
