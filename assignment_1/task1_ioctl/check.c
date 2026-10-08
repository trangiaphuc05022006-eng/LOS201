#include <stdio.h>
#include "asgn1_ioctl.h"

static void show(const char *name, unsigned int cmd)
{
    printf("%-20s cmd=0x%08x  dir=%u type='%c' nr=%u size=%u\n",
           name, cmd, _IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));
}

int main(void)
{
    show("ASGN1_RESET_BUFFER", ASGN1_RESET_BUFFER);
    show("ASGN1_GET_STATS",    ASGN1_GET_STATS);
    show("ASGN1_SET_MODE",     ASGN1_SET_MODE);
    show("ASGN1_GET_VERSION",  ASGN1_GET_VERSION);
    return 0;
}