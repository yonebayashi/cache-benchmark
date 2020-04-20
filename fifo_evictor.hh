#include <queue>

#include "evictor.hh"

#pragma once

class FifoEvictor : public Evictor {
private:
  std::queue<key_type> key_queue;

public:
  FifoEvictor();
  ~FifoEvictor();

  // Disallow copies
  FifoEvictor(const FifoEvictor&) = delete;
  FifoEvictor& operator=(const FifoEvictor&) = delete;

  void touch_key(const key_type&);
  const key_type evict();

  void clear();
};
