#include <stdio.h>
#include "dpts.h"

int main() {
    int value = -1;

    value = init();
    if (value)
        printf("Init Error");

    value = initInput();
    if (value)
        printf("Init Input Error");

    value = analysis();
    if (value)
        printf("analysis Error");
    return 0;
}