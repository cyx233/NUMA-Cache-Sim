#pragma once

#include <cassert>
#include <unordered_map>
#include <vector>
#include "numa_node.h"
#include "protocols.h"

enum class DirectoryState
{
    U = 0, // uncached - no caches have valid copy
    S = 1, // shared - at least one cache has data
    EM = 2 // exclusive / modified - one cache owns it
};

struct DirectoryLine
{
    DirectoryLine(int procs) : state_(DirectoryState::U), owner_(-1)
    {
        presence_.resize(procs, false);
    }
    DirectoryState state_;
    std::vector<bool> presence_;
    int owner_;
};

class Directory
{
public:
    Directory(int procs, int b, Protocol protocol)
        : procs_(procs),
          block_offset_bits_(b),
          protocol_(protocol),
          memory_reads_(0) {}
    ~Directory()
    {
        for (auto &[addr, line] : directory_)
            delete line;
    }

    void assignToNode(NUMANode *interface);
    void cacheRead(int proc, size_t addr, int numa_node);
    void cacheWrite(int proc, size_t addr, int numa_node);

    // cache -> directory messages
    void receiveBusRd(int cache_id, size_t addr);
    void receiveBusRdX(int cache_id, size_t addr);
    void receiveEviction(int cache_id, size_t addr);
    void receiveData(int cache_id, size_t addr, bool is_dirty);
    void receiveBroadcast(int cache_id, size_t addr);

    size_t getMemoryReads() const { return memory_reads_; }

private:
    long getAddr(size_t addr);
    DirectoryLine *getLine(size_t addr);
    int findOwnerEM(DirectoryLine *line);
    void invalidateSharers(DirectoryLine *line, int new_owner, size_t addr);

    int procs_;
    int block_offset_bits_;
    Protocol protocol_;
    size_t memory_reads_;
    std::unordered_map<size_t, DirectoryLine *> directory_;

    NUMANode *numa_node_;
};
