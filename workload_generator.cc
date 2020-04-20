#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <thread>

#include "cache.hh"
#include "evictor.hh"

#define LOCALHOST_ADDRESS "127.0.0.1"
#define PORT "8080"

int MIN_KEY_SIZE = 1;
int MAX_KEY_SIZE = 250;
int MIN_VAL_SIZE = 1;
int MAX_VAL_SIZE = 10000;
int NUM_REQUESTS = 1000;
int WARMUP_STEPS = 100;


static const std::string LETTERS = "abcdefghijklmnopqrstuvwxyz";
int stringLength = sizeof(LETTERS) - 1;

char string_generator(const int& seed)
{
  std::default_random_engine gen(seed);
  std::uniform_int_distribution<int> dist(0, stringLength);
  return LETTERS[dist(gen)];
}


class Generator {
    std::default_random_engine generator;
    std::extreme_value_distribution<double> distribution;
    int min;
    int max;
public:
    Generator(const double& a, const double& b, const int& min, const int& max, const int& seed):
        generator(seed), distribution(a, b), min(min), max(max)  {}

    int operator ()() {
        while (true) {
            double number = this->distribution(generator);
            if (number >= this->min && number <= this->max)
                return static_cast<int>(number);
        }
    }
};



int workload_generator(const int& seed, std::string& key, std::string& val) {
  std::default_random_engine gen(seed);
  std::discrete_distribution<int> dist{68, 17, 15};
  auto req_type = dist(gen);

  Generator KeyGen(30.0, 8.0, MIN_KEY_SIZE, MAX_KEY_SIZE, seed);
  Generator ValGen(10.0, 50.0, MIN_VAL_SIZE, MAX_VAL_SIZE, seed);

  int key_size = KeyGen();
  for (int i = 0; i < key_size; i++) {
    key += string_generator(key_size + i);
  }

  int val_size = ValGen();
  for (int i = 0; i < val_size; i++) {
    val += string_generator(val_size + i);
  }

  return req_type;
}


void baseline_latencies(Cache& cache, Cache::size_type& size, double* measurements, const int& nreq) {
  typedef std::chrono::system_clock Time;

  for (int i = 0; i < nreq; i++) {
    key_type key;
    std::string val;
    auto req_type = workload_generator(i+1, key, val);
    Cache::val_type pval = val.c_str();

    auto t1 = Time::now();

    if (req_type == 0) {
      cache.get(key, size);
    } else if (req_type == 1) {
      cache.set(key, pval, strlen(pval)+1);
    } else {
      cache.del(key);
    }

    auto t2 = Time::now();
    auto t = std::chrono::duration<double, std::milli>(t2 - t1);
    measurements[i] = t.count();
  }
  return;
}

void baseline_performance(Cache& cache, Cache::size_type& size, double& p95, int& mean) {
  int nreq = NUM_REQUESTS;
  double* measurements = new double[nreq];

  baseline_latencies(cache, size, measurements, nreq);

  std::sort(measurements, measurements+nreq);
  int idx = std::round(.95 * nreq);
  p95 = measurements[idx];

  double sum = 0.0;
  double ms_mean = std::accumulate(measurements, measurements+nreq, sum) / (double) nreq;
  mean = std::lround(1 / (double)(ms_mean * 0.001));     // convert ratio milliseconds/request to requests/second

  delete[] measurements;
  return;
}


int main(int argc, char const *argv[]) {
  int num_requests = NUM_REQUESTS;
  if (argc == 2)
    num_requests = std::atoi(argv[1]);

  std::cout << "Number of requests: " << num_requests << '\n';


  Cache cache(LOCALHOST_ADDRESS, PORT);
  Cache::size_type size;

  // Warm up cache
  for (int i = 0; i < WARMUP_STEPS; i++) {
    key_type key;
    std::string val;
    workload_generator(i+1, key, val);
    Cache::val_type pval = val.c_str();

    cache.set(key, pval, strlen(pval)+1);
  }

  std::cout << "Space used (after warmup): " << cache.space_used() << '\n';

  // report Cache's hit rate
  int total_get = 0;
  int hits = 0;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

  for (int i=0; i < num_requests; i++) {

    key_type key;
    std::string val;
    auto req_type = workload_generator(seed + i, key, val);
    Cache::val_type pval = val.c_str();

    if (req_type == 0) {
      // std::cout << "GET" << '\n';
      cache.get(key, size);
      total_get++;
      if (size == 1) hits++;

    } else if (req_type == 1) {
      // std::cout << "SET" << '\n';
      cache.set(key, pval, strlen(pval)+1);
    } else {
      // std::cout << "DEL" << '\n';
      cache.del(key);
    }
  }

  std::cout << "Space used (final): " << cache.space_used() << '\n';
  std::cout << "Hit rate:" << (double) hits / total_get << '\n';

  double p95;
  int mean;
  baseline_performance(cache, size, p95, mean);
  std::cout << "95 percentile latency: " << p95 << '\n';
  std::cout << "Mean throughput: " << mean << '\n';
  return 0;
}
