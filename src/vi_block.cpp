#include "vi_block.h"

VIBlock::VIBlock() : CacheBlock(){};

Message VIBlock::updateState(bool is_write)
{
    if (is_valid_)
    {
        hit_ += 1;
        return Message::NOP;
    }
    else
    {
        miss_ += 1;
        if (is_write)
            return Message::BUSRDX;
        else
            return Message::BUSRD;
    }
};

bool VIBlock::isValid() { return is_valid_; };
Message VIBlock::writeBlock(int numa_node)
{
    lru_cnt_ = 0;
    dirty_ = true;
    numa_node_ = numa_node;
    return updateState(true);
};
Message VIBlock::readBlock(int numa_node)
{
    lru_cnt_ = 0;
    numa_node_ = numa_node;
    return updateState(false);
};

Message VIBlock::evictAndReplace(bool is_write, size_t tag, int new_node)
{
    if (dirty_)
    {
        flushes_++;
        dirty_evictions_++;
    }
    evictions_++;

    tag_ = tag;
    dirty_ = is_write;
    lru_cnt_ = 0;
    numa_node_ = new_node;
    is_valid_ = false;

    return updateState(is_write);
};

void VIBlock::invalidate()
{
    invalidations_ += 1;
    is_valid_ = false;
};
void VIBlock::fetch()
{
    flushes_ += 1;
    is_valid_ = false;
};
void VIBlock::receiveReadData([[maybe_unused]] bool exclusive) { is_valid_ = true; };
void VIBlock::receiveWriteData() { is_valid_ = true; };