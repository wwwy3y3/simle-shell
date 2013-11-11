#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define ARR_SIZE 1024


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

void pidofexe(char** args){
	DIR * dir;
	DIR * dir2;
	FILE *pidStat;
    struct dirent * ptr;
    //dir =opendir("/proc");
    dir =opendir("./");
    char cmds[32768][100]= {"\0"};
    int i;

    while((ptr = readdir(dir))!=NULL)
    {
    	char pathname[100];
        sprintf(pathname,"/proc/%s", ptr->d_name);
        //sprintf(pathname,"./%s", ptr->d_name);
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

void cpuinfo(char** args){
	FILE *cmdline = fopen("/proc/cpuinfo", "rb");
	//FILE *cmdline = fopen("./cpuinfo", "rb");
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

void innerCmd(char *cmd, char** args){
	if(strcmp(cmd, "pidofexe")==0){
		pidofexe(args);
	}else if(strcmp(cmd, "cpuinfo")==0){
		cpuinfo(args);
	}
}



int main(int argc, char *argv[]){
    char buffer[BUFFER_SIZE];
    char *args[ARR_SIZE];
    //cmd args
    char *cmdArgs[ARR_SIZE]= {'\0'};
    //redirect args
    char *redArgs[ARR_SIZE]= {'\0'};
    int *ret_status;
    size_t nargs;
    pid_t pid;
    int i,j;
    int fd;
    
    while(1){
        printf("$ ");
        fgets(buffer, BUFFER_SIZE, stdin);
        parse_args(buffer, args, ARR_SIZE, &nargs);

        //empty
        if (nargs==0) continue;
        //exit
        if (!strcmp(args[0], "exit" )) exit(0);

        fd= 0;
        memset(cmdArgs, '\0', sizeof cmdArgs);
        memset(redArgs, '\0', sizeof redArgs);
        //slice args
        for (i=0,j=0; args[i] != '\0'; ++i)
        {
        	if(strcmp(args[i], ">")==0){
        		redArgs[j++]= args[i++];	
        		redArgs[j++]= args[i++];	
        	}else{
        		cmdArgs[i]= args[i];
        	}
        }

        //outer cmd, fork  
        pid = fork();
        if (pid){
            printf("Waiting for child (%d)\n", pid);
            pid = wait(ret_status);
            printf("Child (%d) finished\n", pid);
        } else {
        	
        if(redArgs[0] != '\0'){ //redirect to file
        		//open file
        		fd = open(redArgs[1], O_RDWR | O_CREAT);
        		//error
        		if(!fd)
        			fprintf(stderr, "Can't open input/create file!\n");
        		//dup
        		dup2(fd, 1);
			}

        //inner command
        if(strcmp(args[0], "pidofexe")==0 || strcmp(args[0], "cpuinfo")==0){
        	innerCmd(args[0], args);
        	exit(127);
        	continue;
        }

            if( execvp(args[0], cmdArgs)) {
            	puts(strerror(errno));
            	if(fd)
            		close(fd);
                exit(127);
            }
        }
    }    
    return 0;
}