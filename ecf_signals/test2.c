#include <unistd.h>
#include <sys/wait.h>
// W(A) means write(1, “A”, sizeof “A”), which will display “A” #define W(x) write(1, #x, sizeof #x)
int main()
{
    write(1, "A", sizeof "A");
    int child = fork();
    write(1, "B", sizeof "B");

    if (child)
        wait(NULL);
    write(1, "C", sizeof "C");
    return 0;
}