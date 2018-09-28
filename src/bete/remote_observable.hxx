#pragma once

#include <variant>
#include <functional>
#include <boost/fusion/include/adapt_struct.hpp>

#include <belle/vue/observable.hxx>
#include <bete/websocket/client.hxx>
#include <bete/websocket/server.hxx>

#include <wasmabi/wasmabi.hxx>




namespace bete {


  using namespace std::placeholders;

  struct initial_query_t {
    std::string resource_uri;
  };

  struct broadcast_change_t {
    std::string resource_uri;
    std::string serialized_value;
  };

}

BOOST_FUSION_ADAPT_STRUCT(bete::initial_query_t, resource_uri);
BOOST_FUSION_ADAPT_STRUCT(bete::broadcast_change_t, resource_uri, serialized_value);

namespace bete {

  struct observable_server : public bete::websocket::server {
    
    observable_server() :
      server(std::bind(&observable_server::on_ressource_query, this, _1, _2))
    {}

    void on_ressource_query(server::client_t& client, std::string&& message) {
      std::cout << "initial resource query" << std::endl;
      std::stringstream ss(message);
      initial_query_t query;
      wasmabi::read<initial_query_t>(ss, query);

      std::cout << "initial resource query for : " << query.resource_uri << std::endl;

      auto found_ressource = remote_observables.find(query.resource_uri);
      if (found_ressource != remote_observables.end()) {

        broadcast_change_t change{query.resource_uri, found_ressource->second.handler_initial()};
        std::stringstream changed_ss;
        wasmabi::write(changed_ss, change);

        std::error_condition ec;
        this->send(client, changed_ss.str(), ec);
        if (!ec) {
          // We can add it to the list of clients to broadcast to.
          found_ressource->second.broadcast_to.push_back(client);
        }
      }
    }

    void broadcast(const std::string& resource_uri, std::string&& serialized) {
      auto found_ressource = remote_observables.find(resource_uri);
      if (found_ressource != remote_observables.end()) {
        for (auto client_it = found_ressource->second.broadcast_to.begin(); client_it != found_ressource->second.broadcast_to.end();) {
          std::error_condition ec;
          
          broadcast_change_t change{resource_uri, serialized};
          std::stringstream changed_ss;
          wasmabi::write(changed_ss, change);

          this->send(*client_it, changed_ss.str(), ec);
          std::cout << "Wrote to client : " << client_it->fd << ec.message() << std::endl;
          if (ec == std::errc::broken_pipe) {
            client_it = found_ressource->second.broadcast_to.erase(client_it); // cleanup client list
          } else {
            ++client_it;
          }
        }
      }
    }

    struct observable_handler_t {
      std::function<std::string()> handler_initial;
      std::vector<server::client_t> broadcast_to;
    };

    std::map<std::string, observable_handler_t> remote_observables;

    template<class RemoteObservable>
    void register_observable(RemoteObservable& observable, const std::string& resource_uri) {
      observable_handler_t initial_handler {
        [&observable]() {
          std::stringstream ss;
          wasmabi::write(ss, observable.get_proxied());
          return ss.str();
        }
      };

      observable.observe_onchange([this, resource_uri](const typename RemoteObservable::observed_t& value) {
        std::cout << "notifying bound client of changes for " << resource_uri << " !" << std::endl;
        //TODO: implement diffing containers 
        //TODO: implement diffing structs
        std::stringstream ss;
        wasmabi::write(ss, value);
        this->broadcast(resource_uri, ss.str());
      });

      remote_observables.emplace(std::make_pair(resource_uri, initial_handler));
    }

    


  };


  /**
   * \brief observable_client is a WebSocket client that is used to gather changes 
   *        to a subscribed remote_observable.
   *
   */
  struct observable_client : public bete::websocket::client {

    observable_client(std::function<void()> on_open) :
      client(
        on_open,
        std::bind(&observable_client::on_observed_change, this, _1)
      )
    {}

    std::map<std::string, std::function<void(const std::string&)>> remote_observables;

    void on_observed_change(std::string&& broadcast_change) {
      std::cout << "Receiving changes ! " << std::endl;
      std::stringstream ss(broadcast_change);
      broadcast_change_t changed;
      wasmabi::read(ss, changed);

      std::cout << "changes for " << changed.resource_uri << std::endl;
      auto found_handler = remote_observables.find(changed.resource_uri);
      if (found_handler != remote_observables.end()) {
        found_handler->second(changed.serialized_value);
      }
    }

    template<class RemoteObservable>
    void register_observable(RemoteObservable& observable, const std::string& resource_uri) {
      auto deserialize_handler = [&observable](const std::string& serialized) {
        std::cout << "Deserializing changes ! " << std::endl;
        std::stringstream ss(serialized);
        wasmabi::read(ss, observable.get_proxied());
        observable.notify_observers();
      };

      /*observable.observe_onchange([this, resource_uri](const typename RemoteObservable::observed_t& value) {
        //TODO: Implement sending client made changes
      });*/

      remote_observables.emplace(std::make_pair(resource_uri, deserialize_handler));

      bete::initial_query_t query{resource_uri};
      std::stringstream ss;
      wasmabi::write(ss, query);
      std::error_condition ec;
      this->send(ss.str(), ec);
      std::cout << "Send registration for " << resource_uri << " ec : " << ec.message() << std::endl;
      //TODO: handle error ec
    }


  };


  template<class RemoteObservable, class Endpoint>
  inline void register_deep_observable(RemoteObservable& observable, const std::string& resource_uri, Endpoint& endpoint) {
    using namespace wasmabi;

    
    if constexpr(belle::vue::has_type_observed_t<std::decay_t<RemoteObservable>>::value) {
      using decayed_T = typename std::decay_t<typename RemoteObservable::observed_t>;


      if constexpr (is_adapted_struct_t<decayed_T>::value) {
        pre::fusion::for_each_member(observable.get_proxied(), [&](const char* name, auto& member_value){
          register_deep_observable(member_value, resource_uri + "/"s + name, endpoint);
        }); 

      } else if constexpr (is_container<decayed_T>::value) {
        for (auto iter = observable->begin(); iter != observable->end(); ++iter) {
  //        if constexpr(has_type_observed_t<std::decay_t<decltype(*iter)>>::value) { -> no actually it must be observable all the way down
            register_deep_observable(*iter, resource_uri + "/"s + std::to_string(std::distance(observable->begin(), iter)), endpoint);
  //        } 
        }
        
      } else {
        //leaf, cannot be observed more.
      }


      // register at deepeset level, so that we can mark the leafs when notifying 
      // to not notify a parent change when only the leaf changes.
      std::cout << "registering " << resource_uri << std::endl;
      endpoint.register_observable(observable, resource_uri);
    } else {
      std::cout << ">>>>>>>>>>>>>>>>>>> REGISTERING a non observable : " << resource_uri << ", type : " << typeid(RemoteObservable).name() << std::endl;
      //static_assert(std::is_same<RemoteObservable, int>::value, "The type isn't observable did you forget to register it with BELLE_VUE_OBSERVABLE(T, ...) ?");

    }

  }

  /**
   * \brief The remote_observable is allowing to publish or subscribe to any C++ 
   *        datastructure over a WebSocket connection.
   *
   *        It can be either server or client hosted data structures.
   *
   *        Server publishing a datamodel :
   *          observable_server server;
   *          std::vector<user_t> users; 
   *          remote_observable observable_users(users);
   *          observable_users.publish(server, "/userslist");
   *
   *        On the client side : 
   *          observable_client client;
   *          std::vector<user_t> users;
   *          remote_observable observable_users(users);
   *          observable_users.subscribe(client, "/userslist");
   *        
   */
  template<class T>
  struct remote_observable : public belle::vue::observable<T> {

    //std::variant<observable_server, observable_client> source;

    using belle::vue::observable<T>::observable;
    
    //! Shares on the given server the observed value at the given resource_uri.
    void publish(observable_server& server, const std::string& resource_uri) {
      server.register_observable(static_cast<belle::vue::observable<T>&>(*this), resource_uri);
    }
    
    //! Shares on the given server the observed value registering a change handler at all possible level for the given root resource_uri.
    void deep_publish(observable_server& server, const std::string& resource_uri) {
      register_deep_observable(static_cast<belle::vue::observable<T>&>(*this), resource_uri, server);
    }

    //! Subscribes on the given client the resource_uri for the given type
    void subscribe(observable_client& client, const std::string& resource_uri) {
      client.register_observable(static_cast<belle::vue::observable<T>&>(*this), resource_uri);
    }

    //! Shares on the given server the observed value registering a change handler at all possible level for the given root resource_uri.
    void deep_subscribe(observable_client& client, const std::string& resource_uri) {
      register_deep_observable(static_cast<belle::vue::observable<T>&>(*this), resource_uri, client);
    }
  };

}
