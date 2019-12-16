#ifndef __PROXY_MANAGER_HPP__
#define __PROXY_MANAGER_HPP__

#include "parserHttp.h"
#include "receiver.h"

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <set>

namespace Proxy
{

enum class EndBodyMethod {
    CHUNK,
    CONTENT_LENGTH,
    NONE
};

constexpr bool SHOULD_CHANGE_SOCKET_STATE = true;
constexpr bool SHOULD_REMOVE_SOCKET = true;

class ProxyManager
{
public:
    struct EndBodyParameters {
        EndBodyMethod endBodyMethod;
        std::optional<int> contentLength;
        int numMessagesFromWebBrowser;
        int numMessagesFromServer;
        bool shouldBeClosed;
        bool hasLastMsgBeenSent;
        bool isEncrypted;
    };

    template <typename T>
    bool addDataForDescriptor(int descriptor, T&& data) noexcept;

    void handleReceivedPacket(int descriptor) noexcept;
    std::pair<bool, bool> handleStoredBuffers(int fd, Receiver& receiver) noexcept;

    bool addEstablishedConnection(int source, int destination);
    bool destroyEstablishedConnectionBySource(int source);
    bool destroyEstablishedConnectionByDestination(int destination);
    std::optional<int> getSecondSocketIfEstablishedConnection(int socket);

    bool isDestination(int socket);

    void addEndBodyMethod(int source, int destination, HttpRequest_t headers);
    void createEndBodyParams(int source, int destination, bool isEncrypted);
    void deleteEndBodyParams(int source, int destination);
    std::optional<std::_Rb_tree_iterator<std::pair<const std::pair<int, int>, EndBodyParameters>>> getEndBodyParams(int sock1, int sock2);
    void incrementMessagesFromServer(int sock1, int sock2);
    void incrementMessagesFromWebBrowser(int sock1, int sock2);

private:

    std::unordered_map<int, int> m_sourceToDest;
    std::unordered_map<int, int> m_destToSource;
    
    std::unordered_map< int, std::vector<char> > m_storage;

    std::map<std::pair<int, int>, EndBodyParameters> m_sockEndBody;
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
    static_assert(is_char_vector<T>::value, "your type is not char vector daun");

    if(m_storage.find(descriptor) != m_storage.end()){ return false; }
    m_storage.insert( {descriptor, std::forward<T>(data)} );

    return true;
}

template<typename T>
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}


}
#endif
