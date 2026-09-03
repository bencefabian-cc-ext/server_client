#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

int main(int argc, char** argv) {
    if (argc < 2) {
        BOOST_LOG_TRIVIAL(error) << "Needs to provide a name:\n" << argv[0] << " NAME\n";
        return 1;
    }

    asio::io_context io;
    tcp::resolver resolver(io);
    tcp::socket socket(io);

    std::string name{argv[1]};

    auto endpoints = resolver.resolve("localhost", "5000");
    asio::connect(socket, endpoints);
    std::string message{name};
    message.append("\n");
    asio::write(socket, asio::buffer(message));
    BOOST_LOG_TRIVIAL(info) << "Logging in as " << name << "\n";

    while (true) {
        std::string client_message;
        std::array<char, 1024> buffer{};
        boost::system::error_code ec;
        auto size = socket.read_some(asio::buffer(buffer), ec);
        if (ec) { break; }
        std::string message_received{buffer.data(), size};
        std::cout << message_received << std::endl;
        std::getline(std::cin, message_received);
        asio::write(socket, asio::buffer(message_received), ec);
        if (ec) { break; }
    }
    return 0;
}