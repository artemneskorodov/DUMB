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
        throw std::runtime_error("Unable to open file \"" + filename + "\"");
    }

    file.seekg( 0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg( 0, std::ios::beg);

    std::vector<std::byte> buffer( size);
    if ( !file.read( reinterpret_cast<char *>( &buffer[0]), size) )
    {
        throw std::runtime_error( "Error while reading file \"" + filename + "\"");
    }

    return buffer;
}

std::string
ReadTextFile( const std::string& filename)
{
    std::ifstream file{ filename, std::ios::binary};
    if ( !file.is_open() )
    {
        throw std::runtime_error("Unable to open file \"" + filename + "\"");
    }

    file.seekg( 0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg( 0, std::ios::beg);

    std::string buffer( size, '\0');
    if ( !file.read( &buffer[0], size) )
    {
        throw std::runtime_error( "Error while reading file \"" + filename + "\"");
    }

    return buffer;
}


} // ! namespace utils
} // ! namespace dumb
