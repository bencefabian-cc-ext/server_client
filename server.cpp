#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <unordered_set>

#include "shared_socket.hpp"

using error_code = boost::system::error_code;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class ActiveSockets {
  std::unordered_set<SharedSocket*> sockets_{};
  std::mutex mtx_{};

public:
  class Remover {
    std::function<void()> fn_;
  public:
    explicit Remover(std::function<void()> fn) : fn_{std::move(fn)} {}
    ~Remover() {
      fn_();
    }
    Remover(const Remover &) = default;
    Remover(Remover &&) = delete;
    Remover &operator=(const Remover &) = delete;
    Remover &operator=(Remover &&) = delete;
  };
  Remover add_socket(SharedSocket *const s) {
    {
      std::scoped_lock lock{mtx_};
      sockets_.insert(s);
    }
    return Remover {
      [this, s]() {
        std::scoped_lock lock{mtx_};
        auto it = std::find(sockets_.begin(), sockets_.end(), s);
        if (it != sockets_.end()) {
          sockets_.erase(it);
        }
      }};
  }

  void send_to_all(const std::string &message, error_code & ec) {
    std::scoped_lock lock{mtx_};
    for (auto sock : sockets_) {
      auto socket = sock->acquire();
      asio::write(*socket, asio::buffer(message), ec);
      if (ec) {
        return;
      }
    }
  }
};

void session(tcp::socket socket,
             std::shared_ptr<ActiveSockets> active_sockets) noexcept {
  SharedSocket shared_socket{std::move(socket)};
  auto remover = active_sockets->add_socket(&shared_socket);
  asio::streambuf buffer{1024};
  error_code ec;
  {
    auto sock = shared_socket.acquire();
    asio::read_until(*sock, buffer, '\n', ec);
  }
  if (ec) {
    return;
  }
  std::string name;
  {
    std::istream first_msg{&buffer};
    std::getline(first_msg, name);
  }
  BOOST_LOG_TRIVIAL(info) << name << " has logged on.";
  while (true) {
    {
      auto sock = shared_socket.acquire();
      auto mode = sock->non_blocking();
      sock->non_blocking(true);
      asio::read_until(*sock, buffer, '\n', ec);
      sock->non_blocking(mode);
    }
    if (ec == asio::error::try_again) {
      continue;
    }
    if (ec == asio::error::eof) {
      BOOST_LOG_TRIVIAL(info) << "connection closed";
      break; // from while
    }
    if (ec) {
      BOOST_LOG_TRIVIAL(error) << ec.message();
      return;
    }
    std::istream maybe_line{&buffer};
    std::string content;
    std::getline(maybe_line, content);
    BOOST_LOG_TRIVIAL(info) << "Received: " << content;
    content.append("\n");
    std::string message{name};
    message.append(": ");
    message.append(content);
    message.append("\n");
    active_sockets->send_to_all(message, ec);
    if (ec) {
      return;
    }
  }
}

int main() {
  asio::io_context io;
  const int port_num = 5000;
  tcp::acceptor acceptor{io, tcp::endpoint{tcp::v4(), port_num}};
  BOOST_LOG_TRIVIAL(info) << "started listening on: " << port_num;
  auto active_sockets = std::make_shared<ActiveSockets>();
  while (true) {
    tcp::socket socket{io};
    acceptor.accept(socket);
    BOOST_LOG_TRIVIAL(info) << "connection accepted";
    std::thread(session, std::move(socket), active_sockets).detach();
  }
}
