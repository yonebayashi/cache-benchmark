#include "crow_all.h"
#include "evictor.hh"
#include "fifo_evictor.hh"
#include "cache.hh"

#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>

using namespace crow;

#define LOCALHOST_ADDRESS "127.0.0.1"
#define PORT 8080

int MAXMEM = 8;

void set_header(const crow::request& req, crow::response& res, const Cache& cache) {
  res.add_header("Space-Used", std::to_string(cache.space_used()));
  res.set_header("Accept", req.get_header_value("accept"));
  res.set_header("Content-Type", "application/json");

  res.end();
}


int main(int argc, char *argv[])
{
  int opt;

  int maxmem = MAXMEM;
  std::string host;
  std::string port;
  short threads;

  while((opt = getopt(argc, argv, "m:s:p:t:")) != -1)
  {
      switch(opt)
      {
          case 'm':
              if (optarg) maxmem = std::atoi(optarg);
              std::cout << "Maxmem: " << maxmem << std::endl;
              break;
          case 's':
              if (optarg) host = optarg;
              std::cout << "Server host: " << host << std::endl;
              break;
          case 'p':
              if (optarg) port = optarg;
              std::cout << "Port: " << port << std::endl;
              break;
          case 't':
              if (optarg) threads = std::atoi(optarg);
              std::cout << "Thread count: " << threads << std::endl;
              break;
          default: /* '?' */
              std::cout << "Usage: " << argv[0] << " [-m maxmem] [-s server] [-p port] [-t threads]\n";
              exit(EXIT_FAILURE);
      }
    }

  // FifoEvictor* fifo= new FifoEvictor();
  Cache cache(maxmem, 0.75, nullptr);
  Cache::size_type size;


  SimpleApp app;
  CROW_ROUTE(app, "/")
    .methods("HEAD"_method)
  ([&cache](const crow::request& req, crow::response& res)
  {
    set_header(req, res, cache);
    res.end();

  });


  CROW_ROUTE(app, "/<string>")
    .methods("HEAD"_method ,"GET"_method, "DELETE"_method, "POST"_method)
  ([&cache, &size](const crow::request& req, crow::response& res, const key_type& key)
  {
    if ((key == "reset") && (req.method == "POST"_method)) {
      cache.reset();
      res.end();
    }
    if (req.method == "HEAD"_method) {
      set_header(req, res, cache);
    } else if (req.method =="GET"_method) {
      auto value = cache.get(key, size);
      if (!value) {
        res.code = 404;
        res.write("Key not found");
        res.end();
      } else {
        crow::json::wvalue x;
        x["key"] = key;
        x["value"] = value;
        res.write(crow::json::dump(x));
        res.end();
      }
    } else {
      // DELETE request
      if (cache.del(key)) {
        res.end();
      }
      else {
        res.code = 404;
        res.write("Key not found");
        res.end();
      }
    }

  });


  CROW_ROUTE(app, "/<string>/<string>")
    .methods("HEAD"_method, "PUT"_method)
  ([&cache](const crow::request& req, crow::response& res, const key_type& key, const std::string& value)
  {
    if (req.method == "HEAD"_method) {
      set_header(req, res, cache);
    }

    // PUT request
    Cache::val_type pval = value.c_str();
    cache.set(key, pval, strlen(pval)+1);

    res.end();

  });


  auto _ = async(std::launch::async, [&]{app.bindaddr(LOCALHOST_ADDRESS).port(PORT).run();});

  asio::io_service is;
    {
        asio::ip::tcp::socket c(is);

        try
        {
            c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS), PORT));
        }
        catch(std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

  return 0;
}
