#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#define MAX_LINE 80 /* The maximum length command */
#define MAX_PATH 20
#define BUFFERSIZE 1000
#define HISTORY_SIZE 20
int  HISTORY_ID=0;
int size = 0;
int paths_size = 0;
char *history[HISTORY_SIZE];
char *paths[MAX_PATH];
char *trim(char *s){
    while(isspace(*s)) s++;
    return s;
}
char *concatenate(char* str1, char* str2){
    char *str3 = (char *) malloc(strlen(str1)+ strlen(str2) +1);
    strcpy(str3, str1);
    strcat(str3, str2);
    return str3;
}
int get_args(char *line, char *args[]){
        line=trim(line);
    char * token;
    size = 0;
    token = strtok (line," \t");
    if(strcmp(token,"\n")==0)
        return 0;
        while (token != NULL){
            if(token[strlen(token)-1]=='\n'){
                token[strlen(token)-1] ='\0';
            }
            if(strcmp(token,"\t")!=0 && strcmp(token," ")!=0
                && strcmp(token,"\n")!=0 && strcmp(token,"")!=0
                &&token!=NULL){
                args[size++] = token;
            }
            token = strtok (NULL, " \t");
        }
        args[size] = NULL;
        return 1;
}

int CHECK_COMMAND(char *args[]){        //to do export
   char*line=args[0];
   if(strcmp(line,"cd")==0){
   	if(size<2){
   		args[1]="/home";
   	}
    int ret=chdir(args[1]);
    if(ret!=0){
        perror("Error\n");
    }
    return 0;
   }
   if(strcmp(line,"!!")==0){
       if(history[0]==NULL){
                printf("No commands in history!!!\n");
                return 0;
        }
          printf("previous\n");
          int id=(HISTORY_ID-1+HISTORY_SIZE)%HISTORY_SIZE;
          get_args(history[id],args);
          return 1;
    }
    if(strcmp(line,"!")==0){
        int id=atoi(args[1]);
         id=(HISTORY_ID-id+HISTORY_SIZE)%HISTORY_SIZE;
            if(history[id]==NULL){
                printf("No such command in history!!!\n");
                return 0;
             }
             get_args(history[id],args);
            return 1;
    }
    if(strcmp(line,"history")==0){      
        print_history();
        return 0;
    }
    if(strcmp(line,"exit")==0 ){
        finish();
    }
    return 1;
}

int check_ampersand(char *args[]){
    char *last_word = args[size-1];
    int len = strlen(last_word);
    if(len==1){
        if(last_word[0]=='&'){
            args[size-1] = NULL;
            size--;
            return 1;
        }
    }
    return 0;
}

void FIND_PATH_VARIABLES(){
    char *path_string = getenv("PATH");
    char *token ;
    token = strtok(path_string ,":");
    while(token != NULL){
        paths[paths_size++] = token;
        token = strtok(NULL ,":");
    }
}

char *check_vaild_path(char *args_zero){
    int i;
    char *path;
    for(i = 0 ; i < paths_size ; i++){
        path = NULL;
        path = concatenate(paths[i], "/");
        path = concatenate(path, args_zero);
        int flag = access(path , F_OK);
        // printf("%s\n", path);
        if(flag != -1){
        //    printf("found in %s\n", path);
            return path;
        }
    }
    return args_zero;
}

int forkProcess(char *args[]){
    pid_t pid;
    int status;
    int wait_flag = check_ampersand(args);
    pid = fork();
    if(pid < 0){
        printf(stderr,"Fork Failed!!");
    }else if (pid == 0){ /* child */
        if(execv(check_vaild_path(args[0]),args)==-1){
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if(wait_flag==0){ /* parent */
        waitpid(pid, &status, 0);
        return 1;
    }else if(wait_flag==0){
        return 1;
    }
}

void read_execute_file(char *file_name){
    FILE* file = fopen(file_name, "r"); /* should check the result */
    if (file == NULL){

        printf("Error opening file!\n");
        exit(1);
    }
    char *line;
    char *args[MAX_LINE/2 + 1]; 
    while (fgets(line, sizeof(line), file) != NULL) {
        if(line == EOF || line == NULL){
            fclose(file);
            finish();
        }
        printf("%s\n", line);
        get_args(line,&args);
        int flag = CHECK_COMMAND(&args);
        if(flag==1){
            if(forkProcess(args)==-1){
                printf("%s : command not found!!\n",line);
                continue;
            }
        }
    }
    fclose(file);
    finish();
}

void add_to_history(char *command){
    history[HISTORY_ID]=command;
    HISTORY_ID=(HISTORY_ID+1)%HISTORY_SIZE;
}

void save_history(){
    FILE *file = fopen("history.txt", "w");
    if (file == NULL){
        printf("Error opening file!\n");
        exit(1);
	}
    int cnt=(HISTORY_ID-1+HISTORY_SIZE)%HISTORY_SIZE,ind=0;
    while(history[cnt]!=NULL){
        char *line = history[cnt];
        fprintf(file, "%s\n", line);
        cnt=(cnt-1+HISTORY_SIZE)%HISTORY_SIZE;
    }
    fclose(file);
}

void LOAD_HISTORY(){
    FILE* file = fopen("history.txt", "r");
    if (file == NULL){
       return;
    }
    char line[MAX_LINE];
    int i = 0;
    while (fgets(line, sizeof(line), file) != NULL){
        if(strcmp(line,"")!=0 && strcmp(line,"\n")!=0 && line != NULL){
        //	line[strlen(line)-1]='\0';
            add_to_history(concatenate(line,""));
        }
    }
    fclose(file);
}

void finish(){
    save_history();
    exit(0);
}

void print_history(){
    int cnt=(HISTORY_ID-1+HISTORY_SIZE)%HISTORY_SIZE,id=0;
    while(history[cnt]!=NULL){
        printf("%d %s\n",id++, history[cnt]);
        cnt=(cnt-1+HISTORY_SIZE)%HISTORY_SIZE;
    }
}

int main(int argc, char *argv[]){
    LOAD_HISTORY();
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int running = 1; /* flag to determine when to exit program */
    FIND_PATH_VARIABLES();
    while (running){
        if(argc > 1){ //read from file
            read_execute_file(argv[1]);
        }else{	
            printf("Shell> ");
            char *line;
            fgets(line, BUFFERSIZE , stdin);
            if(strcmp(line,"\n")==0 || strcmp(line,"\r\n")==0)
			{
			  continue;
			}	
            if(strlen(line)>80){
                printf("A very long command line (over 80 characters)\n");
                continue;
            }
            get_args(line,&args);
            if(strcmp(args[0],"history")&&strcmp(args[0],"!!")&&strcmp(args[0],"!")){
          	  add_to_history(concatenate(line,""));
            }
            int flag = CHECK_COMMAND(&args);
            if(flag==1){
                if(forkProcess(&args)==-1){
                    printf("%s : command not found!!\n",line);
                    continue;
                }
            }
        }
    }
    return 0;
}
