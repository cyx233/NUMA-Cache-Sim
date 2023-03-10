#include "directory.h"

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

void Directory::invalidateSharers(DirectoryLine *line, int new_owner, size_t addr)
{
  assert(line->state_ == DirectoryState::SO);
  line->owner_ = new_owner;
  for (size_t i = 0; i < line->presence_.size(); ++i)
  {
    if (line->presence_[i] && i != new_owner)
    {
      numa_node_->sendInvalidate(i, addr);
      line->presence_[i] = false;
    }
  }
}

void Directory::receiveData(int cache_id, size_t addr, bool is_dirty)
{
  if (is_dirty)
    getLine(getAddr(addr))->owner_ = cache_id;
}

void Directory::receiveBroadcast(int cache_id, size_t addr)
{
  assert(protocol_ == Protocol::MOESI);
  size_t addr = getAddr(addr);
  DirectoryLine *line = getLine(addr);
  for (size_t i = 0; i < line->presence_.size(); ++i)
    if (line->presence_[i] && i != (size_t)cache_id)
      numa_node_->sendReadData(i, addr, false);
}

void Directory::receiveEviction(int cache_id, size_t addr)
{
  size_t addr = getAddr(addr);
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

void Directory::receiveBusRd(int cache_id, size_t addr)
{
  size_t addr = getAddr(addr);
  DirectoryLine *line = getLine(addr);

  switch (line->state_)
  {
  case DirectoryState::U:
    memory_reads_++;
    numa_node_->sendReadData(cache_id, addr, true);
    line->presence_[cache_id] = true;
    line->owner_ = cache_id;
    line->state_ = protocol_ == Protocol::MSI ? DirectoryState::SO
                                              : DirectoryState::EM;
    break;
  case DirectoryState::SO:
    if (line->owner_ != -1)
      numa_node_->sendFetch(line->owner_, {addr, numa_node_->getID()});
    else
      memory_reads_++;
    numa_node_->sendReadData(cache_id, addr, false);
    line->presence_[cache_id] = true;
    break;
  case DirectoryState::EM:
    // downgrade the owner and get him to flush or transition to O, now in shared state
    numa_node_->sendFetch(line->owner_, {addr, numa_node_->getID()});

    line->presence_[cache_id] = true;
    numa_node_->sendReadData(cache_id, addr, false);

    line->state_ = DirectoryState::SO;
    break;
  }
}
void Directory::receiveBusRdX(int cache_id, size_t addr)
{
  size_t addr = getAddr(addr);
  DirectoryLine *line = getLine(addr);

  switch (line->state_)
  {
  case DirectoryState::U:
    memory_reads_++;
    numa_node_->sendWriteData(cache_id, addr);
    break;
  case DirectoryState::SO:
    invalidateSharers(line, cache_id, addr);
    memory_reads_++;
    numa_node_->sendWriteData(cache_id, addr);
    break;
  case DirectoryState::EM:
    int owner_id = line->owner_;
    numa_node_->sendFetch(owner_id, {addr, numa_node_->getID()});
    numa_node_->sendInvalidate(owner_id, addr);

    line->presence_[owner_id] = false;

    numa_node_->sendWriteData(cache_id, addr);
    break;
  }
  line->presence_[cache_id] = true;
  line->state_ = DirectoryState::EM;
  line->owner_ = cache_id;
}
