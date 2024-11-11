#include <stdio.h>
#include "sdkconfig.h"
#include "greeting.h"

void greeting(void)
{
    printf("%s\r\n", CONFIG_SUBCORE_GREETING_MESSAGE);
}
