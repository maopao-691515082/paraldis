#include "internal.h"

namespace paraldis
{

namespace reactor
{

static thread_local std::map<int, std::function<void (int)>> read_handlers;

class WriteBuf
{
    std::list<std::string> q_;
    size_t head_ = 0;

public:

    bool Empty() const
    {
        return q_.empty();
    }

    bool DoWrite(int fd)
    {
        while (!Empty())
        {
            auto const &s = q_.front();
            const char *p = s.data() + head_;
            size_t len = s.size() - head_;
            while (len > 0)
            {
                ssize_t ret = write(fd, p, len);
                if (ret == -1)
                {
                    if (errno == EAGAIN)
                    {
                        return true;
                    }
                    Log(Sprintf("write failed: %m"));
                    return false;
                }
                head_ += (size_t)ret;
                p += ret;
                len -= (size_t)ret;
            }
            q_.pop_front();
        }
        return true;
    }

    void AddData(const char *buf, size_t len)
    {
        if (q_.empty() || q_.back().size() + len > 4096)
        {
            q_.emplace_back(buf, len);
        }
        else
        {
            q_.back().append(buf, len);
        }
    }
};
static thread_local std::map<int, WriteBuf> write_bufs;

static thread_local std::set<int> ready_read_fds;

static thread_local int ep_fd = -1;

static void CloseFd(int fd)
{
    read_handlers.erase(fd);
    write_bufs.erase(fd);
    ready_read_fds.erase(fd);
    close(fd);
}

static bool RegPrepare(int fd)
{
    if (read_handlers.count(fd) != 0)
    {
        return false;
    }

    int flags = 1;
    if (ioctl(fd, FIONBIO, &flags) == -1)
    {
        return false;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        return false;
    }

    return true;
}

bool RegListener(int fd, std::function<void (int)> on_new_conn)
{
    if (!RegPrepare(fd))
    {
        return false;
    }

    read_handlers[fd] = [on_new_conn] (int listen_fd) {
        for (;;)
        {
            int conn_fd = accept(listen_fd, nullptr, nullptr);
            if (conn_fd == -1)
            {
                if (errno != EAGAIN)
                {
                    Log(Sprintf("accept failed: %m"));
                    CloseFd(listen_fd);
                }
                return;
            }

            int enable = 1;
            setsockopt(conn_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

            on_new_conn(conn_fd);
        }
    };

    return true;
}

bool RegEvFd(int fd, std::function<void (uint64_t)> on_ev)
{
    if (!RegPrepare(fd))
    {
        return false;
    }

    read_handlers[fd] = [on_ev] (int ev_fd) {
        for (;;)
        {
            uint64_t count;
            if (read(ev_fd, &count, sizeof(count)) == -1)
            {
                if (errno != EAGAIN)
                {
                    Log(Sprintf("read evfd failed: %m"));
                    CloseFd(ev_fd);
                }
                return;
            }

            on_ev(count);
        }
    };

    return true;
}

bool RegConn(int fd, std::function<void (int, const char *, size_t)> on_data_recved)
{
    if (!RegPrepare(fd))
    {
        return false;
    }

    read_handlers[fd] = [on_data_recved] (int conn_fd) {
        for (;;)
        {
            char buf[4096];
            ssize_t ret = read(conn_fd, buf, sizeof(buf));
            if (ret == -1)
            {
                if (errno != EAGAIN)
                {
                    Log(Sprintf("read from conn failed: %m"));
                    CloseFd(conn_fd);
                }
                return;
            }

            on_data_recved(conn_fd, buf, (size_t)ret);
            if (ret == 0)
            {
                CloseFd(conn_fd);
                return;
            }

            ready_read_fds.insert(conn_fd);
            return;
        }
    };

    return true;
}

void CloseConn(int fd)
{
    CloseFd(fd);
}

void WriteConn(int fd, const char *buf, size_t len)
{
    WriteBuf *wb = nullptr;
    auto iter = write_bufs.find(fd);
    if (iter == write_bufs.end())
    {
        //write directly first
        while (len > 0)
        {
            ssize_t ret = write(fd, buf, len);
            if (ret == -1)
            {
                if (errno != EAGAIN)
                {
                    Log(Sprintf("write failed: %m"));
                    CloseConn(fd);
                    return;
                }
                //new buf
                wb = &write_bufs[fd];
                break;
            }
            buf += ret;
            len -= (size_t)ret;
        }
        if (len == 0)
        {
            //success
            return;
        }
    }
    else
    {
        wb = &iter->second;
    }
    wb->AddData(buf, len);
}

void Init()
{
    ep_fd = epoll_create1(0);
    if (ep_fd == -1)
    {
        Die(Sprintf("create epoll fd failed: %m"));
    }
}

void Start()
{
    for (;;)
    {
        static const int kEpollEvCountMax = 1024;
        struct epoll_event evs[kEpollEvCountMax];
        int ev_count = epoll_wait(ep_fd, evs, kEpollEvCountMax, ready_read_fds.empty() ? 100 : 0);
        if (ev_count == -1)
        {
            if (errno != EINTR)
            {
                Die(Sprintf("epoll_wait failed: %m"));
            }
            ev_count = 0;
        }
        if (ev_count == 0 && !ready_read_fds.empty())
        {
            std::set<int> rrfs(std::move(ready_read_fds));
            ready_read_fds.clear();
            for (auto fd : rrfs)
            {
                auto h = read_handlers[fd];
                h(fd);
            }
        }
        for (int i = 0; i < ev_count; ++ i)
        {
            const struct epoll_event &ev = evs[i];
            int fd = ev.data.fd;

            if (ev.events & (EPOLLIN | EPOLLERR | EPOLLHUP))
            {
                //read
                auto h = read_handlers[fd]; //must exists
                h(fd);
            }
            else if (ev.events & (EPOLLOUT | EPOLLERR | EPOLLHUP))
            {
                //write
                auto iter = write_bufs.find(fd);
                if (iter != write_bufs.end())
                {
                    auto &wb = iter->second;
                    if (!wb.DoWrite(fd))
                    {
                        CloseFd(fd);
                    }
                    else if (wb.Empty())
                    {
                        write_bufs.erase(iter);
                    }
                }
            }
        }
    }
}

}

}
