#pragma once

#include <string>
#include <emscripten.h>
#include <emscripten/val.h>



namespace bete {
  using emscripten::val;

  struct runtime {
    runtime() {
      EM_ASM(
        require_ = require;

        FS.mkdir('/workdir');
        FS.mount(NODEFS, { root: '.' }, '/workdir');

      );
    }
  };

  inline  val require(const std::string& module) {
    return val::global("require_")(module);
  }
}
