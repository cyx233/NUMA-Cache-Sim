#include "msi_block.h"
#include "cassert"

MSIBlock::MSIBlock() : CacheBlock(),
                       state_(MSI::I){};

CacheMsg MSIBlock::updateState(bool is_write)
{
    switch (state_)
    {
    case MSI::M:
        hit_ += 1;
        return CacheMsg::NOP;
    case MSI::S:
        if (is_write)
        {
            miss_++;
            state_ = MSI::M;
            return CacheMsg::BUSRDX;
        }
        else
        {
            hit_++;
            return CacheMsg::NOP;
        }
    case MSI::I:
        miss_ += 1;
        state_ = is_write ? MSI::M : MSI::S;
        return is_write ? CacheMsg::BUSRDX : CacheMsg::BUSRD;
    }
    return CacheMsg::NOP;
};

bool MSIBlock::isValid() { return state_ != MSI::I; };
CacheMsg MSIBlock::writeBlock(int node_id)
{
    lru_cnt_ = 0;
    dirty_ = true;
    node_id_ = node_id;
    return updateState(true);
};
CacheMsg MSIBlock::readBlock(int node_id)
{
    lru_cnt_ = 0;
    node_id_ = node_id;
    return updateState(false);
};

CacheMsg MSIBlock::evictAndReplace(bool is_write, size_t tag, int new_node)
{
    if (state_ != MSI::I)
    {
        if (dirty_)
        {
            flushes_++;
            dirty_evictions_++;
        }
        evictions_++;
    }

    tag_ = tag;
    dirty_ = is_write;
    lru_cnt_ = 0;
    node_id_ = new_node;
    state_ = MSI::I;

    return updateState(is_write);
};

void MSIBlock::invalidate()
{
    state_ = MSI::I;
    invalidations_ += 1;
};
void MSIBlock::fetch()
{
    assert(state_ == MSI::M);
    state_ = MSI::S;
    flushes_ += 1;
};

void MSIBlock::receiveReadData([[maybe_unused]] bool exclusive)
{
    state_ = MSI::S;
};
void MSIBlock::receiveWriteData()
{
    dirty_ = true;
    state_ = MSI::M;
};