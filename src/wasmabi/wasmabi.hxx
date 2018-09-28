#pragma once

#include <bete/request.hxx>
#include <array>
#include <vector>
#include <list>
#include <sstream>

#include <boost/fusion/include/tag_of.hpp>
#include <boost/fusion/include/struct.hpp>

#include <pre/fusion/for_each_member.hpp>

#include <type_traits>

namespace wasmabi {

  template <typename Container>
  struct is_container : std::false_type { };

  template <typename... Ts> struct is_container<std::list<Ts...> > : std::true_type { };
  template <typename... Ts> struct is_container<std::vector<Ts...> > : std::true_type { };
  template <> struct is_container<std::string> : std::true_type { };

  template<class T>
  using is_adapted_struct_t = typename std::is_same<
      typename boost::fusion::traits::tag_of<T>::type, 
      boost::fusion::struct_tag
  >::type;
  
  template<class T>
  inline void write(std::stringstream& stream, const T& value) {
    using decayed_T = typename std::decay_t<T>;

    if constexpr (is_adapted_struct_t<decayed_T>::value) {
      pre::fusion::for_each_member(value, [&stream](const char* name, const auto& member_value){
        write(stream, member_value); 
      }); 

    } else if constexpr (is_container<decayed_T>::value) {
      wasmabi::write(stream, value.size());
      std::for_each(value.begin(), value.end(), [&stream](auto& v) { wasmabi::write(stream, v); });
      
    } else if constexpr(std::is_trivially_copyable_v<decayed_T>) {
      std::array<char, sizeof(decayed_T)> buf{'\0',};

      const char* addressof_value = reinterpret_cast<const char*>(std::addressof(value));
      std::copy(addressof_value, addressof_value + sizeof(value), buf.data());
      stream.write(buf.data(), buf.size());

    } else if constexpr(belle::vue::has_type_observed_t<decayed_T>::value) {
      wasmabi::write(stream, value.get_proxied());
    } else {
      static_assert(std::is_same<T, int>::value, "Unsupported type, maybe you forgot to adapt it with : BOOST_FUSION_ADAPT_STRUCT ?");
    }
  }


  template<class T>
  inline void read(std::stringstream& stream, T& value) {
    using decayed_T = typename std::decay_t<T>;

    if constexpr (is_adapted_struct_t<decayed_T>::value) {
      pre::fusion::for_each_member(value, [&stream](const char* name, auto& member_value){
          wasmabi::read(stream, member_value); 

      }); 

    } else if constexpr (is_container<decayed_T>::value) {
      std::size_t container_size = 0;
      wasmabi::read(stream, container_size);
      std::cout << "We have read container size : " << container_size << std::endl;
      value.reserve(container_size);
      value.clear();
      for (std::size_t i = 0; i < container_size; ++i) {
        typename decayed_T::value_type deserialized{};
        wasmabi::read(stream, deserialized);
        value.push_back(deserialized);
      }

    } else if constexpr(std::is_trivially_copyable_v<decayed_T>) {
      stream.read(reinterpret_cast<char*>(&value), sizeof(value));

    } else if constexpr(belle::vue::has_type_observed_t<decayed_T>::value) {
      wasmabi::read(stream, value.get_proxied());
    
    } else {
      static_assert(std::is_same<T, int>::value, "Unsupported type, maybe you forgot to adapt it with : BOOST_FUSION_ADAPT_STRUCT ?");
    }

  }
}
