#include "moesi_block.h"
MOESIBlock::MOESIBlock() : CacheBlock(), state_(MOESI::I) {}
bool MOESIBlock::isValid() { return state_ != MOESI::I; }

CacheMsg MOESIBlock::updateState(bool is_write)
{
    switch (state_)
    {
    case MOESI::M:
        hit_ += 1;
        return CacheMsg::NOP;
    case MOESI::O:
        hit_ += 1;
        if (is_write)
            return CacheMsg::BROADCAST;
        else
            return CacheMsg::NOP;
    case MOESI::E:
        hit_ += 1;
        if (is_write)
            state_ = MOESI::M;
        return CacheMsg::NOP;
    case MOESI::S:
        if (is_write)
        {
            miss_ += 1;
            state_ = MOESI::M;
            return CacheMsg::BUSRDX;
        }
        else
        {
            hit_ += 1;
            return CacheMsg::NOP;
        }
    case MOESI::I:
        miss_ += 1;
        state_ = is_write ? MOESI::M : MOESI::E;
        return is_write ? CacheMsg::BUSRDX : CacheMsg::BUSRD;
    }
    return CacheMsg::NOP;
}

CacheMsg MOESIBlock::writeBlock(int node_id)
{
    lru_cnt_ = 0;
    dirty_ = true;
    node_id_ = node_id;
    return updateState(true);
}
CacheMsg MOESIBlock::readBlock(int node_id)
{
    lru_cnt_ = 0;
    node_id_ = node_id;
    return updateState(false);
}

CacheMsg MOESIBlock::evictAndReplace(bool is_write, size_t tag, int new_node)
{
    if (state_ != MOESI::I)
    {
        if (dirty_)
        {
            flushes_ += 1;
            dirty_evictions_ += 1;
        }
        evictions_ += 1;
    }
    dirty_ = is_write;
    tag_ = tag;
    node_id_ = new_node;
    state_ = MOESI::I;
    return updateState(is_write);
}

void MOESIBlock::invalidate()
{
    invalidations_ += 1;
    state_ = MOESI::I;
}
void MOESIBlock::fetch()
{
    assert(state_ == MOESI::O || state_ == MOESI::E || state_ == MOESI::M);
    if (state_ == MOESI::M)
    {
        state_ = MOESI::O;
    }
    else if (state_ == MOESI::E)
    {
        state_ = MOESI::S;
    }
};
void MOESIBlock::receiveReadData(bool exclusive) { state_ = exclusive ? MOESI::E : MOESI::S; }
void MOESIBlock::receiveWriteData() { state_ = MOESI::M; };