#pragma once
#include "cache_block.h"

class VIBlock : public CacheBlock
{
private:
    Message updateState(bool is_write) override;
    bool is_valid_ = false;

public:
    VIBlock();
    virtual ~VIBlock() {}
    virtual bool isValid() override;
    virtual Message writeBlock(int numa_node) override;
    virtual Message readBlock(int numa_node) override;

    virtual Message evictAndReplace(bool is_write, size_t tag, int new_node) override;

    virtual void invalidate() override;
    virtual void fetch() override;
    virtual void receiveReadData(bool exclusive) override;
    virtual void receiveWriteData() override;
};