#include <iostream>
#include <utility>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

template<typename E>
class list;

template<typename E>
class node {
    E element;
    node<E> *next = nullptr;
    friend class list<E>;

public:
    explicit node(E &&e) : element(e) {
    }
};

template<typename E>
class list {
    node<E> *head = nullptr;

public:
    void append(E &&element) {
        if (head == nullptr) {
            head = new node<E>(std::move(element));
        } else {
            node<E> *last = head;
            while (last->next != nullptr) {
                last = last->next;
            }
            last->next = new node<E>(std::move(element));
        }
    }

    struct iterator {
        node<E> *current;

        E &operator*() {
            return current->element;
        }

        iterator &operator++() {
            current = current->next;
            return *this;
        }

        bool operator==(const iterator &other) const {
            return current == other.current;
        }
    };

    iterator begin() {
        return iterator{head};
    }

    iterator end() {
        return iterator{nullptr};
    }
};

namespace fs = boost::filesystem;
using error_code = boost::system::error_code;

class MySpecificFsError : public fs::filesystem_error {
    // some of my context data
public:
    MySpecificFsError() : fs::filesystem_error("This is my error", error_code()) {
    }
};

int count_files(const char *path, error_code &ec) {
    auto entries = fs::directory_iterator(path, ec);
    if (ec) {
        return -1;
    }
    int result = 0;
    for (auto &entry: entries) {
        if (entry.is_regular_file(ec)) {
            if (ec) {
                return -1;
            }
            result++;
        }
    }
    return result;
}

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

void session(tcp::socket socket) noexcept {
    asio::streambuf buffer{1024};
    error_code ec;
    while (true) {
        auto size_read= asio::read_until(socket, buffer, '\n', ec);
        if (ec == asio::error::eof) {
            std::cout << "connection closed" << std::endl;
            break; // from while
        }
        if (ec) {
            return;
        }
        std::istream maybe_line{&buffer};
        std::string content;
        std::getline(maybe_line, content);
        std::cout << "Received: " << content << std::endl;
        content.append("\n");
        BOOST_LOG_TRIVIAL(info) << "Received: " << content << std::endl;
        asio::write(socket, asio::buffer(content), ec);
        if (ec) {
            return;
        }
    }
}

int main() {
    asio::io_context io;
    const int port_num = 5000;
    tcp::acceptor acceptor{io, tcp::endpoint{tcp::v4(), port_num}};
    std::cout << "started listening on: " << port_num << "\n";
    while (true) {
        tcp::socket socket{io};
        acceptor.accept(socket);
        std::cout << "connection accepted" << std::endl;
        std::thread(session, std::move(socket)).detach();
    }
}
