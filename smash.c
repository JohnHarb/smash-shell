/* 
pingpong
John Harb
10-13-2022
10-12-2022
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

void ctrlz(){
    struct sigaction olds;
    sigaction( SIGINT, &olds, NULL);
    write(STDOUT_FILENO, "\nexit has occured\n", 18);
    exit(0);  
}

int main(int argc, char* argv[]){
    char errormessage[30] = "an error has occurred\n";

    FILE* file;
    int fout;
    char* paths[30];
    paths[0] = strdup("/bin");
    paths[1] = NULL;
    int batch = 1;
    int rd = 0;
    if (argc > 2)
        printf("error: too many args\n");
    else if (argc == 2){
        file = fopen(argv[1], "r");
        if (file == NULL){
            write(STDERR_FILENO, errormessage, strlen(errormessage));
            exit(1);
        }
    }
    else {
        file = stdin;
        batch = 0;
    }
    
    char *line = NULL;
    size_t len = 0;

    while(!feof(file)){
        if (batch == 0)
            printf("smash> ");

        struct sigaction news;
        struct sigaction olds;
             
        news.sa_handler = ctrlz;
        news.sa_flags = 0;
        sigaction(SIGTSTP, &news, &olds);

        getline(&line, &len, file);
        
        if(batch!=1)
            line[strlen(line)-1] = '\0';

        char *semiColon;
        while((semiColon = strsep(&line,";")) != NULL){
            char *cmds[10][10];
            int i = 0;
            char *amp;
            while((amp = strsep(&semiColon,"&")) != NULL){
                int j = 0;
                char* space;
                while((space = strsep(&amp," ")) != NULL){
                    cmds[i][j] = strdup(space);
                    j++;
                }
                cmds[i][j] = NULL;
                i++;
            }
            cmds[i][0] = NULL;

            int c = 0;
            while(cmds[c][0]!= NULL){
                char* last;
                int g = 0;
                while(cmds[c][g]!= NULL){
                    last = strdup(cmds[c][g]);
                    g++;
                }

                int rds = 0;
                char *redir;
                char *redirs[2];
                redirs[1] = strdup("");
                fout = STDOUT_FILENO;
                while((redir = strsep(&last,">")) != NULL){
                    redirs[rds] = strdup(redir);
                    rds++;
                }
                if(strlen(redirs[1])>0){
                    fout = open(redirs[1], O_WRONLY | O_TRUNC | O_CREAT, 0644);
                    if (fout < 0){
                        write(STDERR_FILENO, errormessage, strlen(errormessage));
                        exit(1);
                    }
                    rd = 1;
                }
                if (rd == 1){
                    cmds[c][g-1] = strdup(redirs[0]);
                }

                if(strcmp(cmds[c][0], "exit") == 0){
                    write(fout, "exit has occured\n", 17);
                    if(batch == 1)
                        fclose(file);
                    if(rd == 1)
                        close(fout);
                    exit(0);    
                }
                else if(strcmp(cmds[c][0], "cd") == 0){
                    char cwd[256];
                    getcwd(cwd, sizeof(cwd));
                    write(fout, "before cd: ", 11);
                    write(fout, cwd, strlen(cwd));
                    if(chdir(cmds[c][1]) == 0){
                        char cwda[256];
                        getcwd(cwda, sizeof(cwd));
                        write(fout, "\nafter cd: ", 11);
                        write(fout, cwda, strlen(cwda));
                        write(fout, "\n", 1);
                    }
                    else{
                        if(cmds[c][1] == NULL)
                            write(STDERR_FILENO, errormessage, strlen(errormessage));
                        else
                            write(STDERR_FILENO, errormessage, strlen(errormessage));
                    }
                }
                else if(strcmp(cmds[c][0], "path") == 0){
                    if(cmds[c][1] == NULL)
                        write(STDERR_FILENO, errormessage, strlen(errormessage));
                    else{
                        if(strcmp(cmds[c][1], "add") == 0 && cmds[c][2] != NULL){
                        int s = 0;
                        while(paths[s] != NULL){
                            s++;
                        }
                        paths[s] = strdup(cmds[c][2]);
                    }
                    else if(strcmp(cmds[c][1], "remove") == 0 && cmds[c][2] != NULL){
                        int x = 0;
                        int foun = 0;
                        while(paths[x] != NULL){
                            if(strcmp(paths[x], cmds[c][2]) == 0){
                                foun = 1;
                                paths[x] = NULL;
                                break;
                            }
                            x++;
                        }
                        if (foun == 0){
                            write(STDERR_FILENO, errormessage, strlen(errormessage));
                        }
                    }
                    else if(strcmp(cmds[c][1], "clear") == 0){
                        int z = 0;
                        while(paths[z] != NULL){
                            paths[z] = NULL;
                            z++;
                        }
                    }
                    else{
                        write(STDERR_FILENO, errormessage, strlen(errormessage));
                    }
                    int y = 0;
                    while(paths[y] != NULL){
                        printf("path %d: %s\n", y, paths[y]);
                        y++;
                    }
                    }
                }
                else{
                    char* fpath;
                    int p = 0;
                    int found = 0;
                    while (paths[p] != NULL){
                        char* tempath;
                        tempath = strdup(paths[p]);
                        strcat(tempath, "/");
                        strcat(tempath, cmds[c][0]);
                        if (access(tempath, X_OK) == 0){
                            fpath = strdup(tempath);
                            found = 1;
                            break;
                        }
                        p++;
                    }
                    if(found == 1){
                        pid_t pid = fork();
                        if(pid == 0){
                            dup2(fout, 1);
                            int status = execv(fpath, cmds[c]);
                            if (status == -1)
                                write(STDERR_FILENO, errormessage, strlen(errormessage));
                        }
                        else if(pid > 0){
                            int status = 0;
                            wait(&status);
                        }
                        else{
                            write(STDERR_FILENO, errormessage, strlen(errormessage));
                        }
                    }
                    else{
                        write(STDERR_FILENO, errormessage, strlen(errormessage));
                    }
                }
                c++;
            }
        }
    }
    return 0;
}
