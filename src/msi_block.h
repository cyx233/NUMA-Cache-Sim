#pragma once
#include "cache_block.h"

enum class MSI
{
    M,
    S,
    I
};

class MSIBlock : public CacheBlock
{
private:
    CacheMsg updateState(bool is_write) override;
    MSI state_;

public:
    MSIBlock();
    virtual ~MSIBlock() {}
    virtual bool isValid() override;
    virtual CacheMsg writeBlock(int numa_node) override;
    virtual CacheMsg readBlock(int numa_node) override;

    virtual CacheMsg evictAndReplace(bool is_write, size_t tag, int new_node) override;

    virtual void invalidate() override;
    virtual void fetch() override;
    virtual void receiveReadData(bool exclusive) override;
    virtual void receiveWriteData() override;
};