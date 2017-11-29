#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_BUF 512


// called like ./jms_console -w ./jms_in -r ./jms_out -o operation_file

int main(int argc, char *argv[])
{
    int fdin, fdout, i;
    char *jms_in, *jms_out, *operation_file, *line, buf[MAX_BUF], *input, *token, s[2]=" ";
    FILE *fp;
    size_t len = 0;
    ssize_t read1;
    //Collecting the arguments
    printf ("This program was called with \"%s\".\n",argv[0]);
    if (argc > 1){
        for (i = 1; i < argc; i++){
            printf("argv[%d] = %s\n",i, argv[i] );
            if(strcmp(argv[i], "-w")==0){                   //jms_in
                jms_in = (char*)malloc(strlen(argv[i+1])*sizeof(char));
                strcpy(jms_in, argv[i+1]);
            }
            else if(strcmp(argv[i], "-r")==0){              //jms_out
                jms_out = (char*)malloc(strlen(argv[i+1])*sizeof(char));
                strcpy(jms_out, argv[i+1]);
            }
            else if(strcmp(argv[i], "-o")==0){              //operation file
                operation_file = (char*)malloc(strlen(argv[i+1])*sizeof(char));
                strcpy(operation_file, argv[i+1]);
            }    
        }
    }
    else
        printf("The command had no other arguments.\n");

    /*opens op file and sent each command through pipe*/
    fp = fopen(operation_file, "r");
    if(fp==NULL) perror("Error: Failed to open the file.");
    char temp[MAX_BUF];
    while ((read1 = getline(&line, &len, fp)) != -1){
        if(line[strlen(line)-1]=='\n')
            line[strlen(line)-1]=0;
        char temp[strlen(line)];
        strcpy(temp,line);
        strcpy(buf,line);
        fdin = open(jms_in, O_WRONLY);
        write(fdin, buf, sizeof(buf));
        close(fdin);
        token = strtok(temp, s);
        if(strcmp(token,"submit")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            token=strtok(buf,s);
            printf("JobID: %s, ",token );
            token = strtok(NULL, s);
            printf("PID: %s.\n",token );

        }
        else if(strcmp(token,"status")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            if(strcmp(buf,"not_have")==0){
                token=strtok(NULL,s);
                printf("JobID: %s not found\n",token );
                continue;
            }
            token=strtok(buf,s);
            printf("JobID: %s Status: ",token );
            token=strtok(NULL,s);//state
            if(strcmp(token,"f")==0)
                printf("Finished\n");
            else if(strcmp(token,"s")==0)
                printf("Suspended\n");
            else if(strcmp(token,"a")==0){
                token=strtok(NULL,s);
                printf("Active (running for %s sec)\n",token);
            }
        }
        else if(strcmp(token,"status-all")==0){
            printf("Active jobs\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                token=strtok(buf,s);
                printf("%d. JobID: %s Status: ",i,token);
                token=strtok(NULL,s);//state
                if(strcmp(token,"f")==0)
                    printf("Finished\n");
                else if(strcmp(token,"s")==0)
                    printf("Suspended\n");
                else if(strcmp(token,"a")==0){
                    token=strtok(NULL,s);
                    printf("Active (running for %s sec)\n",token);
                }
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"show-active")==0){
            printf("Active:\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                printf("%d. JobID %s\n",i,buf);
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"show-pools")==0){
            printf("Pools & NumOfJobs:\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                printf("%d. %s\n",i, buf);
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"show-finished")==0){
             printf("Finished jobs:\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                printf("%d. JobID %s\n",i,buf);
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"suspend")==0 || strcmp(token,"resume")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            if(strcmp(buf,"not_have")==0){
                token=strtok(NULL,s);
                printf("JobID: %s not found\n",token );
                continue;
            }
            if(strcmp(token,"suspend")==0)
                printf("Sent suspend signal to JobID %s\n",buf);
            else
                printf("Sent resume signal to JobID %s\n",buf);
        }
        else if(strcmp(token,"shutdown")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            token=strtok(buf,s);
            printf("Served %s jobs, ",token);
            token=strtok(NULL,s);
            printf("%s were still in progress\n",token );
            free(jms_in);
            free(jms_out);
            free(operation_file);
            return 0;
        }
        else if(strcmp(buf,"exit")==0){
            free(jms_in);
            free(jms_out);
            free(operation_file);
            return 0;
        }
    }
    while(1){
        printf("Give a command: ");
        scanf("%s", buf);
        fdin = open(jms_in, O_WRONLY);
        write(fdin, buf, sizeof(buf));
        close(fdin);
        strcpy(temp,buf);
        token = strtok(temp, s);
        if(strcmp(token,"submit")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            token=strtok(buf,s);
            printf("JobID: %s, ",token );
            token = strtok(NULL, s);
            printf("PID: %s.\n",token );

        }
        else if(strcmp(token,"status")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            if(strcmp(buf,"not_have")==0){
                token=strtok(NULL,s);
                printf("JobID: %s not found\n",token );
                continue;
            }
            token=strtok(buf,s);
            printf("JobID: %s Status: ",token );
            token=strtok(NULL,s);//state
            if(strcmp(token,"f")==0)
                printf("Finished\n");
            else if(strcmp(token,"s")==0)
                printf("Suspended\n");
            else if(strcmp(token,"a")==0){
                token=strtok(NULL,s);
                printf("Active (running for %s sec)\n",token);
            }
        }
        else if(strcmp(token,"status-all")==0){
            printf("Active jobs\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                token=strtok(buf,s);
                printf("%d. JobID: %s Status: ",i,token);
                token=strtok(NULL,s);//state
                if(strcmp(token,"f")==0)
                    printf("Finished\n");
                else if(strcmp(token,"s")==0)
                    printf("Suspended\n");
                else if(strcmp(token,"a")==0){
                    token=strtok(NULL,s);
                    printf("Active (running for %s sec)\n",token);
                }
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"show-active")==0){
            printf("Active:\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                printf("%d. JobID %s\n",i,buf);
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"show-pools")==0){
            printf("Pools & NumOfJobs:\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                printf("%d. %s\n",i, buf);
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"show-finished")==0){
             printf("Finished jobs:\n");
            i=0;
            while(1){
                fdout = open(jms_out, O_RDONLY);        
                read(fdout, buf, MAX_BUF);
                close(fdout);
                if(strcmp(buf,"bye")==0)
                    break;
                i++;
                printf("%d. JobID %s\n",i,buf);
                strcpy(buf,"ok");
                fdin = open(jms_in, O_WRONLY);
                write(fdin, buf, sizeof(buf));
                close(fdin);
            }
        }
        else if(strcmp(token,"suspend")==0 || strcmp(token,"resume")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            if(strcmp(buf,"not_have")==0){
                token=strtok(NULL,s);
                printf("JobID: %s not found\n",token );
                continue;
            }
            if(strcmp(token,"suspend")==0)
                printf("Sent suspend signal to JobID %s\n",buf);
            else
                printf("Sent resume signal to JobID %s\n",buf);
        }
        else if(strcmp(token,"shutdown")==0){
            fdout = open(jms_out, O_RDONLY);        
            read(fdout, buf, MAX_BUF);
            close(fdout);
            token=strtok(buf,s);
            printf("Served %s jobs, ",token);
            token=strtok(NULL,s);
            printf("%s were still in progress\n",token );
            free(jms_in);
            free(jms_out);
            free(operation_file);
            return 0;
        }
        else if(strcmp(buf,"exit")==0){
            free(jms_in);
            free(jms_out);
            free(operation_file);
            return 0;
        }
    }
    printf("END CONSOLE\n");
    free(jms_in);
    free(jms_out);
    free(operation_file);
    return 0;
}