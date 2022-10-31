#include "internal.h"

namespace paraldis
{

class RObj_Str : public RObj
{
    std::string s_;

public:

    RObj_Str(const std::string &s) : s_(s)
    {
    }

    const std::string &Str() const
    {
        return s_;
    }
};

void ProcCmdGet(const std::vector<std::string> &args, std::string &rsp)
{
    if (args.size() != 2)
    {
        rsp = "-ERR cmd arg count error\r\n";
        return;
    }

    RObj::Ptr robj = db::GetRObj(args.at(1));
    if (robj)
    {
        auto robj_s = dynamic_cast<RObj_Str *>(robj.get());
        if (robj_s)
        {
            rsp = Sprintf("$%zu\r\n", robj_s->Str().size());
            rsp.append(robj_s->Str());
            rsp.append("\r\n");
        }
        else
        {
            rsp = "-WRONGTYPE robj is not string for GET cmd\r\n";
        }
    }
    else
    {
        rsp = "$-1\r\n";
    }
}

void ProcCmdSet(const std::vector<std::string> &args, std::string &rsp)
{
    if (args.size() != 3)
    {
        rsp = "-ERR cmd arg count error\r\n";
        return;
    }

    db::SetRObj(args.at(1), RObj::Ptr(new RObj_Str(args.at(2))));
    rsp = "+OK\r\n";
}

}
