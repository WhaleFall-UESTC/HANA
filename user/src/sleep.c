#include <ulib.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        write(2, "Usage: sleep ticks\n", 20);
        exit(1);
    }

    int n = atoi(argv[1]);
    sleep(n);

    exit(0);
}