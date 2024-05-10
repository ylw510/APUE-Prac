#include <stdio.h>

void func()
{
    static int counter;
    counter++;
    printf("counter = %d\n", counter);
}

int main()
{
    func();
    func();
    func();
    func();
    func();
}
/*result
counter = 1
counter = 2
counter = 3
counter = 4
counter = 5
*/