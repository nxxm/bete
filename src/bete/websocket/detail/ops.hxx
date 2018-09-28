#pragma once

#include <string>
#include <system_error>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/types.h>


namespace bete::websocket::detail {

  using namespace std::string_literals;

  //! Reads out of a websocket descriptor a full message.
  inline std::string read(int sockfd, std::error_condition& ec) {
    ec = std::error_condition{};
    ssize_t res;

    std::size_t msg_size{};
    
    res = recvfrom(sockfd, &msg_size, sizeof(msg_size), 0, NULL, NULL);
    if (res == -1) {
      ec = std::error_code(errno, std::generic_category()).default_error_condition();
      return ""s;
    } else if (res == 0) {
      return 0;
    }

    printf("do_msg_read: allocating %zu bytes for message\n", msg_size);

    std::string return_buf(msg_size, char{});

    res = recvfrom(sockfd, return_buf.data(), return_buf.size(), 0, NULL, NULL);
    if (res == -1) {
      ec = std::error_code(errno, std::generic_category()).default_error_condition();
      return ""s;
    }

    return_buf.resize(res);
    printf("do_msg_read: read %zd bytes\n", res);

    return return_buf;
  }


  inline void write(int sockfd, const std::string& message, std::error_condition& ec) {
    ec = std::error_condition{};
    ssize_t res;

    auto msg_size = message.size();
    std::string buffer(sizeof(msg_size) + msg_size, char{});
    std::copy(reinterpret_cast<char*>(&msg_size), reinterpret_cast<char*>(&msg_size) + sizeof(msg_size), buffer.begin());
    std::copy(message.begin(), message.end(), buffer.begin() + sizeof(msg_size));

    printf("do_msg_write: sending message header for %zu bytes\n", msg_size);

    res = send(sockfd, buffer.data(), buffer.size(), 0);

    if (res == -1) {
      ec = std::error_code(errno, std::generic_category()).default_error_condition();
      return;
    }

    printf("do_msg_write: wrote %zd bytes %zd\n", res, message.size());
  }

}
