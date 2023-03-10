#pragma once
#include <vector>
#include <stddef.h>

class Cache;
class Directory;

struct Addr;
enum class CacheMsg;
enum class DirectoryMsg;

// NUMA Node = Directory*1 + Processor(Cache)*N
class NUMANode
{
public:
    NUMANode(int node_id_, int num_numa_nodes, int num_procs, Directory *directory, std::vector<Cache> *caches);
    ~NUMANode();
    void connectWith(NUMANode *node, int id);

    // operations
    void cacheRead(int proc, size_t addr, int numa_node);
    void cacheWrite(int proc, size_t addr, int numa_node);

    // cache -> directory messages
    void emitCacheMsg(int src, Addr addr, CacheMsg msg_type, bool is_dirty = false);

    // directory -> cache messages
    void emitDirectoryMsg(int dst, size_t addr, DirectoryMsg msg, int request_node_id = -1);

    size_t getLocalEvents() { return directory_events_ + cache_events_; }
    size_t getGlobalEvents() { return global_events_; }

    int getID();

private:
    int getNode(int dest);

    int node_id_;
    int num_numa_nodes_;
    int num_procs_;

    Directory *directory_;
    std::vector<Cache> *caches_;

    std::vector<NUMANode *> interconnects_;

    // metrics
    unsigned long cache_events_;
    unsigned long directory_events_;
    unsigned long global_events_;
};
