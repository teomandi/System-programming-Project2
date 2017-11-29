#include <stdio.h>
int main(int argc, char *argv[]){
	int i;
	for (i = 1; i < argc; i++){
            printf("argv[%d] = %s\n",i, argv[i] );
        }
	for(i=0;i<5;i++)
		printf("Slave: hi\n");
	sleep(10);
	printf("SLAVE: out\n");
}