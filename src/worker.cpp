#include "internal.h"

namespace paraldis
{

static void OnConnDataRecved(int conn_fd, const char *data, size_t len)
{
    //echo test
    if (len > 0)
    {
        reactor::WriteConn(conn_fd, data, len);
    }
}

class Worker
{
    size_t idx_;

    std::mutex new_conn_queue_lock_;
    std::vector<int> new_conns_;
    int new_conn_notify_fd_;

    void ThreadMain()
    {
        reactor::Init();

        if (!reactor::RegEvFd(new_conn_notify_fd_, [this] (uint64_t) {
            std::vector<int> new_conns;
            {
                std::lock_guard<std::mutex> lg(new_conn_queue_lock_);
                new_conns = std::move(new_conns_);
                new_conns_.clear();
            }
            for (auto conn_fd : new_conns)
            {
                if (!reactor::RegConn(conn_fd, OnConnDataRecved))
                {
                    Log("reg new conn failed");
                    close(conn_fd);
                }
            }
        }))
        {
            Die("reg new_conn_notify_fd_ to reactor failed");
        }

        reactor::Start();

        Die("bug");
    }

public:

    Worker(size_t idx) : idx_(idx)
    {
        new_conn_notify_fd_ = eventfd(0, 0);
        if (new_conn_notify_fd_ == -1)
        {
            Die(Sprintf("create eventfd failed: %m"));
        }

        std::thread(&Worker::ThreadMain, this).detach();
    }

    void DeliverConn(int conn_fd)
    {
        {
            std::lock_guard<std::mutex> lg(new_conn_queue_lock_);
            new_conns_.emplace_back(conn_fd);
        }
        uint64_t count = 1;
        if (write(new_conn_notify_fd_, &count, sizeof(count)) == -1)
        {
            Die(Sprintf("write to new_conn_notify_fd_ failed: %m"));
        }
    }
};

static std::vector<Worker *> workers;

void InitWorkers(size_t worker_count)
{
    workers.resize(worker_count);
    for (size_t i = 0; i < worker_count; ++ i)
    {
        workers[i] = new Worker(i);
    }
}

void DeliverConnToWorkers(int conn_fd)
{
    static thread_local size_t next_idx = 0;
    workers[next_idx % workers.size()]->DeliverConn(conn_fd);
    ++ next_idx;
}

}
