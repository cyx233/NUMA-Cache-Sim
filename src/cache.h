#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "cache_block.h"
#include "moesi_block.h"

struct Addr
{
    size_t addr;
    uint32_t numa_node;
};

enum class Protocol
{
    MSI,
    MOESI
};

struct Set
{
    Set(int ways, Protocol proto)
    {
        for (int i = 0; i < ways; i++)
        {
            switch (proto)
            {
            case Protocol::MOESI:
                blocks_.push_back(new MOESIBlock);
                break;
            default:
                break;
            }
        }
    };
    std::vector<CacheBlock *> blocks_;
};

class Interconnect;

class Cache
{
public:
    // 2^index_len sets, 2^offset_len bytes per block and ways
    Cache(int id, int numa_node, int index_len, int ways, int offset_len, Protocol protocol);

    void cacheWrite(Addr addr);
    void cacheRead(Addr addr);

    void receiveMsg(Interconnect *interconnect);

    void receiveInvalidate(size_t addr);
    void receiveFetch(Addr addr);
    void receiveReadData(size_t addr, bool exclusive);
    void receiveWriteData(size_t addr);

    int getID() const;

private:
    void performOperation(Addr address, bool is_write);

    CacheBlock *readCache(size_t addr);
    std::pair<size_t, size_t> splitAddr(size_t addr); // tag & set
    CacheBlock *findInSet(size_t tag, size_t set);

    void evictAndReplace(size_t tag, size_t index, Addr addr, bool is_write);
    void performMessage(Message msg, Addr addr);
    void sendEviction(size_t tag, size_t addr, int numa_node);

    int cache_id_;
    int numa_node_;
    int index_len_;
    int totSets_;
    int ways_;
    int offset_len_;
    int line_size_;
    Protocol protocol_;

    Interconnect *interconnect_;
    std::vector<std::shared_ptr<Set>> sets_;
};