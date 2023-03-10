#include "vi_block.h"

VIBlock::VIBlock() : CacheBlock(){};

CacheMsg VIBlock::updateState(bool is_write)
{
    if (is_valid_)
    {
        hit_ += 1;
        return CacheMsg::NOP;
    }
    else
    {
        miss_ += 1;
        if (is_write)
            return CacheMsg::BUSRDX;
        else
            return CacheMsg::BUSRD;
    }
};

bool VIBlock::isValid() { return is_valid_; };
CacheMsg VIBlock::writeBlock(int node_id)
{
    lru_cnt_ = 0;
    dirty_ = true;
    node_id_ = node_id;
    return updateState(true);
};
CacheMsg VIBlock::readBlock(int node_id)
{
    lru_cnt_ = 0;
    node_id_ = node_id;
    return updateState(false);
};

CacheMsg VIBlock::evictAndReplace(bool is_write, size_t tag, int new_node)
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
    node_id_ = new_node;
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