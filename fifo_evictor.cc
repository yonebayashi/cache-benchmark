#include <queue>

#include "evictor.hh"
#include "fifo_evictor.hh"


FifoEvictor::FifoEvictor() {}

FifoEvictor::~FifoEvictor() {
	while (!key_queue.empty()) {
		key_queue.pop();
	}
}

void FifoEvictor::touch_key(const key_type& key) {
	key_queue.push(key);
	return;
}

const key_type FifoEvictor::evict()  {
  if (key_queue.empty()) {
    return "";
  }
  key_type evicted = key_queue.front();
  key_queue.pop();
  return evicted;
}
