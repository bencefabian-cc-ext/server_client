#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

using error_code = boost::system::error_code;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class ActiveSockets {
  std::vector<tcp::socket *> sockets_{};
  std::mutex mtx_{};

public:
  void add_socket(tcp::socket * const s) {
    sockets_.push_back(s);
  }
};

void session(tcp::socket socket,
             std::shared_ptr<ActiveSockets> active_sockets) noexcept {
  asio::streambuf buffer{1024};
  error_code ec;
  asio::read_until(socket, buffer, '\n', ec);
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
    auto size_read = asio::read_until(socket, buffer, '\n', ec);
    if (ec == asio::error::eof) {
      BOOST_LOG_TRIVIAL(info) << "connection closed";
      break; // from while
    }
    if (ec) {
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
    asio::write(socket, asio::buffer(message), ec);
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
