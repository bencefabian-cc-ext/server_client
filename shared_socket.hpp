#pragma once

#include <boost/asio.hpp>
#include <mutex>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class SharedSocket {
  tcp::socket socket_;
  std::mutex mtx_;

public:
  SharedSocket(const SharedSocket &) = delete;
  SharedSocket(SharedSocket &&) = delete;
  SharedSocket &operator=(const SharedSocket &) = delete;
  SharedSocket &operator=(SharedSocket &&) = delete;
  explicit SharedSocket(asio::io_context &io) : socket_{io}, mtx_{} {}
  class Guard {
    tcp::socket &socket_;
    std::scoped_lock<std::mutex> lock_;
    explicit Guard(tcp::socket &socket, std::mutex &mutex)
        : socket_{socket}, lock_{mutex} {}
    friend class SharedSocket;

  public:
    Guard(const Guard &) = delete;
    Guard(Guard &&) = delete;
    Guard &operator=(const Guard &) = delete;
    Guard &operator=(Guard &&) = delete;

    tcp::socket &operator*() { return socket_; }

    const tcp::socket &operator*() const { return socket_; }
    tcp::socket *const operator->() { return &socket_; }

    const tcp::socket *const operator->() const { return &socket_; }
  };
  Guard acquire() { return Guard{socket_, mtx_}; }
};
