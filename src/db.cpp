#include "internal.h"

namespace paraldis
{

namespace db
{

class Slot
{
    std::mutex lock_;

    std::map<std::string, RObj::Ptr> kvs_;

public:

    RObj::Ptr GetRObj(const std::string &k)
    {
        std::lock_guard<std::mutex> lg(lock_);
        auto iter = kvs_.find(k);
        return iter == kvs_.end() ? nullptr : iter->second;
    }

    void SetRObj(const std::string &k, RObj::Ptr robj)
    {
        std::lock_guard<std::mutex> lg(lock_);
        kvs_[k] = robj;
    }
};

static const size_t kSlotCount = 65537;

static Slot slots[kSlotCount];

static Slot &SlotByKey(const std::string &k)
{
    size_t h = k.size();
    for (char c : k)
    {
        h = (h + (unsigned char)c) * 1000003;
    }
    return slots[h % kSlotCount];
}

RObj::Ptr GetRObj(const std::string &k)
{
    return SlotByKey(k).GetRObj(k);
}

void SetRObj(const std::string &k, RObj::Ptr robj)
{
    SlotByKey(k).SetRObj(k, robj);
}

}

}
