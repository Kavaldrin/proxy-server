#ifndef __PROXY_MANAGER_HPP__
#define __PROXY_MANAGER_HPP__


#include <unordered_map>
#include <vector>
#include <type_traits>
#include <set>
#include <deque>

namespace Proxy
{


class ProxyManager
{

public:

    template <typename T>
    bool addDataForDescriptor(int descriptor, T&& data) noexcept;

    void handleReceivedPacket(int descriptor) noexcept;
    std::pair<bool, bool> handleStoredBuffers(int fd) noexcept;

    bool addEstablishedConnection(int source, int destination);
    bool destroyEstablishedConnectionBySource(int source);
    bool destroyEstablishedConnectionByDestination(int destination);
    std::optional<int> getSecondSocketIfEstablishedConnection(int socket);


    bool isDestination(int socket);
    bool isSource(int socket);


private:

    std::unordered_map<int, int> m_sourceToDest;
    std::unordered_map<int, int> m_destToSource;

    std::unordered_map< int, std::deque< std::vector<char> > > m_storage;
};


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
    static_assert(is_char_vector<T>::value, "your type is not char vector");

    m_storage[descriptor].push_back(std::forward<T>(data));
    return true;
}




}
#endif
