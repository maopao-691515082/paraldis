#pragma once

#ifndef __GNUC__
#   error need GNUC
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <iostream>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <set>
#include <mutex>
#include <list>
#include <thread>
#include <memory>

#include <sys/time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/eventfd.h>

namespace paraldis
{

void Main(int argc, char *argv[]);

void InitWorkers(size_t worker_count);
void DeliverConnToWorkers(int conn_fd);
void StartSvr();
bool ParseInt64(const std::string &s, int64_t &v);

int64_t NowMS();
std::string Sprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void Log(const std::string &msg);
void Die(const std::string &msg);

namespace reactor
{

void Init();
void Start();

void WriteConn(int fd, const char *buf, size_t len);
void CloseConn(int fd);

bool RegListener(int fd, std::function<void (int)> on_new_conn);
bool RegEvFd(int fd, std::function<void (uint64_t)> on_ev);
bool RegConn(int fd, std::function<void (int, const char *, size_t)> on_data_recved);

}

class ReqParser
{
public:

    typedef std::shared_ptr<ReqParser> Ptr;

    virtual void Feed(const char *data, size_t len) = 0;
    virtual bool PopCmd(std::vector<std::string> &args) = 0;

    static Ptr New();
};

}
