#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#define BUFFER_SIZE 1<<16
#define ARR_SIZE 1<<16

void parse_args(char *buffer, char** args, 
                size_t args_size, size_t *nargs)
{
    char *buf_args[args_size]; /* You need C99 */
    char **cp;
    char *wbuf;
    size_t i, j;
    
    wbuf=buffer;
    buf_args[0]=buffer; 
    args[0] =buffer;
    
    for(cp=buf_args; (*cp=strsep(&wbuf, " \n\t")) != NULL ;){
        if ((*cp != '\0') && (++cp >= &buf_args[args_size]))
            break;
    }
    
    for (j=i=0; buf_args[i]!=NULL; i++){
        if(strlen(buf_args[i])>0)
            args[j++]=buf_args[i];
    }
    
    *nargs=j;
    args[j]=NULL;
}

void pidofexe(){
	DIR * dir;
	DIR * dir2;
	FILE *pidStat;
    struct dirent * ptr;
    dir =opendir("/proc");
    char cmds[32768][100]= {"\0"};
    int i;

    while((ptr = readdir(dir))!=NULL)
    {
    	char pathname[100];
        sprintf(pathname,"/proc/%s", ptr->d_name);
        if((dir2 = opendir(pathname))!=NULL) {
            //printf("%s: file\n", ptr->d_name);
            int pid= strtol(ptr->d_name, NULL, 10);
            if(pid != 0){ //a pid folder
            	sprintf(pathname, "%s%s",pathname, "/stat");
            	pidStat= fopen(pathname, "rb");
            	char num[10];
            	char proc[100];

            	fscanf(pidStat, "%s", num);
            	fscanf(pidStat, "%s", proc);
            	strcpy(cmds[atoi(num)], proc);
            }
        }
    }

    printf("pid 	name \n");
    for (i = 0; i < 32768; ++i)
    {
    	if(strcmp(cmds[i], "\0")!=0)
    		printf("%d 	  %s\n",i, cmds[i]);
    }
    //free(cmds);
    closedir(dir);
}

void cpuinfo(){
	FILE *cmdline = fopen("/proc/cpuinfo", "rb");
	char *arg = 0;
	char *str= ":";
	char *found;
	size_t size = 0;
	double mhz[20]= {0};
	int core=0;
	int i;
	while(getline(&arg, &size, cmdline) != -1)
	{
		if(strstr(arg, "cpu MHz") != NULL) {
	        sscanf(arg+11, "%lf", &mhz[core++]);
		}
	}

	printf("processor	CPU MHZ \n");
	for (i = 0; i < core; ++i)
	{
		printf("%d    %lf\n", i,mhz[i]);
	}
	free(arg);
	fclose(cmdline);
}

void innerCmd(char *cmd){
	if(strcmp(cmd, "pidofexe")==0){
		pidofexe();
	}else if(strcmp(cmd, "cpuinfo")==0){
		cpuinfo();
	}
}



int main(int argc, char *argv[]){
    char buffer[BUFFER_SIZE];
    char *args[ARR_SIZE];
    int *ret_status;
    size_t nargs;
    pid_t pid;
    
    while(1){
        printf("$ ");
        fgets(buffer, BUFFER_SIZE, stdin);
        parse_args(buffer, args, ARR_SIZE, &nargs);

        if(strcmp(args[0], "pidofexe")==0 || strcmp(args[0], "cpuinfo")==0){
        	innerCmd(args[0]);
        	continue;
        }

        if (nargs==0) continue;
        if (!strcmp(args[0], "exit" )) exit(0);       
        pid = fork();
        if (pid){
            printf("Waiting for child (%d)\n", pid);
            pid = wait(ret_status);
            printf("Child (%d) finished\n", pid);
        } else {
            if( execvp(args[0], args)) {
                puts(strerror(errno));
                exit(127);
            }
        }
    }    
    return 0;
}