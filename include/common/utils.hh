#ifndef DUMB_UTILS_HH__
#define DUMB_UTILS_HH__

#include <vector>
#include <string>

namespace dumb
{
namespace utils
{

std::vector<std::byte> ReadBinaryFile( const std::string& filename);
std::string ReadTextFile( const std::string& filename);

} // ! namespace utils
} // ! namespace dumb

#endif // ! DUMB_UTILS_HH__
