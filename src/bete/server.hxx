#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <regex>
#include <numeric>

#include <boost/callable_traits/args.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_parse_auto.hpp>
#include <boost/lexical_cast.hpp>

#include <js/bind.hpp>
#include <bete/common.hxx>
#include <bete/mime/mime.types.hxx>
#include <bete/runtime.hxx>
#include <bete/request.hxx>
#include <bete/response.hxx>
#include <bete/detail/parameter/serve.hxx>


namespace bete {

  namespace fs = boost::filesystem;
  using namespace std::placeholders;

  struct session {

    session(request request_, response response_) 
      : request(request_),
        response(response_)
    {}

    session(session&& other)
      : request(other.request),
        response(other.response)
    {
      other.moved_from = true;
    }

    //TODO: disable copy operation
    bool moved_from = false;
    request request;
    response response;

    ~session() {

      if (!moved_from) {
        // Flush the response
        response.end();
      }
    }
  };

  struct server {
    
    server() :
      this_(bete::require("http")),
      fs_(bete::require("fs"))
    {}

    void listen(std::int32_t port) {

      this_.call<val>("createServer",
        js::bind(&server::on_request, this,  _1, _2))
        .call<val>("listen", port);
    }

    template<class Handler>
    void GET(const std::string& route, Handler&& functor_to_serve) {
      std::function<bool(request , response& )> generated_handler = [route, functor_to_serve](request request, response& response) {

        namespace x3 = boost::spirit::x3;
        namespace qi = boost::spirit::qi;
        namespace ct = boost::callable_traits;
        using namespace boost::mp11;


        std::string path(request.url());
        path = path.substr(route.size());


        std::vector<std::string> splitted_args;
        using x3::char_;
        using x3::lit;

        auto const slash_separated_string=  *(char_ - "/");
        bool parse_success = x3::parse(path.begin(), path.end(), -lit("/") >> (slash_separated_string % "/"), splitted_args);

        if (!parse_success) {
          std::cout << "Path parsing failed. Leaving;" << std::endl; //TODO: Logging concept
          return false;
        }

        auto url_decode = [](auto str) { 
          std::string ret;
          char ch;
          int i, ii, len = str.length();

          for (i=0; i < len; i++){
              if(str[i] != '%'){
                  if(str[i] == '+')
                      ret += ' ';
                  else
                      ret += str[i];
              }else{
                  sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
                  ch = static_cast<char>(ii);
                  ret += ch;
                  i = i + 2;
              }
          }
          return ret;  
        };

        //ct::args_t<Handler> args_parsed;
        using session_filtered_args = 
          mp_remove<ct::args_t<Handler>, session&&>;
        
        session_filtered_args args_parsed;

        auto fail = false;
        auto raw = splitted_args.begin();
        tuple_for_each(args_parsed, [&](auto& result) {
          if (fail) return;

          if constexpr(std::is_same_v<std::string, std::decay_t<decltype(result)> >) {
            result = url_decode(*raw);
          } else {
            fail = !qi::parse(raw->begin(), raw->end(), qi::auto_, result);
          }

          ++raw;
        });

        if (fail) {
          //response.status_code(404);
          std::cout << "Argument parsing failed. Leaving;" << std::endl; //TODO: Logging concept
          return false;
        } else {
          std::apply(functor_to_serve, std::tuple_cat(std::make_tuple(session { request, response }), args_parsed));
          return true;
        }

      };

      handlers.insert({route, generated_handler});
    }

    private:
    void on_request(request req, response res) {
      //val::global("console").call<val>("log", req.val_, res.val_);


      for (auto& handler : handlers) {
        std::cout << "handler : " << handler.first << "-- " << handler.first.size() << std::endl;
      }

      auto url = req.url();
      std::cout << "Serving : " << url << std::endl;

      std::regex url_regex("(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
      std::smatch what;

      if (regex_match(url, what, url_regex)) {

        std::string path       = what[1];
        //auto parameters = what[2];
        //auto fragment   = what[3];

        std::regex rx("/");
        std::vector<std::string> path_parts {
           std::sregex_token_iterator(path.begin(), path.end(), rx, -1),
           std::sregex_token_iterator()
        };


        std::string base = path_parts[0];
        for (auto iter_path = std::next(path_parts.begin()); iter_path != path_parts.end(); ++iter_path) {
          base += "/"s + *iter_path;
          std::cout << "trying to handle : " << base << "-- " << base.size() << std::endl;
          auto range = handlers.equal_range(base);
          for (auto it = range.first; it != range.second; ++it) {
            std::cout << "Handling with route : " << it->first << std::endl;
            if (it->second(req, res)) return;
          }
        };

        // Check if a static file could not serve this
        if (path == "/") { path = "index.html"; } else { path = "." + path; }

        auto path_rep = fs::path("workdir") / path; 

        std::string content_type = "application/octet-stream";
        if (mime::mime_types.find(path_rep.extension().generic_string()) != mime::mime_types.end()) {
          content_type = mime::mime_types.at(path_rep.extension().generic_string());
        }

        if ( (fs::exists(path_rep)) && (fs::is_regular_file(path_rep)) ) {
          res.writeHead(200, { {"Content-Type", content_type } });
          auto read_stream = fs_.call<val>("createReadStream", path);
          read_stream.call<val>("pipe", res.val_);
        } else {
          // end any unserved request
          res.writeHead(404, {});
          res.end();
        }
      }
      
    }

    private:
    val this_;
    val fs_;

    //! handlers attached to a root route path, that return true if they can handle the request. 
    std::multimap<std::string, std::function<bool(request , response& )> > handlers;
  };

}
