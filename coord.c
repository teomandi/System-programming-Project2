#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_BUF 512

typedef struct node{
	int pid;
	struct node *next;
}node;



int main(int argc, char *argv[]){
    int i, fdin, fdout, fdgive, fdget,  pools=0, logicJob=0,pid;
    char buf[MAX_BUF],buf2[MAX_BUF], s[2]=" ", *token, *jms_in, *jms_out, *path, *jobs_pool;

    //Collecting the arguments
    printf ("This program was called with \"%s\".\n",argv[0]);
    if (argc > 1){
        for (i = 1; i < argc; i++){
            printf("argv[%d] = %s\n",i, argv[i] );
            if(strcmp(argv[i], "-r")==0){                   //jms_in	here we read to in
                jms_in = (char*)malloc(strlen(argv[i+1])*sizeof(char));
                strcpy(jms_in, argv[i+1]);
            }
            else if(strcmp(argv[i], "-w")==0){              //jms_out	here we write to out 
              	jms_out = (char*)malloc(strlen(argv[i+1])*sizeof(char));
                strcpy(jms_out, argv[i+1]);
            }
            else if(strcmp(argv[i], "-l")==0){
                path = (char*)malloc(strlen(argv[i+1])*sizeof(char));
                strcpy(path, argv[i+1]);
            }
            else if(strcmp(argv[i], "-n")==0) {             //pool's job number
            	jobs_pool = (char*)malloc(strlen(argv[i+1])*sizeof(char));
            	strcpy(jobs_pool, argv[i+1]);
            }
        }
    }
    else
        printf("The command had no other arguments.\n");
    
    mkfifo( , 0666);  // create the FIFO (named pipe)
    mkfifo(jms_out, 0666);
    node *head=NULL;//head of the list
    printf("START\n");

    char *foruse1 = "/home/teomandi/Desktop/E2/pool_get";			//pool gets information through this file
    char *foruse2 = "/home/teomandi/Desktop/E2/pool_give";			//pool gives information through this file	

   	while(1){
   		char *id[4], lj[5], *get, *give;
   		int  fdgive, fdget;
   		fdin = open(jms_in, O_RDONLY);    	
   		read(fdin, buf, MAX_BUF);
   		close(fdin);
   		char temp[MAX_BUF];
   		strcpy(temp,buf);
   		token = strtok(temp, s);
   		printf("==Coord:GOT: %s \n",buf);
		if(strcmp(token,"submit")==0){
			logicJob++;
			token = strtok(NULL, s);
			if(pools==0){//we have no pools
				pools++;
				sprintf(id,"%d",pools);
				get = (char*)malloc((strlen(foruse1)+strlen(id))*sizeof(char));
				strcpy(get, foruse1);
				strcat(get,id);
				give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
				strcpy(give, foruse2);
				strcat(give,id);
				mkfifo(get, 0666);//those should be destoroyed (in pool)
				mkfifo(give, 0666);
				node *temp=malloc(sizeof(node));
				temp->next=NULL;
				head=temp;
				temp->pid = fork();
				if(temp->pid < 0){
					perror("Coord: Error at fork.");
					exit(1);
				}
				else if(temp->pid == 0)
					execlp("./pool","/pool", give, get, id, jobs_pool, path, NULL);
				else{
					sprintf(buf2,"%d",temp->pid);//sent his pid
					fdgive = open(give, O_WRONLY);
        			write(fdgive, buf2, sizeof(buf2));
				    close(fdgive);
				    fdget = open(get, O_RDONLY);    	
   					read(fdget, buf2, MAX_BUF);
   					close(fdget);
					printf("First POOL done*Buf:%s\n",buf );

					fdgive = open(give, O_WRONLY);
        			write(fdgive, buf, sizeof(buf));
			        close(fdgive);//submit ...
				    fdget = open(get, O_RDONLY);    	
   					read(fdget, buf2, MAX_BUF);
   					close(fdget);
   					if(strcmp(buf2,"NoSpace")==0){
   						perror("Coord: No space the first pool");
   						exit(1);
   					}
   					free(give);
					free(get);
   				}
   			}
   			else{//put it in the first available pool
   				int done=0;
   				node *temp;
				temp=head;
   				for(i=1;i<=pools;i++){
   					if(temp==NULL)break;
   					if(waitpid(temp->pid, NULL, WNOHANG)==temp->pid){
   						if(temp->next!=NULL){
   							temp=temp->next;
   							continue;
   						}
   						else
   							break;
   					}
   					sprintf(id,"%d",i);
					get = (char*)malloc((strlen(foruse1)+strlen(id))*sizeof(char));
					strcpy(get, foruse1);
					strcat(get,id);
					give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
					strcpy(give, foruse2);
					strcat(give,id);
	
					fdgive = open(give, O_WRONLY);
        			write(fdgive, buf, sizeof(buf));
			        close(fdgive);//submit ...

			        fdget = open(get, O_RDONLY);    	
   					read(fdget, buf2, MAX_BUF);				
   					close(fdget);
   					free(give);
					free(get);
   					if(strcmp(buf2,"NoSpace")==0)
   						continue;
   					else{
   						done=1;
   						break;
   					}
   					temp=temp->next;
   				}
   				if(done==0){								//that means that all the pools were full so we create a new
   					printf("CREATE NEW POOL\n");
   					pools++;
   					sprintf(id,"%d",pools);
					get = (char*)malloc((strlen(foruse1)+strlen(id))*sizeof(char));
					strcpy(get, foruse1);
					strcat(get,id);
					give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
					strcpy(give, foruse2);
					strcat(give,id);
					mkfifo(get, 0666);			//those should be destoroed
					mkfifo(give, 0666);

					node *temp=head;
					node *new=malloc(sizeof(node));
					while(temp->next!=NULL)
						temp=temp->next;
					temp->next=new;
					new->next=NULL;
					new->pid = fork();
					if(new->pid < 0){
						perror("Coord: Error at fork.");
						exit(1);
					}
					else if(new->pid == 0)
						execlp("./pool","/pool", give, get, id, jobs_pool, path, NULL);
					else{
						sprintf(buf2,"%d",new->pid);
						fdgive = open(give, O_WRONLY);
        				write(fdgive, buf2, sizeof(buf2));
					    close(fdgive);//sent his pid
					    fdget = open(get, O_RDONLY);    	
   						read(fdget, buf2, MAX_BUF);
	   					close(fdget);//ok
	   					fdgive = open(give, O_WRONLY);
        				write(fdgive, buf, sizeof(buf));
					    close(fdgive);//sent submit
				        fdget = open(get, O_RDONLY);    	
   						read(fdget, buf2, MAX_BUF);				//t: THAT WAITS UNTIL SOMETHIN GOT WRITEN;
   						close(fdget);
   						if(strcmp(buf2,"NoSpace")==0){
   							perror("Coord: No space to just created pool");
   							exit(1);
	   					}
   						free(give);
						free(get);
   					}
   				}
			}
			fdout = open(jms_out, O_WRONLY); 
    		write(fdout, buf2, sizeof(buf2));
        	close(fdout);
        }//here ends submit if
		else if(strcmp(token,"exit")==0){
			strcpy(buf,"exit");
			node *temp;
			temp=head;
			for(i=1;i<=pools;i++){
				if(waitpid(temp->pid, NULL, WNOHANG)==temp->pid){
   					if(temp->next!=NULL){
   						temp=temp->next;
   						continue;
   					}
   					else
   						break;
   				}
   				sprintf(id,"%d",i);
				give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
				strcpy(give, foruse2);
				strcat(give,id);
			
				fdgive = open(give, O_WRONLY);
	        	write(fdgive, buf, sizeof(buf));
				close(fdgive);
				free(give);
				temp=temp->next;
			}
			printf("Coord EXIT\n");
			break;
		}
		else if(strcmp(token,"status")==0 || strcmp(token,"suspend")==0 || strcmp(token,"resume")==0){	//same way
			token = strtok(NULL, s);
			node *temp;
			temp=head;
			for(i=1;i<=pools;i++){
				if(temp==NULL)break;
				if(waitpid(temp->pid, NULL, WNOHANG)==temp->pid){
   					if(temp->next!=NULL){
   						temp=temp->next;
   						continue;
   					}
   					else
   						break;
   				}

   				sprintf(id,"%d",i);
				give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
				strcpy(give, foruse2);
				strcat(give,id);
				get = (char*)malloc((strlen(foruse1)+strlen(id))*sizeof(char));
				strcpy(get, foruse1);
				strcat(get,id);

				fdgive = open(give, O_WRONLY);
	        	write(fdgive, buf, sizeof(buf));
				close(fdgive);
				//wait for answer
				fdget = open(get, O_RDONLY);    	
   				read(fdget, buf2, MAX_BUF);	
   				close(fdget);
   				if(strcmp(buf2,"not_have")==0)
   					continue;
   				else
   					break;//send answer to console.
   				free(get);
   				free(give);
   				temp=temp->next;
			}    
			fdout = open(jms_out, O_WRONLY); 
    		write(fdout, buf2, sizeof(buf2));//answer could be "not_have"!
        	close(fdout);//answer to console
		}
		else if(strcmp(token, "status-all")==0 || strcmp(token,"show-active")==0 || strcmp(token,"show-finished")==0){//same use they have
			node *temp;
			temp=head;
			for(i=1;i<=pools;i++){
				if(temp==NULL)break;
				if(waitpid(temp->pid, NULL, WNOHANG)==temp->pid){
   					if(temp->next!=NULL){
   						temp=temp->next;
   						continue;
   					}
   					else
   						break;
   				}

   				sprintf(id,"%d",i);
				give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
				strcpy(give, foruse2);
				strcat(give,id);
				get = (char*)malloc((strlen(foruse1)+strlen(id))*sizeof(char));
				strcpy(get, foruse1);
				strcat(get,id);

				fdgive = open(give, O_WRONLY);
	        	write(fdgive, buf, sizeof(buf));
				close(fdgive);
				while(1){
					fdget = open(get, O_RDONLY);    	
   					read(fdget, buf2, MAX_BUF);				
   					close(fdget);
        			if(strcmp(buf2,"bye")==0)
        				break;
        			fdout = open(jms_out, O_WRONLY);
    				write(fdout, buf2, sizeof(buf2));
        			close(fdout);//sent to console at the same moment

        			fdin = open(jms_in, O_RDONLY);    	
   					read(fdin, buf2, MAX_BUF);
	   				close(fdin);
	   				if(strcmp(buf2,"ok")==0){
	   					fdgive = open(give, O_WRONLY);
		        		write(fdgive, buf2, sizeof(buf2));
						close(fdgive);
	   				}
				}
				free(give);
				free(get);
				temp=temp->next;
			}

			strcpy(buf2,"bye");//maybe useless
			fdout = open(jms_out, O_WRONLY);
    		write(fdout, buf2, sizeof(buf2));
        	close(fdout);//sent to console to stop
		}
		else if(strcmp(token,"show-pools")==0){
			node *temp;
			temp=head;
			for(i=1;i<=pools;i++){
				if(temp==NULL)break;
				if(waitpid(temp->pid, NULL, WNOHANG)==temp->pid){
   					if(temp->next!=NULL){
   						temp=temp->next;
   						continue;
   					}
   					else
   						break;
   				}
   				sprintf(id,"%d",i);
				give = (char*)malloc((strlen(foruse2)+strlen(id))*sizeof(char));
				strcpy(give, foruse2);
				strcat(give,id);
				get = (char*)malloc((strlen(foruse1)+strlen(id))*sizeof(char));
				strcpy(get, foruse1);
				strcat(get,id);
				fdgive = open(give, O_WRONLY);
	        	write(fdgive, buf, sizeof(buf));
				close(fdgive);

				fdget = open(get, O_RDONLY);    	
   				read(fdget, buf2, MAX_BUF);				
   				close(fdget);
   				fdout = open(jms_out, O_WRONLY);
	    		write(fdout, buf2, sizeof(buf2));
        		close(fdout);
        		fdin = open(jms_in, O_RDONLY);    	
   				read(fdin, buf2, MAX_BUF);
	   			close(fdin);
	   			temp=temp->next;

        	}
        	strcpy(buf2,"bye");//maybe useless
			fdout = open(jms_out, O_WRONLY);
    		write(fdout, buf2, sizeof(buf2));
        	close(fdout);
		}
		else if(strcmp(token,"shutdown")==0){
			int count=0, act=0;
			node *temp;
			temp=head;
			for(i=1;i<=pools;i++){
				if(temp==NULL)break;
				if(waitpid(temp->pid, NULL, WNOHANG)==temp->pid){
   					if(temp->next!=NULL){
   						temp=temp->next;
   						continue;
   					}
   					else
   						break;
   				}
   				kill(temp->pid, SIGTERM);
   				fdget = open(get, O_RDONLY);    	
   				read(fdget, buf2, MAX_BUF);				
   				close(fdget);
   				token=strtok(buf2,s);
   				count=count+atoi(token);
   				token=strtok(NULL,s);
   				act=act+atoi(token);
   				temp=temp->next;
   			}
   			sprintf(buf2, "%d %d",count, act);
   			fdout = open(jms_out, O_WRONLY);
    		write(fdout, buf2, sizeof(buf2));
        	close(fdout);
			break;
		}
    }
    printf("Console exit.\n");
    unlink(jms_in);       //remove the FIFO 
    unlink(jms_out);
    free(jms_in);
    free(jms_out);
    free(path);
    node *temp2,*temp;
    temp = head;
    while(temp!=NULL){
    	temp2=temp->next;
    	free(temp);//frree memory
    	temp=temp2;
    }
    return 0;
}//program
