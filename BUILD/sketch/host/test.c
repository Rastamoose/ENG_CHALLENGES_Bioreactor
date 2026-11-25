#line 1 "/home/harris/Programming/Uni/Bioreactor/host/test.c"
#include <stdio.h>
#include <time.h>

int main(){
    time_t rawtime;
    struct tm* timeInfo;

    time(&rawtime);
    timeInfo = localtime(&rawtime);
    char* time = asctime(timeInfo);
    printf("Time: %s\n", time);

    return 0;

}