#include <unordered_map>

#include "cache.hh"
#include "evictor.hh"
#include <cmath>
#include <iostream>
#include <cstring>


struct default_hash {
  Cache::size_type operator()(key_type const& key) const {
      const Cache::size_type p = 53;
      unsigned long long int result= 0;
      for (Cache::size_type i = 0; i < key.size(); i++) {
        result+=((key[i])*(unsigned long long int) std::pow(p, i));
      }
      return result;
  }
};


class Cache::Impl {
  public:
    size_type maxmem;
    float max_load_factor;
    Evictor* evictor;
    hash_func hasher;
    size_type memused;
    std::unordered_map<key_type, val_type, hash_func> m_cache;


    Impl(size_type maxmem,
        float max_load_factor,
        Evictor* evictor,
        hash_func hasher = default_hash()) :
                            maxmem(maxmem), max_load_factor(max_load_factor), evictor(evictor), hasher(hasher),
                            memused(0), m_cache(0, hasher)
      {
        m_cache.max_load_factor(max_load_factor);
      }

    ~Impl() {
      for (auto it=m_cache.begin(); it!= m_cache.end(); ++it) {
        del(it->first);
      }
      if (evictor!= nullptr) {
        delete evictor;
      }

    }

    void set(key_type key, val_type val, size_type size)
    {
      if (size>maxmem) {
        return;
      }
      if (m_cache.find(key)!= m_cache.end() ) {
        //If the key is already in the cache and we're updating the value,
        //we want to free the memory pointed to by the pointer previously corresponding
        //to key before losing track of its address.
        del(key);
      }
      if (memused + size >maxmem && evictor==nullptr) {
        return;
      }
      while (memused + size > maxmem) {
        auto key_to_evict = evictor->evict();
        del(key_to_evict);
      }

      byte_type* new_val = new byte_type[size];
      // C-Syle Copying: std::memcpy(new_val, val, size);
      std::copy(val, val + size, new_val);
      m_cache[key] = new_val;
      memused += size;
      if (evictor!= nullptr) {
        evictor->touch_key(key);
      }
      return;
    }

    val_type get(key_type key, size_type& val_size) const {
      auto item = m_cache.find(key);
      if (item == m_cache.end()) {
        val_size = 0;
        return nullptr;
      }
      val_size = strlen(item->second)+1;
      if (evictor!= nullptr) {
        evictor->touch_key(key);
      }
      return item->second;
    };

    bool del(key_type key) {
      size_type size =0;
      get(key, size);
      if (size>0) {
        delete[] m_cache[key];
        m_cache.erase(key);
        memused-= size;
        return true;
      } else {
        return false;
      }
    };



    size_type space_used() const {
      return memused;
    };

    void reset() {
      for (auto it=m_cache.begin(); it!= m_cache.end(); ++it) {
        del(it->first);
      }
      if (evictor!= nullptr) {
        while (evictor-> evict()!= "") {
          bool dummy = false;
          dummy +=1;
        }
      }
      memused = 0;
    };


};

Cache::Cache(size_type maxmem,
      float max_load_factor,
      Evictor* evictor,
      hash_func hasher) : pImpl_(new Impl(maxmem, max_load_factor, evictor, hasher)) {}
Cache::~Cache() {}

void Cache::set(key_type key, val_type val, size_type size)
{
  return pImpl_-> set(key, val, size);
}

Cache::val_type Cache::get(key_type key, size_type& val_size) const
{
  return pImpl_->get(key, val_size);
}

bool Cache::del(key_type key)
{
  return pImpl_->del(key);
}

Cache::size_type Cache::space_used() const
{
  return pImpl_->space_used();
}

void Cache::reset()
{
  return pImpl_->reset();
}
