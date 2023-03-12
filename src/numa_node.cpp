#include "numa_node.h"
#include "directory.h"

NUMANode::NUMANode(int node_id, int num_numa_nodes, int num_procs, Directory *directory, std::vector<Cache *> caches)
    : node_id_(node_id),
      num_numa_nodes_(num_numa_nodes),
      num_procs_(num_procs),
      procs_per_node_(num_procs_ / num_numa_nodes_),
      directory_(directory),
      caches_(caches),
      cache_events_(0L),
      directory_events_(0L),
      global_events_(0L)
{
    interconnects_.resize(num_numa_nodes_);
    directory_->assignToNode(this);
    for (Cache *cache : caches_)
        cache->assignToNode(this);
}
NUMANode::~NUMANode()
{
    delete directory_;
    for (Cache *cache : caches_)
    {
        delete cache;
    }
}

int NUMANode::getID() { return node_id_; }

void NUMANode::connectWith(NUMANode *node, int id)
{
    interconnects_[id] = node;
}

int NUMANode::getNode(int dest) { return dest / (num_procs_ / num_numa_nodes_); }

NodeStats NUMANode::getStats() const
{
    NodeStats stats;
    for (Cache *cache : caches_)
        stats += cache->getStats();
    stats.memory_reads_ = directory_->getMemoryReads();
    stats.local_events_ = getLocalEvents();
    stats.global_events_ = getGlobalEvents();
    return stats;
}

void NUMANode::printStats() const
{
    std::cout << "\tNUMA NODE: " << node_id_ << "\n"
              << "\t-----------";
    for (Cache *cache : caches_)
        cache->printState();
    std::cout << "*** Interconnect Events ***\n"
              << "Cache Events:\t\t\t" << cache_events_ << "\n"
              << "Directory Events:\t\t" << directory_events_ << "\n"
              << "Global Interconnect Events:\t" << global_events_ << "\n";
    std::cout << std::endl;
    std::cout << "*** Memory Reads ***\n";
    std::cout << "Memory Reads:\t\t" << directory_->getMemoryReads() << "\n";
    std::cout << std::endl;
}

void NUMANode::cacheRead(int proc, unsigned long addr, int numa_node)
{
    caches_[proc % procs_per_node_]->cacheRead({addr, numa_node});
}

void NUMANode::cacheWrite(int proc, unsigned long addr, int numa_node)
{
    caches_[proc % procs_per_node_]->cacheWrite({addr, numa_node});
}

void NUMANode::emitCacheMsg(int src, Addr addr, CacheMsg msg_type, bool is_dirty)
{
    cache_events_++;
    if (addr.node_id != node_id_)
    {
        global_events_++;
        interconnects_[addr.node_id]->emitCacheMsg(src, addr, msg_type, is_dirty);
    }
    else
        directory_->receiveMsg(src, addr.addr, msg_type, is_dirty);
}

void NUMANode::emitDirectoryMsg(int dst, size_t addr, DirectoryMsg msg, int request_node_id)
{
    directory_events_++;
    int dst_node;
    if ((dst_node = getNode(dst)) != node_id_)
    {
        global_events_++;
        interconnects_[dst_node]->emitDirectoryMsg(dst, addr, msg, request_node_id);
    }
    else
        caches_[dst % (num_procs_ / num_numa_nodes_)]->receiveMsg(addr, msg, request_node_id);
}