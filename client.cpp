#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

int main() {
    asio::io_context io;
    tcp::resolver resolver(io);
    tcp::socket socket(io);

    auto endpoints = resolver.resolve("localhost", "5000");
    asio::connect(socket, endpoints);
    // use socket
    return 0;
}