#include <iostream>
#include <fstream>
#include <mstch/mstch.hpp>
#include <emscripten.h>

#include <boost/spirit/home/x3.hpp>

  namespace mime_types_parser {
    using namespace boost::spirit;
    using x3::char_;
    using x3::eol;
    using x3::_attr;
    using x3::rule;
    using x3::phrase_parse;
    using x3::ascii::space;
    using x3::blank;

    rule<class value_id, std::string> const value = "value";
    auto const value_def = +(char_ - space);

    rule<class comments_id> const comments = "comments";
    auto const comments_def = ("#" >> *(char_ - eol) >> eol);

    rule<class empty_line_id> const empty_line = "empty_line";
    auto const empty_line_def = (*blank >> eol);

    BOOST_SPIRIT_DEFINE(value, comments, empty_line)
  }


int main(int argc, char** argv) {

  EM_ASM(
    require("fs");
    FS.mkdir('/workdir');
    FS.mount(NODEFS, { root: '.' }, '/workdir');
  );
  
  if (argc < 2) {
    std::cout << "usage " << argv[0] << " /path/to/mime.types \n"
      << "This is the file which register mappings between file extensions and mime typses provided by the debian package mime-support. \n"
      << "It usually resides in /etc/mime.types\n"
      << std::endl;
    std::exit(1);
  }

  using namespace std::literals;

/*  auto input = R"(###############################################################################


application/activemessage
application/andrew-inset                        ez boom
application/annodex                             anx)"s;*/

  std::cout << "Opening " << argv[1] << std::endl;
  std::fstream ifs(argv[1], std::ios::binary | std::ios::in);
  std::string input {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};

  std::cout << input << std::endl;

  using namespace mime_types_parser;

  std::string current_mime_type;
  auto set_current_mime_type = [&current_mime_type](auto& ctx) {
    current_mime_type = _attr(ctx);
  };

  std::map<std::string, std::string> mime_types;

  auto add_current_mime_type_to_extension = [&mime_types, &current_mime_type](auto& ctx) {
    mime_types[_attr(ctx)] = current_mime_type;
  };

  auto start = input.begin();
  bool r = phrase_parse(start, input.end(),
    +(
      empty_line
      | (value_def[set_current_mime_type] >> (
            // Either there are values
            (+blank >> *(value_def[add_current_mime_type_to_extension] % +blank) >> *blank >> -eol)
            // Or the mimetype is not linked to a file extension, then we ignore it.
          | (*blank >> eol)
      )) 
     )
    ,
    comments
  );

  for (auto mime : mime_types) {
    std::cout << mime.first << " - " << mime.second << std::endl;
  }

  std::cout << "parsing stopped at : " << std::distance(input.begin(), start) << std::endl;

  if (start != input.end()) // fail if we did not get a full match
      return 1;


  return r ? 0 : 1;
}
