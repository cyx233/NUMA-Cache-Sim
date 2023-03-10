#include "cache.h"

Cache::Cache(int id, int numa_node, int index_len, int ways, int offset_len, Protocol protocol)
    : cache_id_(id),
      index_len_(index_len),
      set_size_(1 << index_len),
      ways_(ways),
      offset_len_(offset_len),
      line_size_(1 << offset_len)
{
    for (int i = 0; i < set_size_; i++)
        sets_.push_back(std::make_shared<Set>(ways, protocol_));
};

void Cache::cacheWrite(Addr addr)
{
    return performOperation(addr, true);
};

void Cache::cacheRead(Addr addr)
{
    return performOperation(addr, false);
};

void Cache::assignToNode(NUMANode *node)
{
    numa_node_ = node;
};

void Cache::receiveInvalidate(size_t addr){};
void Cache::receiveFetch(Addr addr){};
void Cache::receiveReadData(size_t addr, bool exclusive){};
void Cache::receiveWriteData(size_t addr){};

int Cache::getID() const
{
    return cache_id_;
};

void Cache::performOperation(Addr addr, bool is_write)
{
    std::pair<size_t, size_t> pair = splitAddr(addr.addr);
    size_t tag = pair.first;
    size_t index = pair.second;

    CacheBlock *block = findInSet(tag, index);

    if (block != nullptr)
        return;
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
    for (auto block : set->blocks_)
    {
        block->incrLruCnt();
        if (block->isValid() && block->getTag() == tag)
            target = block;
    }
    return target;
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
        sendEviction((*evict_block)->getTag(), addr.addr, (*evict_block)->getNumaNode());

    Message msg = (*evict_block)->evictAndReplace(is_write, tag, addr.numa_node);
    performMessage(msg, addr);
};
void Cache::performMessage(Message msg, Addr addr)
{
    if (numa_node_ == nullptr)
        return;
    switch (msg)
    {
    case Message::BUSRD:
        break;
    case Message::BUSRDX:
        break;
    case Message::BROADCAST:
        break;
    case Message::NOP:
        break;
    default:
        break;
    }
};
void Cache::sendEviction(size_t tag, size_t addr, int numa_node){};

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