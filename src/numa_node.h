#pragma once
#include <vector>
#include <stddef.h>
#include "directory.h"

struct Addr;
enum class CacheMsg;
enum class DirectoryMsg;

struct NodeStats
{
    size_t hits_ = 0, misses_ = 0, flushes_ = 0, evictions_ = 0, dirty_evictions_ = 0,
           invalidations_ = 0, local_events_ = 0, global_events_ = 0, memory_reads_ = 0,
           memory_writes_ = 0;
    NodeStats &operator+=(const NodeStats &other)
    {
        hits_ += other.hits_;
        misses_ += other.misses_;
        flushes_ += other.flushes_;
        evictions_ += other.evictions_;
        dirty_evictions_ += other.dirty_evictions_;
        invalidations_ += other.invalidations_;
        local_events_ += other.local_events_;
        global_events_ += other.global_events_;
        memory_reads_ += other.memory_reads_;
        memory_writes_ += other.memory_writes_;
        return *this;
    }

    NodeStats &operator+=(const CacheStats &other)
    {
        hits_ += other.hits_;
        misses_ += other.misses_;
        flushes_ += other.flushes_;
        evictions_ += other.evictions_;
        dirty_evictions_ += other.dirty_evictions_;
        invalidations_ += other.invalidations_;
        memory_writes_ += other.memory_writes_;
        return *this;
    }
};

// NUMA Node = Directory*1 + Processor(Cache)*N
class NUMANode
{
public:
    NUMANode(int node_id_, int num_numa_nodes, int num_procs, Directory *directory, std::vector<Cache *> caches);
    ~NUMANode();
    void connectWith(NUMANode *node, int id);

    // operations
    void cacheRead(int proc, size_t addr, int numa_node);
    void cacheWrite(int proc, size_t addr, int numa_node);

    // cache -> directory messages
    void emitCacheMsg(int src, Addr addr, CacheMsg msg_type, bool is_dirty = false);

    // directory -> cache messages
    void emitDirectoryMsg(int dst, size_t addr, DirectoryMsg msg, int request_node_id = -1);

    size_t getLocalEvents() const { return directory_events_ + cache_events_; }
    size_t getGlobalEvents() const { return global_events_; }

    int getID();
    NodeStats getStats() const;
    void printStats() const;

private:
    int getNode(int dest);

    int node_id_;
    int num_numa_nodes_;
    int num_procs_;
    int procs_per_node_;

    Directory *directory_;
    std::vector<Cache *> caches_;

    std::vector<NUMANode *> interconnects_;

    // metrics
    unsigned long cache_events_;
    unsigned long directory_events_;
    unsigned long global_events_;
};
