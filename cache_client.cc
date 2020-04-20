#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>
#include <cstring>

#include "cache.hh"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;

#define LOCALHOST_ADDRESS "127.0.0.1"
#define PORT "8080"


class httpstream {
  std::string host;
  std::string port;
  net::io_context ioc;
  tcp::socket socket{ioc};
  beast::flat_buffer buffer;

public:
  httpstream() {};
  virtual ~httpstream() { close(); };

  httpstream(const httpstream&) = delete;
  httpstream& operator= (const httpstream&) = delete;

  httpstream(httpstream&) = delete;
  httpstream& operator= (httpstream&) = delete;

  void connect(const std::string& host, const std::string& port);
  void connect() { connect(host, port); };
  auto request (const http::verb& method, const std::string& target);

  bool is_open() { return socket.is_open(); };

  void close();
};

void httpstream::connect(const std::string& host, const std::string& port)
{
  this->host = host;
  this->port = port;

  // These objects perform our I/O
  tcp::resolver resolver{ioc};

  // Look up the domain name
  auto const results = resolver.resolve(host, port);

  // Make the connection on the IP address we get from a lookup
  net::connect(socket, results.begin(), results.end());
}

void httpstream::close()
{
  // Gracefully close the socket
  beast::error_code ec;
  socket.shutdown(tcp::socket::shutdown_both, ec);

  // not_connected happens sometimes
  // so don't bother reporting it.
  //
  if(ec && ec != beast::errc::not_connected)
      throw beast::system_error{ec};

  // If we get here then the connection is closed gracefully
}

auto httpstream::request(const http::verb& method, const std::string& target)
{
  // Set up an HTTP request message
  int version = 11;
  http::request<http::empty_body> req{method, target, version};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  //req.keep_alive(true);

  // Send the HTTP request to the remote host
  http::response<http::string_body> res;
  beast::error_code ec;

  try {
    http::write(socket, req);

    // Receive the HTTP response
    http::read(socket, this->buffer, res, ec);

    if ( ec ) {
        if ( ec == beast::http::error::end_of_stream ) {
            close();
            buffer.consume(buffer.size());
            connect();
        } else {
            throw beast::system_error{ec};
        };
      }

  } catch(std::exception const& e)
  {
        std::cerr << "Error: " << e.what() << std::endl;
  }


  return res;
}



class Cache::Impl {
  public:
    std::string host_;
    std::string port_;

    Impl(std::string host, std::string port):
                  host_(host), port_(port)
    {}

    ~Impl() {}

    void set(key_type key, val_type val, size_type size)
    {
      httpstream stream_;
      stream_.connect(host_, port_);

      std::string target = "/" + key + "/" + val;

      auto res = stream_.request(http::verb::put, target);

      boost::ignore_unused(size);

      stream_.close();
      return;
    }


    val_type get(key_type key, size_type& val_size) const {
      httpstream stream_;
      stream_.connect(host_, port_);

      std::string target = "/" + key;

      auto res = stream_.request(http::verb::get, target);

      if (res.result() == http::status::ok) {
        val_size = 1;
      } else {
        val_size = 0;
      }

      stream_.close();
      return nullptr;
    };


    bool del(key_type key) {
      httpstream stream_;
      stream_.connect(host_, port_);

      std::string target = "/" + key;
      auto res = stream_.request(http::verb::delete_, target);

      if (res.result() == http::status::not_found)
          return false;

      stream_.close();
      return true;
    };


    size_type space_used() const {
      httpstream stream_;
      stream_.connect(host_, port_);

      std::string target = "/";   // could be anything
      auto res = stream_.request(http::verb::head, target);
      auto memused = std::atoi(res["Space-Used"].data());

      stream_.close();
      return memused;
    };


    void reset() {
      httpstream stream_;
      stream_.connect(host_, port_);

      std::string target = "/reset";
      stream_.request(http::verb::post, target);

      stream_.close();
      return;
    };
};


Cache::Cache(std::string host, std::string port) : pImpl_(new Impl(host, port)) {}
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
