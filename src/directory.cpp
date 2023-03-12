#include "directory.h"
#include "numa_node.h"

DirectoryLine *Directory::getLine(size_t addr)
{
  auto it = directory_.find(addr);
  if (it == directory_.end())
  {
    DirectoryLine *line = new DirectoryLine(procs_);
    directory_.insert({addr, line});
    return line;
  }
  return it->second;
}

size_t Directory::getAddr(size_t address) { return address & ~((size_t)(1 << block_offset_bits_) - 1); }

void Directory::assignToNode(NUMANode *node) { numa_node_ = node; }

void Directory::receiveMsg(int cache_id, size_t address, CacheMsg msg_type, bool is_dirty)
{
  size_t addr = getAddr(address);
  switch (msg_type)
  {
  case CacheMsg::BUSRD:
    receiveBusRd(cache_id, addr);
    break;
  case CacheMsg::BUSRDX:
    receiveBusRdX(cache_id, addr);
    break;
  case CacheMsg::EVICTION:
    receiveEviction(cache_id, addr);
    break;
  case CacheMsg::DATA:
    receiveData(cache_id, addr, is_dirty);
    break;
  case CacheMsg::BROADCAST:
    receiveBroadcast(cache_id, addr);
    break;
  case CacheMsg::NOP:
    break;
  }
}

void Directory::invalidateSharers(DirectoryLine *line, int new_owner, size_t addr)
{
  assert(line->state_ == DirectoryState::SO);
  for (size_t i = 0; i < line->presence_.size(); ++i)
  {
    if (line->presence_[i] && (int)i != new_owner)
    {
      numa_node_->emitDirectoryMsg(i, addr, DirectoryMsg::INVALIDATE);
      line->presence_[i] = false;
    }
  }
}

void Directory::receiveData(int cache_id, size_t addr, bool is_dirty)
{
  DirectoryLine *line = getLine(addr);
  if (is_dirty)
    line->owner_ = cache_id;
  else if (line->owner_ == cache_id)
    line->owner_ = -1;
}

void Directory::receiveBroadcast(int cache_id, size_t addr)
{
  assert(protocol_ == Protocol::MOESI);
  DirectoryLine *line = getLine(addr);
  for (size_t i = 0; i < line->presence_.size(); ++i)
    if (line->presence_[i] && i != (size_t)cache_id)
      numa_node_->emitDirectoryMsg(i, addr, DirectoryMsg::READDATA);
  line->owner_ = cache_id;
}

void Directory::receiveEviction(int cache_id, size_t addr)
{
  DirectoryLine *line = getLine(addr);

  assert(line->presence_[cache_id]);
  line->presence_[cache_id] = false;

  if (line->owner_ == cache_id)
    line->owner_ = -1;

  switch (line->state_)
  {
  case DirectoryState::SO:
    if (std::count(line->presence_.begin(), line->presence_.end(), true) == 0)
      line->state_ = DirectoryState::U;
    break;
  case DirectoryState::EM:
    line->state_ = DirectoryState::U;
    break;
  default:
    assert(false);
  }
}

void Directory::receiveBusRd(int cache_id, size_t addr)
{
  DirectoryLine *line = getLine(addr);

  switch (line->state_)
  {
  case DirectoryState::U:
    memory_reads_++;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::READDATA_EX);
    line->owner_ = cache_id;
    line->state_ = protocol_ == Protocol::MSI ? DirectoryState::SO
                                              : DirectoryState::EM;
    break;
  case DirectoryState::SO:
    if (protocol_ == Protocol::MOESI && line->owner_ != -1)
      numa_node_->emitDirectoryMsg(line->owner_, addr, DirectoryMsg::FETCH, numa_node_->getID());
    else
      memory_reads_++;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::READDATA);
    break;
  case DirectoryState::EM:
    numa_node_->emitDirectoryMsg(line->owner_, addr, DirectoryMsg::FETCH, numa_node_->getID());
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::READDATA);
    line->state_ = DirectoryState::SO;
    break;
  }
  line->presence_[cache_id] = true;
}

void Directory::receiveBusRdX(int cache_id, size_t addr)
{
  DirectoryLine *line = getLine(addr);

  switch (line->state_)
  {
  case DirectoryState::U:
    memory_reads_++;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::WRITEDATA);
    break;
  case DirectoryState::SO:
    invalidateSharers(line, cache_id, addr);
    memory_reads_++;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::WRITEDATA);
    break;
  case DirectoryState::EM:
    int owner_id = line->owner_;
    numa_node_->emitDirectoryMsg(owner_id, addr, DirectoryMsg::FETCH, numa_node_->getID());
    numa_node_->emitDirectoryMsg(owner_id, addr, DirectoryMsg::INVALIDATE);
    line->presence_[owner_id] = false;

    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::WRITEDATA);
    break;
  }
  line->presence_[cache_id] = true;
  line->state_ = DirectoryState::EM;
  line->owner_ = cache_id;
}
