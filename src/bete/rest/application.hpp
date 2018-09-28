#pragma once

#include <string>

#include <rest/detail/route.hpp>

namespace rest {

  /**
   * \brief RESTful application.
   */  
  struct application {

    void DELETE(detail::route&&);
    void GET(detail::route&&);
    void HEAD(detail::route&&);
    void OPTIONS(detail::route&&);
    void PATCH(detail::route&&);
    void POST(detail::route&&);
    void PUT(detail::route&&);
  };

}