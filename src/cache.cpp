#include "cache.h"
#include "numa_node.h"

Cache::Cache(int id, int index_len, int ways, int offset_len, Protocol protocol)
    : cache_id_(id),
      index_len_(index_len),
      set_size_(1 << index_len),
      ways_(ways),
      offset_len_(offset_len),
      line_size_(1 << offset_len),
      protocol_(protocol)
{
    for (int i = 0; i < set_size_; i++)
        sets_.push_back(std::make_shared<Set>(ways, protocol_));
};

void Cache::assignToNode(NUMANode *node)
{
    numa_node_ = node;
};

int Cache::getID() const
{
    return cache_id_;
};

std::pair<size_t, size_t> Cache::splitAddr(size_t addr)
{
    long tag_size = ADDR_LEN - (index_len_ + offset_len_);
    size_t set_mask = ((1L << index_len_) - 1L) << offset_len_;
    size_t tag_mask = ((1L << tag_size) - 1L) << (offset_len_ + index_len_);

    size_t tag = (addr & tag_mask) >> (offset_len_ + index_len_);
    size_t index = (addr & set_mask) >> offset_len_;

    return {tag, index};
};

CacheBlock *Cache::findInSet(size_t tag, size_t index)
{
    CacheBlock *target = nullptr;
    std::shared_ptr<Set> set = sets_[index];

    // run the entire loop here so we can increment last_used_ for all blocks
    for (CacheBlock *block : set->blocks_)
    {
        block->incrLruCnt();
        if (block->isValid() && block->getTag() == tag)
            target = block;
    }
    return target;
};

CacheBlock *Cache::findInCache(size_t addr)
{
    std::pair<size_t, size_t> pair = splitAddr(addr);
    size_t tag = pair.first;
    size_t index = pair.second;

    CacheBlock *block = findInSet(tag, index);
    return block;
}

void Cache::receiveMsg(size_t addr, DirectoryMsg msg, int request_node_id)
{
    CacheBlock *block = findInCache(addr);
    switch (msg)
    {
    case DirectoryMsg::READDATA_EX:
        block->receiveReadData(true);
        break;
    case DirectoryMsg::READDATA:
        block->receiveReadData(false);
        break;
    case DirectoryMsg::WRITEDATA:
        block->receiveWriteData();
        break;
    case DirectoryMsg::FETCH:
        block->fetch();
        numa_node_->emitCacheMsg(cache_id_, {addr, request_node_id}, CacheMsg::DATA, block->isDirty());
        break;
    case DirectoryMsg::INVALIDATE:
        block->invalidate();
        break;
    }
}

void Cache::cacheWrite(Addr addr)
{
    return performOperation(addr, true);
};

void Cache::cacheRead(Addr addr)
{
    return performOperation(addr, false);
};

void Cache::performOperation(Addr addr, bool is_write)
{
    std::pair<size_t, size_t> pair = splitAddr(addr.addr);
    size_t tag = pair.first;
    size_t index = pair.second;

    CacheBlock *block = findInSet(tag, index);

    if (block != nullptr)
        numa_node_->emitCacheMsg(
            cache_id_,
            addr,
            is_write ? block->writeBlock(addr.node_id)
                     : block->readBlock(addr.node_id));
    else
        evictAndReplace(tag, index, addr, is_write);
};

void Cache::evictAndReplace(size_t tag, size_t index, Addr addr, bool is_write)
{
    std::shared_ptr<Set> set = sets_[index];
    std::vector<CacheBlock *>::iterator evict_block = set->blocks_.begin();
    for (auto it = set->blocks_.begin(); it != set->blocks_.end(); ++it)
    {
        if (!(*it)->isValid())
        {
            evict_block = it;
            break;
        }
        else if ((*it)->getLruCnt() >= (*evict_block)->getLruCnt())
        {
            evict_block = it;
        }
    }

    if ((*evict_block)->isValid())
    {
        size_t old_tag = (*evict_block)->getTag() << (index_len_ + offset_len_);
        size_t set_mask = ((1 << index_len_) - 1) << offset_len_;
        numa_node_->emitCacheMsg(cache_id_, {old_tag | (addr.addr & set_mask), (*evict_block)->getNodeID()}, CacheMsg::EVICTION);
    }
    CacheMsg msg = (*evict_block)->evictAndReplace(is_write, tag, addr.node_id);
    numa_node_->emitCacheMsg(cache_id_, addr, msg);
};

void Cache::printConfig() const
{
    std::cout << "set size:\t" << set_size_ << "\n"
              << "associativity:\t" << ways_ << "\n"
              << "line size:\t" << line_size_ << "\n\n";
}

CacheStats Cache::getStats() const
{
    CacheStats stats;
    for (auto set : sets_)
    {
        for (CacheBlock *block : set->blocks_)
        {
            stats.hits_ += block->getHitCnt();
            stats.misses_ += block->getMissCnt();
            stats.flushes_ += block->getFlushCnt();
            stats.invalidations_ += block->getInvalidationCnt();
            stats.evictions_ += block->getEvictionCnt();
            stats.dirty_evictions_ += block->getDirtyEvictionCnt();
        }
    }
    stats.memory_writes_ = stats.dirty_evictions_ + stats.flushes_;
    return stats;
}

void Cache::printState() const
{
    auto stats = getStats();
    std::cout << "\n*** Cache " << cache_id_ << " ***\n"
              << "Hits:\t\t\t" << stats.hits_ << "\n"
              << "Misses:\t\t\t" << stats.misses_ << "\n"
              << "Flushes:\t\t" << stats.flushes_ << "\n"
              << "Evictions:\t\t" << stats.evictions_ << "\n"
              << "Dirty Evictions:\t" << stats.dirty_evictions_ << "\n"
              << "Invalidations:\t\t" << stats.invalidations_ << "\n"
              << "\n\n";
}