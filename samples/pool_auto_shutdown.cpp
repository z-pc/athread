#include "athread/athread.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace at;

int main(void)
{
    AT_COUT("sfdsfsdf");

    ThreadPoolFixed pool(2);
    pool.push(
        []()
        {
            AT_COUT("auto_shutdown: run taks 1");
            this_thread::sleep_for(1s);
            AT_COUT("auto_shutdown: end taks 1");
        });
    pool.push(
        []()
        {
            AT_COUT("auto_shutdown: run taks 2");
            this_thread::sleep_for(1s);
            AT_COUT("auto_shutdown: end taks 2");
        });
    pool.start();
    pool.wait();
    return 0;
}
