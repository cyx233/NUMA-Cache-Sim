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

size_t Directory::getAddr(size_t addr) { return addr & ~((1L << block_offset_bits_) - 1L); }

void Directory::assignToNode(NUMANode *node) { numa_node_ = node; }

void Directory::receiveMsg(int cache_id, size_t addr, CacheMsg msg_type, bool is_dirty)
{
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
  line->owner_ = new_owner;
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
  if (is_dirty)
    getLine(getAddr(addr))->owner_ = cache_id;
}

void Directory::receiveBroadcast(int cache_id, size_t address)
{
  assert(protocol_ == Protocol::MOESI);
  size_t addr = getAddr(address);
  DirectoryLine *line = getLine(addr);
  for (size_t i = 0; i < line->presence_.size(); ++i)
    if (line->presence_[i] && i != (size_t)cache_id)
      numa_node_->emitDirectoryMsg(i, addr, DirectoryMsg::READDATA);
}

void Directory::receiveEviction(int cache_id, size_t address)
{
  size_t addr = getAddr(address);
  DirectoryLine *line = getLine(addr);

  assert(line->presence_[cache_id]);
  line->presence_[cache_id] = 0;

  if (line->owner_ == cache_id)
    line->owner_ = -1;

  int num_sharers = 0;
  switch (line->state_)
  {
  case DirectoryState::SO:
    num_sharers = std::count(line->presence_.begin(), line->presence_.end(), true);
    if (num_sharers == 0)
      line->state_ = DirectoryState::U;
    break;
  case DirectoryState::EM:
    line->state_ = DirectoryState::U;
    break;
  default:
    assert(false);
  }
}

void Directory::receiveBusRd(int cache_id, size_t address)
{
  size_t addr = getAddr(address);
  DirectoryLine *line = getLine(addr);

  switch (line->state_)
  {
  case DirectoryState::U:
    memory_reads_++;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::READDATA_EX);
    line->presence_[cache_id] = true;
    line->owner_ = cache_id;
    line->state_ = protocol_ == Protocol::MSI ? DirectoryState::SO
                                              : DirectoryState::EM;
    break;
  case DirectoryState::SO:
    if (line->owner_ != -1)
      numa_node_->emitDirectoryMsg(line->owner_, addr, DirectoryMsg::FETCH, numa_node_->getID());
    else
      memory_reads_++;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::READDATA);
    line->presence_[cache_id] = true;
    break;
  case DirectoryState::EM:
    numa_node_->emitDirectoryMsg(line->owner_, addr, DirectoryMsg::FETCH, numa_node_->getID());

    line->presence_[cache_id] = true;
    numa_node_->emitDirectoryMsg(cache_id, addr, DirectoryMsg::READDATA);

    line->state_ = DirectoryState::SO;
    break;
  }
}
void Directory::receiveBusRdX(int cache_id, size_t address)
{
  size_t addr = getAddr(address);
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
