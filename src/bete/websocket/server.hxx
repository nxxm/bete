#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <unordered_map>
#include <bete/websocket/detail/ops.hxx>

#include <emscripten.h>

namespace bete::websocket {

  //! A websocket server
  struct server {
    
    int server_fd{};

    struct client_t {
      int fd{};
    };

    //! maps client fd to it's client
    std::unordered_map<int, client_t> client_map;

    std::function<void(client_t&, std::string&&)> on_message_;

    /**
     * \param on_message user provided callback when a message arrives
     */
    server(const std::function<void(client_t&, std::string&&)>& on_message) 
      : on_message_(on_message)
    {}

    ~server() {
      if (server_fd) {
        ::close(server_fd);
        server_fd = 0;
      }
    }

    void async_listen(std::int32_t port) {
      struct sockaddr_in addr;
      int res;

      // create the socket and set to non-blocking
      server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (server_fd == -1) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
      }

      fcntl(server_fd, F_SETFL, O_NONBLOCK);

      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      if (inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr) != 1) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
      }

      res = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
      if (res == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
      }

      res = listen(server_fd, 50);
      if (res == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
      }

      // for simplicity this test just passes a basic char*
      emscripten_set_socket_connection_callback(
          static_cast<void*>(this), 
          [](int fd, void* this_) { static_cast<server*>(this_)->on_connect(fd); } 
      );
      emscripten_set_socket_message_callback(
          static_cast<void*>(this), 
          [](int fd, void* this_) { static_cast<server*>(this_)->on_message(fd); } 
      );
      emscripten_set_socket_close_callback(
          static_cast<void*>(this), 
          [](int fd, void* this_) { static_cast<server*>(this_)->on_close(fd); } 
      );
    }

    //TODO: add on_error

    //! accepts and keep track of client
    inline void on_connect(int fd) {
      client_t client;
      fd_set fdr;

      // see if there are any connections to accept or read / write from
      FD_ZERO(&fdr);
      FD_SET(server_fd, &fdr);

      if (client.fd) FD_SET(client.fd, &fdr);

      // for TCP sockets, we may need to accept a connection
      if (FD_ISSET(server_fd, &fdr)) {
        client.fd = accept(server_fd, NULL, NULL);
        assert(client.fd != -1);
      }

      client_map[client.fd] = client;
    }

    //! sends a message to the client, ec is fed with std::errc::broken_pipe if the client is invalid.
    inline void send(client_t& client, const std::string& message, std::error_condition& ec) {

      if (client_map.find(client.fd) != client_map.end()) {
        detail::write(client.fd, message, ec);
      } else {
        ec = std::errc::broken_pipe; 
      }
    }

    inline void on_message(int fd) {

      auto& client = client_map[fd];
      std::error_condition ec{};
      auto message = detail::read(fd, ec);

      if (!ec) {
        on_message_(client, std::move(message));
      } else {
        std::cout << "ERROR reading from client : " << fd << ", text: " << ec.message() << std::endl;
      }

    }

    inline void on_close(int fd) {
      std::cout << "deregistering client with fd : " << fd << std::endl;
      client_map.erase(fd);
    }
  };

}
