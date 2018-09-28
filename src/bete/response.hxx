#pragma once

#include <cstdint>
#include <bete/common.hxx>


namespace bete {
  struct response {
    response(const val& val)
      : val_(val) 
    {}

    bool write(const std::string& chunk) {
      return val_.call<bool>("write", chunk);
    }

    bool write(const std::string& chunk,
        const std::string& encoding,
        std::function<void()> write_done) {

      return val_.call<bool>("write", chunk, encoding,
        js::bind(write_done));
    }

    bool flush() {
      return val_.call<bool>("flush");
    }

    void status_code(std::int32_t status_code) {
      this->writeHead(status_code, {});
    }

    bool writeHead(std::int32_t status_code, const std::map<std::string, std::string>& headers) {
      auto headers_js = val::global("Object").new_();
      for (auto entry : headers) {
        headers_js.set(entry.first, entry.second);
      }
      return val_.call<bool>("writeHead", status_code, headers_js);
    }

    void end() {
      val_.call<val>("end");
    }

    val val_;
  };
}
