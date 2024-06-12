#include "m-os-api.h"

void testFn() {
    goutf("I'm testFn()\n");
}

void testTask1() {
    goutf("Hello, murmulator!\n");
    goutf("I'm testTask1()\n");
    testFn();
}
