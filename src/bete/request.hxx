#pragma once

#include <bete/common.hxx>


namespace bete {
  struct request {
    request(const val& val)
      : val_(val) 
    {}

    std::string url() const {
      return val_["url"].as<std::string>();
    }

    val val_;
  };
}
