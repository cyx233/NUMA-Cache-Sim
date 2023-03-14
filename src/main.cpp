#include <getopt.h>
#include <fstream>
#include <iostream>

#include "numa_node.h"
#include "latencies.h"

// returns the NUMA node proc is on
int procToNode(int proc, int num_procs, int numa_nodes) { return proc / (num_procs / numa_nodes); }

// connect all of the NUMA regions interconnects, so node1->interconnect_[i] ==
// node->interconnect_[i]
void setupInterconnects(std::vector<NUMANode *> &nodes)
{
  for (NUMANode *node : nodes)
  {
    for (NUMANode *node1 : nodes)
    {
      node1->connectWith(node, node->getID());
    }
  }
}

void printAggregateStats(std::vector<NUMANode *> &nodes, int total_events, bool skip0)
{
  NodeStats stats;
  for (NUMANode *node : nodes)
  {
    stats += node->getStats(skip0);
  }

  if (skip0)
  {
    std::cout << "\t** Aggregate Stats Without Processor 0 ***" << std::endl;
  }
  else
  {
    std::cout << "\t** Aggregate Stats ***" << std::endl;
  }

  std::cout << std::endl
            << "Total Reads/Writes:\t\t" << total_events << std::endl
            << std::endl;

  std::cout << "Caches" << std::endl
            << "------" << std::endl
            << "Total Hits:\t\t" << stats.hits_ << std::endl
            << "Total Misses:\t\t" << stats.misses_ << std::endl
            << "Total Flushes:\t\t" << stats.flushes_ << std::endl
            << "Total Evictions: \t" << stats.evictions_ << std::endl
            << "Total Dirty Evictions: \t" << stats.dirty_evictions_ << std::endl
            << "Total Invalidations: \t" << stats.invalidations_ << std::endl
            << std::endl;

  std::cout << "Interconnects" << std::endl
            << "-------------" << std::endl
            << "Total Local Events: \t" << stats.local_events_ << std::endl
            << "Total Global Events: \t" << stats.global_events_ << std::endl
            << std::endl;

  std::cout << "Memory" << std::endl
            << "------" << std::endl
            << "Total Memory Reads: \t" << stats.memory_reads_ << std::endl
            << std::endl;

  std::cout << "Latencies" << std::endl
            << "---------" << std::endl
            << "Cache Access Latency:\t\t" << outputLatency(stats.hits_ * CACHE_LATENCY) << "\n"
            << "Memory Read Latency:\t\t" << outputLatency(stats.memory_reads_ * MEMORY_LATENCY)
            << "\n"
            << "Memory Write Latency:\t\t" << outputLatency(stats.memory_writes_ * MEMORY_LATENCY)
            << "\n"
            << "Memory Access Latency:\t\t"
            << outputLatency((stats.memory_reads_ + stats.memory_writes_) * MEMORY_LATENCY) << "\n"
            << "Local Events Latency:\t"
            << outputLatency(stats.local_events_ * LOCAL_INTERCONNECT_LATENCY) << "\n"
            << "Global Events Latency:\t"
            << outputLatency(stats.local_events_ * GLOBAL_INTERCONNECT_LATENCY) << "\n"
            << std::endl
            << std::endl;
}

NUMANode *NewNumaNode(int num_procs, int num_nodes, int node_id, int index_len, int ways, int offset_len, Protocol protocol)
{
  int procs_per_node = num_procs / num_nodes;
  std::vector<Cache *> caches;
  for (int i = 0; i < procs_per_node; ++i)
  {
    int cache_id = procs_per_node * node_id + i;
    caches.push_back(new Cache(cache_id, index_len, ways, offset_len, protocol));
  }
  Directory *dir = new Directory(num_procs, offset_len, protocol);
  return new NUMANode(node_id, num_nodes, num_procs, dir, caches);
}

void runSimulation(int s, int E, int b, std::ifstream &trace, int procs, int numa_nodes, Protocol protocol, bool individual, bool aggregate, bool aggr_skip0)
{
  std::vector<NUMANode *> nodes;
  for (int i = 0; i < numa_nodes; ++i)
  {
    NUMANode *node = NewNumaNode(procs, numa_nodes, i, s, E, b, protocol);
    nodes.push_back(node);
  }

  setupInterconnects(nodes);
  int total_events = 0;
  int total_events_skip0 = 0;

  int proc;    // the requesting proc
  char rw;     // whether it is a read or write
  int node_id; // the node where addr resides
  size_t addr;

  while (trace >> proc >> rw >> std::hex >> addr >> std::dec >> node_id)
  {
    if (node_id >= numa_nodes or proc >= procs)
    {
      std::cout << "Invalid value of p or n for given trace\n";
      exit(1);
    }

    // get the NUMA node that the requesting proc belongs to
    int proc_node = procToNode(proc, procs, numa_nodes);
    if (rw == 'R')
    {
      nodes[proc_node]->cacheRead(proc, addr, node_id);
    }
    else
    {
      nodes[proc_node]->cacheWrite(proc, addr, node_id);
    }
    total_events++;
    if (proc != 0)
    {
      total_events_skip0++;
    }
  }

  if (aggregate)
  {
    printAggregateStats(nodes, total_events, false);
  }

  if (aggr_skip0)
  {
    printAggregateStats(nodes, total_events_skip0, true);
  }

  for (NUMANode *node : nodes)
  {
    if (individual)
    {
      node->printStats();
    }
    delete node;
  }
}

int main(int argc, char **argv)
{
  std::string usage;
  usage += "-t <tracefile>: name of the trace file\n";
  usage += "-p <processors>: number of processors\n";
  usage += "-n <numa nodes>: number of NUMA nodes\n";
  usage += "-m <MSI | MOESI>: the cache protocol to use, default is MOESI\n";
  usage += "-s <s>: cache index bits (sets = 2^s)\n";
  usage += "-E <E>: cache associativity\n";
  usage += "-b <b>: cache offset bits (line size = 2^b)\n";
  usage += "-a: display aggregate stats\n";
  usage += "-A: display aggregate stats without process 0\n";
  usage += "-i: display individual stats (i.e.per cache, per NUMA node)\n";
  usage += "-h: help\n";

  char opt;
  std::string filepath;
  std::string protocol;

  // default to Intel L1 cache
  int s = 6;
  int E = 8;
  int b = 6;
  int procs = 1;
  int numa_nodes = 1;
  bool aggregate = false;
  bool aggr_skip0 = false;
  bool individual = false;

  // parse command line options
  while ((opt = getopt(argc, argv, "hvaAis:E:b:t:p:n:m:")) != -1)
  {
    switch (opt)
    {
    case 'h':
      std::cout << usage;
      return 0;
    case 'a':
      aggregate = true;
      break;
    case 'A':
      aggr_skip0 = true;
      break;
    case 'i':
      individual = true;
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      filepath = std::string(optarg);
      break;
    case 'p':
      procs = atoi(optarg);
      break;
    case 'n':
      numa_nodes = atoi(optarg);
      break;
    case 'm':
      protocol = std::string(optarg);
      break;
    default:
      std::cerr << usage;
      return 1;
    }
  }

  // -t option is required
  if (filepath == "")
  {
    std::cerr << "No trace file given\n";
    return 1;
  }

  Protocol prot;
  if (protocol == "" || protocol == "MOESI")
  {
    // default to MOESI
    prot = Protocol::MOESI;
  }
  else if (protocol == "MSI")
  {
    prot = Protocol::MSI;
  }
  else
  {
    return 1;
  }

  std::ifstream trace(filepath);
  if (!trace.is_open())
  {
    std::cerr << "Invalid trace file\n";
    return 1;
  }

  // run the input trace on the cache
  runSimulation(s, E, b, trace, procs, numa_nodes, prot, individual, aggregate, aggr_skip0);

  trace.close();

  return 0;
}
