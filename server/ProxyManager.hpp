#ifndef __PROXY_MANAGER_HPP__
#define __PROXY_MANAGER_HPP__

#include "parserHttp.h"

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <set>

namespace Proxy
{


class ProxyManager
{

public:

    //a wiesz co nigdy nie uzywalem perfect forwardingu mysle ze czas gdy pali nam sie dupa przez mase projektow jest odpowiedni
    template <typename T>
    bool addDataForDescriptor(int descriptor, T&& data) noexcept;

    void handleReceivedPacket(int descriptor) noexcept;
    std::pair<bool, bool> handleStoredBuffers(int fd) noexcept;

    bool addEstablishedConnection(int source, int destination);
    bool destroyEstablishedConnectionBySource(int source);
    bool destroyEstablishedConnectionByDestination(int destination);
    std::optional<int> getSecondSocketIfEstablishedConnection(int socket);

    bool isDestination(int socket);

    void addEndBodyMethod(int source, int destination, HttpRequest_t headers);


private:

    //stac mnie na pamiec to nie lata 90
    std::unordered_map<int, int> m_sourceToDest;
    std::unordered_map<int, int> m_destToSource;
    std::unordered_map<int, std::pair<std::string, std::string>> m_sockEndBody;

    std::unordered_map< int, std::vector<char> > m_storage;
};



//lubie strzelac z armaty do wrobla
//zeby grzesiowi bylo glupio ze takim panom inzynierom jak milosz f i kamil b kazal pisac taki gowno-programik
template <typename T, typename _ = void>
struct is_char_vector : std::false_type {};


template <typename U>
struct is_char_vector< U,
                    typename std::enable_if<
                        std::is_same<U,
                            std::vector< typename std::vector<char>::value_type,
                                         typename std::vector<char>::allocator_type >
                            >::value
                        >::type
                    >
        : std::true_type {};



template <typename T>
bool ProxyManager::addDataForDescriptor(int descriptor, T&& data) noexcept
{
    static_assert(is_char_vector<T>::value, "your type is not char vector daun");

    if(m_storage.find(descriptor) != m_storage.end()){ return false; }
    m_storage.insert( {descriptor, std::forward<T>(data)} );

    return true;
}




}
#endif
