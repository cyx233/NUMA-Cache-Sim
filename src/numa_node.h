#pragma once
#include "cache.h"

class Cache;
class Directory;
struct Addr;

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
    void receiveBusRd(int src, Addr address);
    void receiveBusRdX(int src, Addr address);
    void receiveData(int src, Addr address, bool is_dirty);
    void receiveEviction(int src, Addr address);
    void receiveBroadcast(int src, Addr address);

    // directory -> cache messages
    void sendReadData(int dest, long addr, bool exclusive);
    void sendWriteData(int dest, long addr);
    void sendFetch(int dest, Addr address);
    void sendInvalidate(int dest, long addr);

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
