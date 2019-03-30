#pragma once

#include <string>
#include <map>

namespace bete::mime {

   static inline const std::map<std::string, std::string> mime_types{
    { ".html", "text/html"},
    { ".js", "text/javascript"},
    { ".css", "text/css"},
    { ".json", "application/json"},
    { ".png", "image/png"},
    { ".jpg", "image/jpg"},
    { ".gif", "image/gif"},
    { ".wav", "audio/wav"},
    { ".mp4", "video/mp4"},
    { ".woff", "application/font-woff"},
    { ".ttf", "application/font-ttf"},
    { ".eot", "application/vnd.ms-fontobject"},
    { ".otf", "application/font-otf"},
    { ".svg", "application/image/svg+xml"}
  };

}
