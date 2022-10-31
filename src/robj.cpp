#include "internal.h"

namespace paraldis
{

RObj::~RObj()
{
}

void ProcCmd(const std::vector<std::string> &args, std::string &rsp)
{
    if (args.at(0) == "GET")
    {
        return ProcCmdGet(args, rsp);
    }
    if (args.at(0) == "SET")
    {
        return ProcCmdSet(args, rsp);
    }

    rsp = "-ERR unknown cmd\r\n";
}

}
