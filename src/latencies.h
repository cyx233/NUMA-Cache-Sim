#pragma once
#include <chrono>
#include <string>
// times are given in nanoseconds

static const int NUMA_DISTANCE = 2;

// L1 cache hit time
static const int CACHE_LATENCY = 1;

// main memory read/write time
static const int MEMORY_LATENCY = 100;

// directory -> cache and cache->directory latency
static const int LOCAL_INTERCONNECT_LATENCY = 1;

// NUMA node -> NUMA node latency
static constexpr int GLOBAL_INTERCONNECT_LATENCY = LOCAL_INTERCONNECT_LATENCY * NUMA_DISTANCE;

std::string outputLatency(size_t x);