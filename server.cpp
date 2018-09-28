#include <bete/runtime.hxx>

#include <bete/websocket/server.hxx>
#include <bete/remote_observable.hxx>
#include <bete/server.hxx>

#include "datamodel.hxx"

using namespace std::string_literals;

struct app_scope {

  bete::server rest;
  bete::observable_server ws;

  belle::vue::observable<datamodel_t> datamodel{};

  void main() {
    register_deep_observable(datamodel, "/datamodel", ws);
    ws.async_listen(8888);

    datamodel->benchmarks->push_back({"The first benchmark", 43, 43, 43});

    rest.GET("/add", [&](bete::session&& session, std::string name, size_t min, size_t max, size_t avg) { 
    datamodel->benchmarks->push_back({name, min, max, avg});

      session.response.write("Added : "s + name 
          + "with min="s + std::to_string(min) 
          + ", max="s + std::to_string(max) 
          + ", avg="s + std::to_string(avg) + "."
          );

    });

    rest.listen(8787);
  }
};

static std::shared_ptr<app_scope> app_scope_;

int main() {
  bete::runtime r{};

  app_scope_ = std::make_shared<app_scope>();
  app_scope_->main();

  return 0;
}
