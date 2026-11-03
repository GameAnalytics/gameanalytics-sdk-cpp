// Keeps libc++ string allocations inside this binary.
#include <string>
#include <sstream>

#if defined(__APPLE__) && defined(_LIBCPP_VERSION)

template class std::basic_string<char>;
template std::string std::operator+<char, std::char_traits<char>, std::allocator<char>>(const char*, const std::string&);

template class std::basic_stringbuf<char>;
template class std::basic_stringstream<char>;
template class std::basic_ostringstream<char>;
template class std::basic_istringstream<char>;

#endif
