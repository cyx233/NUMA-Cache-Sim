#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "cache_block.h"
#include "moesi_block.h"
#include "vi_block.h"

const int ADDR_LEN = 64;

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
                break;
            default:
                blocks_.push_back(new VIBlock);
                break;
            }
        }
    };
    std::vector<CacheBlock *> blocks_;
};

struct CacheStats
{
    size_t hits_ = 0, misses_ = 0, flushes_ = 0, invalidations_ = 0, evictions_ = 0,
           dirty_evictions_ = 0, memory_writes_ = 0;
};

class NUMANode;

class Cache
{
public:
    // 2^index_len sets, 2^offset_len bytes per block and ways
    Cache(int id, int numa_node, int index_len, int ways, int offset_len, Protocol protocol);

    void cacheWrite(Addr addr);
    void cacheRead(Addr addr);

    void assignToNode(NUMANode *node);

    void receiveInvalidate(size_t addr);
    void receiveFetch(Addr addr);
    void receiveReadData(size_t addr, bool exclusive);
    void receiveWriteData(size_t addr);

    int getID() const;
    void printConfig() const;
    CacheStats getStats() const;
    void printState() const;

private:
    void performOperation(Addr address, bool is_write);

    CacheBlock *readCache(size_t addr);
    std::pair<size_t, size_t> splitAddr(size_t addr); // tag & set index
    CacheBlock *findInSet(size_t tag, size_t index);

    void evictAndReplace(size_t tag, size_t index, Addr addr, bool is_write);
    void performMessage(Message msg, Addr addr);
    void sendEviction(size_t tag, size_t addr, int numa_node);

    int cache_id_;
    int index_len_;
    int set_size_;
    int ways_;
    int offset_len_;
    int line_size_;
    Protocol protocol_;

    NUMANode *numa_node_;
    std::vector<std::shared_ptr<Set>> sets_;
};