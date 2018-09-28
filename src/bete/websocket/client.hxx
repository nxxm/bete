#pragma once

#include <functional>
#include <bete/websocket/detail/ops.hxx>

#include <emscripten.h>


namespace bete::websocket {

  struct client {

    int server_fd{};
    int client_fd{};

    std::function<void()> on_open_;
    std::function<void(std::string&&)> on_message_;

    /**
     * \param on_message user provided callback when a message arrives
     */
    client(const std::function<void()>& on_open,
        const std::function<void(std::string&&)>& on_message) 
      : on_open_(on_open),
        on_message_(on_message)
    {}

    ~client() {

      if (client_fd) {
        ::close(client_fd);
        client_fd = 0;
      }

      if (server_fd) {
        ::close(server_fd);
        server_fd = 0;
      }
    }

    void connect(const std::string& address, std::int32_t port) {

      // create the socket and set to non-blocking
      server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (server_fd == -1) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
      }
      fcntl(server_fd, F_SETFL, O_NONBLOCK);

      // connect the socket
      struct sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      if (inet_pton(AF_INET, address.data(), &addr.sin_addr) != 1) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
      }
      
      auto res = ::connect(server_fd, (struct sockaddr *)&addr, sizeof(addr));
      if (res == -1 && errno != EINPROGRESS) {
        perror("connect failed");
        exit (EXIT_FAILURE);
      }

      emscripten_set_socket_open_callback(
        static_cast<void*>(this), 
        [](int fd, void* this_) { static_cast<client*>(this_)->on_open(fd); } 
      );

      emscripten_set_socket_message_callback(
        static_cast<void*>(this), 
        [](int fd, void* this_) { static_cast<client*>(this_)->on_message(fd); } 
      );

      emscripten_set_socket_close_callback(
        static_cast<void*>(this), 
        [](int fd, void* this_) { static_cast<client*>(this_)->on_close(fd); } 
      );
      
      emscripten_set_socket_error_callback(
        static_cast<void*>(this), 
        [](int fd, int err, const char* msg, void* this_) { 
          static_cast<client*>(this_)->on_error(fd, err, msg); 
        } 
      );
    }

    inline void on_open(int fd) { on_open_(); }
    inline void on_message(int fd) { 
      std::error_condition ec{};
      auto message = detail::read(fd, ec);

      if (!ec) {
        on_message_(std::move(message));
      } else {
        std::cout << "ERROR reading from fd : " << fd << ", text: " << ec.message() << std::endl;
      }
    }

    inline void on_close(int fd) {}
    inline void on_error(int fd, int err, const char* msg) {}

    inline void send(const std::string& message, std::error_condition& ec) {
      detail::write(server_fd, message, ec);
    }
 
  };

}
