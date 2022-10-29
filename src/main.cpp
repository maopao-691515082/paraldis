#include "internal.h"

namespace paraldis
{

void Main(int argc, char *argv[])
{
    int worker_count = 1;
    for (int i = 1; i < argc; ++ i)
    {
        if (strcmp(argv[i], "-w") == 0)
        {
            ++ i;
            if (i >= argc)
            {
                Die("no args for flag -w (worker count)");
            }
            worker_count = atoi(argv[i]);
            if (!(worker_count >= 1 && worker_count <= 64))
            {
                Die("invalid worker count (-w)");
            }
        }
        else
        {
            Die(Sprintf("invalid arg or flag: %s", argv[i]));
        }
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        Die("ignore SIGPIPE failed");
    }

    InitWorkers((size_t)worker_count);
    StartSvr();

    Die("bug");
}

}

int main(int argc, char *argv[])
{
    paraldis::Main(argc, argv);
}
