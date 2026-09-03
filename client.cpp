#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/system_error.hpp>

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

static void
receive_from_chat_process(std::shared_ptr<SharedSocket> socket) noexcept {
  static constexpr std::size_t BUFSIZE = 1024;
  while (true) {
    BOOST_LOG_TRIVIAL(debug) << "Waiting for a message";
    std::array<char, BUFSIZE> buf{};
    std::string message{};
    while (std::find(message.begin(), message.end(), '\n') == message.end()) {
      auto sock = socket->acquire();
      auto mode = sock->non_blocking();
      sock->non_blocking(true);
      boost::system::error_code ec;
      auto read = sock->read_some(asio::buffer(buf), ec);
      sock->non_blocking(mode);
      if (ec) {
        continue;
      }
      if (read == 0) {
        continue;
      }
      auto part = std::string{buf.data(), read};
      BOOST_LOG_TRIVIAL(debug) << "got part: " << part;
      message.append(part);
      buf.fill(0);
    }

    std::cout << message;
    // redisplay prompt
    std::cout << "> ";
    std::cout.flush();
  }
}

static void send_to_chat_process(std::shared_ptr<SharedSocket> socket) {
  std::cout << "> ";
  std::cout.flush();
  while (true) {
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
      return;
    }
    line.append("\n");
    auto sock = socket->acquire();
    BOOST_LOG_TRIVIAL(debug) << "acquired socket for sending message";
    auto n = asio::write(*sock, asio::buffer(line));
    BOOST_LOG_TRIVIAL(debug) << "sent " << n << " bytes to server";
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    BOOST_LOG_TRIVIAL(error) << "Needs to provide a name:\n"
                             << argv[0] << " NAME\n";
    return 1;
  }

  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  asio::io_context io;
  tcp::resolver resolver(io);
  auto socket = std::make_shared<SharedSocket>(io);

  std::string name{argv[1]};

  auto endpoints = resolver.resolve("localhost", "5000");
  try {
    auto sock = socket->acquire();
    asio::connect(*sock, endpoints);
    std::string message{name};
    message.append("\n");
    BOOST_LOG_TRIVIAL(debug) << "Logging in as " << name << "\n";
    asio::write(*sock, asio::buffer(message));
  } catch (const std::exception &exception) {
    BOOST_LOG_TRIVIAL(error) << exception.what();
    return 1;
  }

  std::thread{receive_from_chat_process, socket}.detach();
  try {
    send_to_chat_process(socket);
  } catch (const std::exception &exception) {
    return 1;
  }

  return 0;
}
