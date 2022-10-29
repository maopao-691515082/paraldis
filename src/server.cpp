#include "internal.h"

namespace paraldis
{

static const uint16_t kSvrPort = 6379;

void StartSvr()
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        Die(Sprintf("create listen socket failed: %m"));
    }

    int reuseaddr_on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on)) == -1)
    {
        Die(Sprintf("set listen socket reuse-addr failed: %m"));
    }

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(kSvrPort);

    if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1)
    {
        Die(Sprintf("bind failed: %m"));
    }

    if (listen(listen_fd, 1024) == -1)
    {
        Die(Sprintf("listen failed: %m"));
    }

    reactor::Init();

    if (!reactor::RegListener(listen_fd, [] (int conn_fd) {
        DeliverConnToWorkers(conn_fd);
    }))
    {
        Die("reg listener to reactor failed");
    }

    reactor::Start();
}

}
