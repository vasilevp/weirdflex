#include <stdio.h>

extern double test();

int main()
{
    printf("test: %f", test());
}