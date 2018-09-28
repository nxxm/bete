#pragma once

#include <functional>
#include <map>
#include <tuple>
#include <string_view>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_parse_auto.hpp>
#include <boost/lexical_cast.hpp>

#include <rest/detail/parameter/at.hpp>
#include <rest/detail/parameter/serve.hpp>

namespace rest {

  struct session {
  };

  struct request {
    std::string path;
  };
  struct response {
    std::int32_t status_code;
  };

} // namespace rest

namespace rest::detail {
  struct route {

    void set_option( ) { }

    void set_option(const pre::named_param::value_<std::string>&& at ) {
      route_path = at;
    }

    template<class Handler>
    void set_option(pre::named_param::nary_<Handler>&& functor_to_serve) {

      //{ ':method': 'GET',
      //  ':path': '/',
      //  ':authority': '127.0.0.1:8443',
      //  ':scheme': 'https',
      //  'user-agent': 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:59.0) Gecko/20100101 Firefox/59.0',
      //  accept: 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
      //  'accept-language': 'en-US,en;q=0.5',
      //  'accept-encoding': 'gzip, deflate, br',
      //  'upgrade-insecure-requests': '1' }

      auto route_input_checker = [this, functor_to_serve](request&& request, response& response) {
        namespace ct = boost::callable_traits;
        using namespace boost::mp11;
        using namespace boost::algorithm;

        std::string path(request.path); //XXX: string_view here
        
        if (!starts_with(path, route_path)) {
          response.status_code = 404;
          return;
        }

        path = path.substr(route_path.size());

        using namespace boost::spirit;
        std::vector<std::string> args_raw;
        using x3::char_;
        using x3::lit;

        auto const slash_separated_string=  *(char_ - "/");
        bool parse_success = x3::parse(path.begin(), path.end(), -lit("/") >> (slash_separated_string % "/"), args_raw);

        if (!parse_success) {
          response.status_code = 404;
          std::cout << "Path parsing failed. Leaving;" << std::endl; //TODO: Logging concept
          return;
        }


        //ct::args_t<Handler> args_parsed;
        using session_filtered = mp_remove<
          mp_remove<ct::args_t<Handler>, rest::session>,
          rest::session&
        >;
        
        session_filtered args_parsed;

        auto args_parse_fail = false;
        auto arg_raw = args_raw.begin();
        tuple_for_each(args_parsed, [&arg_raw, &args_parse_fail](auto& arg) {
          // Cancel for_each if there was a single failure
          if (args_parse_fail) return;

          using namespace boost::spirit;
          args_parse_fail = !qi::parse(arg_raw->begin(), arg_raw->end(), qi::auto_, arg);
          if (args_parse_fail) return;

          std::cout << "arg_raw : " << *arg_raw << " -- ";
          std::cout << "arg : " << typeid(arg).name() << std::endl;
          std::cout << arg << std::endl;
          ++arg_raw;
        });

        if (args_parse_fail) {
          response.status_code = 404;
          std::cout << "Argument parsing failed. Leaving;" << std::endl; //TODO: Logging concept
          return;
        } else {
          rest::session current_session{};
          std::apply(functor_to_serve, std::tuple_cat(std::tie(current_session), args_parsed));
        }
      };

      route_handler = route_input_checker;
    }

    template<class Handler>
    static std::map<route*, std::reference_wrapper<pre::named_param::nary_<Handler>>> _functor_to_serve;

    //private:
    std::string route_path;
    std::function<void(request&&,response&)> route_handler;
  };

}