#include <vector>
#include <string>
#include <fstream>

#include "utils.hh"

namespace dumb
{
namespace utils
{

std::vector<std::byte>
ReadBinaryFile( const std::string& filename)
{
    std::ifstream file{ filename, std::ios::binary};
    if ( !file.is_open() )
    {
        throw std::runtime_error{ "Unable to open file \"" + filename + "\": " + std::strerror( errno)};
    }

    file.seekg( 0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg( 0, std::ios::beg);

    std::vector<std::byte> buffer( size);
    if ( !file.read( reinterpret_cast<char *>( &buffer[0]), size) )
    {
        throw std::runtime_error{ "Unable to read file \"" + filename + "\": " + std::strerror( errno)};
    }

    file.close();

    return buffer;
}

std::string
ReadTextFile( const std::string& filename)
{
    std::ifstream file{ filename, std::ios::binary};
    if ( !file.is_open() )
    {
        throw std::runtime_error{ "Unable to open file \"" + filename + "\": " + std::strerror( errno)};
    }

    file.seekg( 0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg( 0, std::ios::beg);

    std::string buffer( size, '\0');
    if ( !file.read( &buffer[0], size) )
    {
        throw std::runtime_error{ "Unable to read file \"" + filename + "\": " + std::strerror( errno)};
    }

    file.close();

    return buffer;
}

void
WriteTextFile( const std::string& filename,
               std::string        data)
{
    std::ofstream file{ filename, std::ios::binary};
    if ( !file.is_open() )
    {
        throw std::runtime_error{ "Unable to open file \"" + filename + "\": " + std::strerror( errno)};
    }

    file.clear();

    if ( !file.write( &data[0], data.size()) )
    {
        throw std::runtime_error{ "Unable to write in file \"" + filename + "\": " + std::strerror( errno)};
    }

    file.close();
}

} // ! namespace utils
} // ! namespace dumb
