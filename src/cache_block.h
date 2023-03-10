#pragma once
#include <stddef.h>

enum class Message
{
    NOP,
    BUSRD,
    BUSRDX,
    BRODCAST
};

class CacheBlock
{
private:
    // flags
    bool dirty_;
    size_t tag_;
    size_t lru_cnt_;

    int numa_node_;

    // metrics
    size_t flushes_;
    size_t hit_;
    size_t miss_;
    size_t invalidations_;
    size_t evictions_;
    size_t dirty_evictions_;

    virtual Message updateState(bool is_write) = 0;

public:
    CacheBlock()
        : dirty_(false),
          tag_(0),
          lru_cnt_(0),
          flushes_(0),
          hit_(0),
          miss_(0),
          invalidations_(0),
          evictions_(0),
          dirty_evictions_(0) {}
    virtual ~CacheBlock(){};

    // Get metadata about the block
    virtual bool isValid() = 0;
    bool isDirty() const { return dirty_; }
    size_t getLruCnt() const { return lru_cnt_; }
    size_t getTag() const { return tag_; }
    void incrLruCnt() { lru_cnt_++; }
    int getNumaNode() { return numa_node_; }

    virtual Message writeBlock(int numa_node) = 0;
    virtual Message readBlock(int numa_node) = 0;
    virtual Message evictAndReplace(bool is_write, size_t tag, int new_node) = 0;

    virtual void invalidate() = 0;
    virtual void fetch() = 0; // fetch a line, send it to main memory if dirty_ == true
    virtual void receiveReadData(bool exclusive) = 0;
    virtual void receiveWriteData() = 0;

    // Get stats on the block
    size_t getHitCnt() { return hit_; }
    size_t getMissCnt() { return miss_; }
    size_t getFlushCnt() { return flushes_; }
    size_t getEvictionCnt() { return evictions_; }
    size_t getInvalidationCnt() { return invalidations_; }
    size_t getDirtyEvictionCnt() { return dirty_evictions_; }
};