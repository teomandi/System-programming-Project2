#include <stdio.h>
#include <time.h>
#include <string.h>
int main(void){
	char buf[30];
	int i =400;
	//sleep(3);
	sprintf(buf, " %d", i);

	printf("%s %d\n",buf, strlen(buf) );
	int k = time(NULL)-i;
	printf("!!!!timo: %d\n",k);
	printf("tt: out\n");
}