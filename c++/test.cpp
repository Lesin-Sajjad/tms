#include <iostream>
#include <windows.h>
#include "Use1401.h"

#define NUMSAMP 5000

int main()
{
    short hand = U14Open1401(0);

    if (hand < 0)
    {
        std::cout << "Failed to open 1401\n";
        return -1;
    }

    std::cout << "1401 opened\n";

    // load ADCMEM command
    U14Ld(hand, "", "kill,adcmem");

    // start acquisition (10 kHz, channels 0–3)
    char cmd[128];
    sprintf(cmd,"ADCMEM,I,2,0,%d,0 3,1,C,10,10;",NUMSAMP*2);

    U14SendString(hand, cmd);

    long status;

    // wait until capture complete
    do
    {
        U14SendString(hand,"ADCMEM,?;");
        U14LongsFrom1401(hand,&status,1);

    } while(status != 0);

    short buffer[NUMSAMP];

    // fast block transfer
    U14ToHost(hand,(char*)buffer,sizeof(buffer),0,0);

    std::cout << "Data received\n";

    for(int i=0;i<20;i++)
        std::cout << buffer[i] << "\n";

    U14Close1401(hand);

    return 0;
}