#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define MAX_BUF 512

typedef struct Pool_data{
	char* path;
	int our_pid;
	int id;
	int size;
	int *pids;
	int *logic_nums;
	int *exec_time;
	int *run_time;
	char *givePipe; 			//name of the pipe we return info
	char *getPipe;				//name of the pipe we take info
	char *state; 				//char array shows the state of each S:suspended, F:finished, A:active
	char ***args_job;			//its an array with all the arguments for every job
	int *arg_size;
}Pool_data;
Pool_data *Pool;

void printPool(Pool_data *Pool){
	printf("\t*PRINT POOL*\nid: %d size: %d jobs: %d\n",Pool->id,Pool->size);
	printf("PIPES:\nGive-->%s\nGet-->%s\n",Pool->givePipe, Pool->getPipe);
	int i;
	for(i=0;i<Pool->size;i++)
		printf("pid: %d == logic_num: %d == state: %c\n",Pool->pids[i], Pool->logic_nums[i], Pool->state[i]);
	printf("\t-DONE PRINT-\n");
}
void sighandler(){
	printf("~~~~~~~SIGNAL_HANDLE_p:%s\n",Pool->getPipe);

	int count=0, act=0, i, j, fdget;
	char buf2[MAX_BUF];
	for(i=0;i<Pool->size;i++){
		if(Pool->pids[i]!=0){
			count++;
			if(Pool->state[i]!='f'){
				for(j=0;j<Pool->arg_size[i];j++)
   					free(Pool->args_job[i][j]);
   				free(Pool->args_job[i]);
				kill(Pool->pids[i], SIGTERM);
				if(Pool->state[i]=='a') act++;
			}
		}
	}
	sprintf(buf2,"%d %d",count,act);

	printf("buf2: %s|||| getPipe:%s\n",buf2, Pool->getPipe);
	fdget = open(Pool->getPipe, O_WRONLY);//<----segmatention fault here
	printf("gere\n");
    write(fdget, buf2, sizeof(buf2));
	close(fdget);
	printf("QQQQQQQQQQQQQQQQQq\n");
	unlink(Pool->givePipe);	
	unlink(Pool->getPipe);
   	free(Pool->givePipe);
   	free(Pool->getPipe);
   	free(Pool->path);
   	free(Pool->logic_nums);
   	free(Pool->pids);
   	free(Pool->state);
   	free(Pool->exec_time);
   	free(Pool->run_time);
   	free(Pool->args_job);
   	free(Pool->arg_size);
   	free(Pool);
}


int main(int argc, char *argv[]){
	signal(SIGTERM, sighandler);
	int i, fdgive, fdget, pid, fdlearn, fdinform ,jnum=0;
	char buf[MAX_BUF], buf2[MAX_BUF], id[4], *inform, *learn;
    Pool = malloc(sizeof(Pool_data));

	if (argc > 1){
        for (i = 0; i < argc; i++){
            if(i==1){
            	Pool->givePipe = (char*)malloc(strlen(argv[i])*sizeof(char));
            	strcpy(Pool->givePipe, argv[i]);
            }
            else if(i==2){
            	Pool->getPipe = (char*)malloc(strlen(argv[i])*sizeof(char));
           	    strcpy(Pool->getPipe,argv[i]);
            }
            else if(i==3)
            	Pool->id = atoi(argv[i]);
            else if(i==4)
            	Pool->size = atoi(argv[i]);
            else if(i==5){	
            	Pool->path = (char*)malloc(strlen(argv[i])*sizeof(char));
                strcpy(Pool->path, argv[i]);
            }
        }
    }
    else{
    	perror("Pool: NO arguments\n");
    	exit(1);
    }
    fdgive = open(Pool->givePipe, O_RDONLY);
    read(fdgive, buf, MAX_BUF);
   	close(fdgive);
   	Pool->our_pid=atoi(buf);
   	strcpy(buf2,"ok");
   	fdget = open(Pool->getPipe, O_WRONLY);
    write(fdget, buf2, sizeof(buf2));
	close(fdget);
	Pool->pids = malloc(sizeof(int)*Pool->size);
	Pool->logic_nums = malloc(sizeof(int)*Pool->size);
	Pool->state = malloc(sizeof(char)*Pool->size);
	Pool->exec_time = malloc(sizeof(int)*Pool->size);
	Pool->run_time = malloc(sizeof(int)*Pool->size);

	Pool->args_job = malloc(sizeof(char**)*Pool->size);
	Pool->arg_size = malloc(sizeof(int)*Pool->size);
	for(i=0;i<Pool->size;i++){
		Pool->pids[i]=0;
		Pool->logic_nums[i]=0;
		Pool->state[i] ='-';
		Pool->exec_time[i]=0;
    	Pool->run_time[i] =0;
    	Pool->arg_size[i]=0;
	}    
    char *token, s[2]=" ", *job;
    int key, k, flag;
    while(1){	/*kapou edw 3ekinaei ena mBuf:egalo while*/
    	fdgive = open(Pool->givePipe, O_RDONLY);
    	read(fdgive, buf, MAX_BUF);
   		close(fdgive);
   		//check for waste
   		char temp[MAX_BUF];
   		printf("---POOL:get: %s\n",buf );
   		strcpy(temp, buf);
   		token = strtok(temp, s);
		if(strcmp(token,"submit")==0){
			k=-1;
			for(i=0;i<Pool->size;i++){
				if(Pool->pids[i]==0){
					k=i;
					break;
				}
			}
			if(k==-1){
				strcpy(buf2,"NoSpace");
				fdget = open(Pool->getPipe, O_WRONLY);
    		    write(fdget, buf2, sizeof(buf2));
				close(fdget);
				continue;			//the big while
			}


			Pool->arg_size[k]=0;		//fixing the array with job's arguments
			int j=0;
			while(buf[j]!=0){
				if(buf[j]==' ')Pool->arg_size[k]++;
				j++;
			}
			Pool->args_job[k] = malloc(sizeof(char*)*Pool->arg_size[k]);
			for(i=0;i<Pool->arg_size[k];i++){
				token = strtok(NULL, s);
				Pool->args_job[k][i]=malloc(sizeof(char)*(strlen(token)+1));
				strcpy(Pool->args_job[k][i],token);
			}

			jnum++;
			Pool->logic_nums[k] = jnum+(Pool->id-1)*Pool->size;
			Pool->state[k]='a';
			Pool->exec_time[k] = time(NULL);
			Pool->pids[k]=fork();
			if(Pool->pids[k]<0){
				perror("Pool error at fork");
				exit(1);
			}
			if(Pool->pids[k]==0){
				char name_folder[30], file_name[40], file[40];
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				strcpy(file,Pool->path);
				sprintf(name_folder,"/sdi1200098_%d_%d%d%d_%d%d%d",Pool->logic_nums[k], tm.tm_year+ 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min,tm.tm_sec);
				strcat(file,name_folder);
				mkdir(file, 0755);
				sprintf(file_name,"%s/stdout",file);
				printf("job %s\n", Pool->args_job[k][0]);
				int fd1 = open(file_name, O_WRONLY | O_CREAT, 0666);
				dup2(fd1, 1);   // make stdout go to file 1
				strcpy(file,Pool->path);//we remake it becasue it got destroyed
				sprintf(name_folder,"/sdi1200098_%d_%d%d%d_%d%d%d",Pool->logic_nums[k], tm.tm_year+ 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min,tm.tm_sec);
				strcat(file,name_folder);				
				sprintf(file_name,"%s/stderr",file);
				int fd2 = open(file_name, O_WRONLY | O_CREAT, 0755);
			    dup2(fd2, 2);
		    	close(fd1);
		    	close(fd2);
				execv(Pool->args_job[k][0], Pool->args_job[k]);
			}
			else{
				char num[7];							//sent to coord the pid number;
				sprintf(num,"%d ",Pool->logic_nums[k]);
				strcpy(buf2,num);
				sprintf(num,"%d", Pool->pids[k]);
				strcat(buf2,num);
				fdget = open(Pool->getPipe, O_WRONLY);
    		    write(fdget, buf2, sizeof(buf2));
				close(fdget);

			}
		}
		else if(strcmp(token,"exit")==0){
			printf("Pool %d EXIT!\n",Pool->id );
			int j;
			for(i=0;i<Pool->size;i++){
			if(Pool->pids[i]!=0){
				for(j=0;j<Pool->arg_size[i];j++)
   					free(Pool->args_job[i][j]);
   				free(Pool->args_job[i]);
				kill(Pool->pids[i], SIGTERM);
			}
		}
			unlink(Pool->givePipe);	
			unlink(Pool->getPipe);
		   	free(Pool->givePipe);
   			free(Pool->getPipe);
		   	free(Pool->path);
   			free(Pool->logic_nums);
	   		free(Pool->pids);
	   		free(Pool->state);
   			free(Pool->exec_time);
	   		free(Pool->run_time);
		   	free(Pool->args_job);
   			free(Pool->arg_size);
	   		free(Pool);
			break;
		}
		else if(strcmp(token,"status")==0){
			token = strtok(NULL, s);
			int found=0, jobID;
			jobID = atoi(token);
			for(i=0;i<Pool->size;i++){
				if(Pool->logic_nums[i] == jobID){
					found =1;
					char t[6], t2[2];
					sprintf(t,"%d ", jobID);
					strcpy(buf2, t);
					t2[0]=Pool->state[i];
					t2[1]=0;
					strcat(buf2,t2);
					if(Pool->state[i] == 'a'){
						int timme = time(NULL) - Pool->exec_time[i];
						sprintf(t2," %d", timme);
						strcat(buf2, t2);
					}
				}
			}
			if(found==0)//not found
				strcpy(buf2,"not_have");
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);//write back to coord the buf2
		}
		else if(strcmp(token,"status-all")==0){
			token = strtok(NULL, s);
			int time_dur=0;
			if(token!=NULL)
				time_dur = atoi(token);
			for(i=0;i<Pool->size;i++){
				if(Pool->pids[i]!=0){
					if(time_dur!=0){
						if(time_dur-Pool->exec_time[i]<0)
							continue;//if it exec time is more than time duration cansel it
					}
					char t[6], t2[2];
					sprintf(t,"%d ", Pool->logic_nums[i]);
					strcpy(buf2, t);
					t2[0]=Pool->state[i];
					t2[1]=0;
					strcat(buf2,t2);
					if(Pool->state[i] == 'a'){
						int timme = time(NULL) - Pool->exec_time[i];
						sprintf(t," %d", timme);
						strcat(buf2, t);
					}
				}
				else
					continue;
				
				fdget = open(Pool->getPipe, O_WRONLY);
    			write(fdget, buf2, sizeof(buf2));
				close(fdget);//write back to coord the buf2
				//wait to read
				fdgive = open(Pool->givePipe, O_RDONLY);
    			read(fdgive, buf, MAX_BUF);
	   			close(fdgive);
	   			if(strcmp(buf,"ok")==0)
	   				continue;
			}
			strcpy(buf2,"bye");
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);//write back to coord the buf2
		}
		else if(strcmp(token,"show-active")==0){
			for(i=0;i<Pool->size;i++){
				if(Pool->state[i]=='a'){
					sprintf(buf2,"%d ", Pool->logic_nums[i]);
					fdget = open(Pool->getPipe, O_WRONLY);
    				write(fdget, buf2, sizeof(buf2));
					close(fdget);
					fdgive = open(Pool->givePipe, O_RDONLY);
	    			read(fdgive, buf, MAX_BUF);
		   			close(fdgive);
	   				if(strcmp(buf,"ok")==0)
	   					continue;
	   			}
	   		}
	   		strcpy(buf2,"bye");
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);
		}
		else if(strcmp(token,"show-pools")==0){
			k=0;
			for(i=0;i<Pool->size;i++)
				if(Pool->pids[i]!=0)
					k++;
			sprintf(buf2,"%d %d",Pool->our_pid,k);
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);
		}
		else if(strcmp(token,"show-finished")==0){
			for(i=0;i<Pool->size;i++){
				if(Pool->state[i]=='f'){
					sprintf(buf2,"%d", Pool->logic_nums[i]);
					fdget = open(Pool->getPipe, O_WRONLY);
    				write(fdget, buf2, sizeof(buf2));
					close(fdget);
					fdgive = open(Pool->givePipe, O_RDONLY);
	    			read(fdgive, buf, MAX_BUF);
			   		close(fdgive);
		   			if(strcmp(buf,"ok")==0)
		   				continue;
				}
			}
			strcpy(buf2,"bye");
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);
		}
		else if(strcmp(token,"suspend")==0){
			token = strtok(NULL, s);
			int found=0, jobID;
			jobID = atoi(token);
			for(i=0;i<Pool->size;i++){
				if(Pool->logic_nums[i] == jobID){
					found =1;
					kill(Pool->pids[i], SIGSTOP);
					Pool->state[i]='s';
					Pool->run_time[i]=Pool->run_time[i]+time(NULL)-Pool->exec_time[i];
					sprintf(buf2,"%d",Pool->logic_nums[i]);
				}
			}
			if(found==0)//not found
				strcpy(token,"not_have");
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);//write back to coord the buf2
		}
		else if(strcmp(token,"resume")==0){
			token = strtok(NULL, s);
			int found=0, jobID;
			jobID = atoi(token);
			for(i=0;i<Pool->size;i++){
				if(Pool->logic_nums[i] == jobID){
					if(Pool->state[i] =='s'){
						Pool->state[i]='a';
						Pool->exec_time=time(NULL);
						kill(Pool->pids[i], SIGCONT);
					}
					found =1;
					sprintf(buf2,"%d",Pool->logic_nums[i]);
				}
			}
			if(found==0)//not found
				strcpy(buf2,"not_have");
			fdget = open(Pool->getPipe, O_WRONLY);
    		write(fdget, buf2, sizeof(buf2));
			close(fdget);					
		}

		flag=0;
		int chck,j;//update our data
		for(i=0;i<Pool->size;i++){
			if(Pool->pids[i]!=0){
				if(Pool->state[i]== 'a'){
					chck = waitpid(Pool->pids[i], NULL, WNOHANG);
   					if(chck < 0 ){
   						perror("Pool00: error waitpid");
   						exit(1);
   					}
   					else if(chck == Pool->pids[i]){
   						Pool->state[i]='f';
   						for(j=0;j<Pool->arg_size[i];j++)
   							free(Pool->args_job[i][j]);
   						free(Pool->args_job[i]);
   						printf("process: %d finished\n",Pool->pids[i]);
   					}
				}
			}
			if(Pool->state[i]!='f')
				flag = 1 ;//check if all the jobs are complete
		}		
		if(!flag){//all the jobs are finished	END THE POOL!!!!
			printf("Pool: All finished. Exit.\n");
			break;
		}
	}   
    return 0;
}