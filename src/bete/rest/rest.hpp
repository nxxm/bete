#pragma once

#include <pre/fwd/fwd.hpp>

#include <rest/application.hpp>

//TODO: Implement route-registration to handler.
//TODO: Merge xxhr in-there to have almost the same API for server and client 


namespace rest::detail {

  template <typename T>
  void set_option(route& route_, T&& t) {
      route_.set_option(PRE_FWD(t));
  }

  template <typename T, typename... Ts>
  void set_option(route& app, T&& t, Ts&&... ts) {
      set_option(app, PRE_FWD(t));
      set_option(app, PRE_FWD(ts)...);
  }

} // namespace rest::detail

/**
 * \brief HTTP2 RESTful route specifier library
 */
namespace rest {

  /**
   * \brief HTTP GET route specifier.
   */
  template <typename... Ts>
  void GET(application& app, Ts&&... ts) {  detail::route route; detail::set_option(route, PRE_FWD(ts)...); app.GET(std::move(route)); }

  /**
   * \brief HTTP POST route specifier.
   */
  template <typename... Ts>
  void POST(application& app, Ts&&... ts) { detail::route route; detail::set_option(app, PRE_FWD(ts)...); app.POST(std::move(route)); }

  /**
   * \brief HTTP PUT route specifier.
   */
  template <typename... Ts>
  void PUT(application& app, Ts&&... ts) { detail::route route; detail::set_option(app, PRE_FWD(ts)...); app.PUT(std::move(route)); }

  /**
   * \brief HTTP HEAD route specifier.
   */
  template <typename... Ts>
  void HEAD(application& app, Ts&&... ts) { detail::route route; detail::set_option(app, PRE_FWD(ts)...); app.HEAD(std::move(route)); }

  /**
   * \brief HTTP DELETE route specifier.
   */
  template <typename... Ts>
  void DELETE(application& app, Ts&&... ts) {  detail::route route; detail::set_option(app, PRE_FWD(ts)...); app.DELETE(std::move(route)); }

  /**
   * \brief HTTP OPTIONS route specifier.
   */
  template <typename... Ts>
  void OPTIONS(application& app, Ts&&... ts) { detail::route route; detail::set_option(app, PRE_FWD(ts)...); app.OPTIONS(std::move(route)); }

  /**
   * \brief HTTP PATCH route specifier.
   */
  template <typename... Ts>
  void PATCH(application& app, Ts&&... ts) { detail::route route; detail::set_option(app, PRE_FWD(ts)...); app.PATCH(std::move(route)); }

}