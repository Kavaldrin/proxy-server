#include "ProxyManager.hpp"
#include <sys/socket.h>
#include "logger.h"
#include <algorithm>
#include <unistd.h>

using namespace Proxy;

constexpr bool SHOULD_CHANGE_SOCKET_STATE = true;
constexpr bool SHOULD_CLOSE_SOCKET = true;

std::pair<bool, bool> ProxyManager::handleStoredBuffers(int fd) noexcept
{

    if(m_storage.find(fd) == m_storage.end())
    {
        LoggerLogStatusErrorWithLineAndFile("fatal error in poll logic\n", 0);
        std::cout << fd << std::endl;
        return {SHOULD_CHANGE_SOCKET_STATE, false};
    }

    if(m_storage[fd].empty())
    {
        return {SHOULD_CHANGE_SOCKET_STATE, false};
    }

    int status = ::send(fd, m_storage[fd].front().data(), m_storage[fd].front().size(), MSG_DONTWAIT);

    if(status == -1)
    {
        if( ! (errno == EAGAIN || errno == EWOULDBLOCK) )
        {
            LoggerLogStatusErrorWithLineAndFile("unexpected sending error", status);
            m_storage.erase(m_storage.find(fd));
        }
        else
        {
            LoggerLogStatusErrorWithLineAndFile("'might happend' sending error", status);
        }
    }
    else if(status == (int)m_storage[fd].front().size())
    {

        m_storage[fd].pop_front();
        if(m_storage[fd].empty())
        {
            return {SHOULD_CHANGE_SOCKET_STATE, false};
        }
        return {!SHOULD_CHANGE_SOCKET_STATE, false};
    }
    else
    {
        LoggerLogStatusErrorWithLineAndFile("something very strangeXD", status);
    }

    if(auto secondSoc = getSecondSocketIfEstablishedConnection(fd); ! secondSoc.has_value())
    {
        return {SHOULD_CHANGE_SOCKET_STATE, true};
    }

    return {SHOULD_CHANGE_SOCKET_STATE, false};

}

bool ProxyManager::addEstablishedConnection(int source, int destination)
{
    return m_sourceToDest.insert( {source, destination} ).second && m_destToSource.insert( {destination, source} ).second;
}

bool ProxyManager::destroyEstablishedConnectionBySource(int source)
{
    auto searchResult = m_sourceToDest.find(source);
    if(searchResult != m_sourceToDest.end())
    {
        int dest = searchResult->second;
        m_sourceToDest.erase(searchResult);
        auto searchResult = m_destToSource.find(dest);
        if(searchResult != m_destToSource.end())
        {
            m_destToSource.erase(searchResult);
            return true;
        }

    }
    return false;
}

bool ProxyManager::destroyEstablishedConnectionByDestination(int destination)
{
    auto searchResult = m_destToSource.find(destination);
    if(searchResult != m_destToSource.end())
    {
        int src = searchResult->second;
        m_destToSource.erase(searchResult);
        auto searchResult = m_sourceToDest.find(src);
        if(searchResult != m_sourceToDest.end())
        {
            m_sourceToDest.erase(searchResult);
            return true;
        }
    }
    return false;
}


std::optional<int> ProxyManager::getSecondSocketIfEstablishedConnection(int socket)
{
    if(auto searchResult = m_sourceToDest.find(socket); searchResult != m_sourceToDest.end())
    {
        return searchResult->second;
    }
    
    if(auto searchResult = m_destToSource.find(socket); searchResult != m_destToSource.end())
    {
        return searchResult->second;
    }
    
    return {};
}

bool ProxyManager::isDestination(int socket)
{
    return (m_destToSource.find(socket) != m_destToSource.end());
}

bool ProxyManager::isSource(int socket)
{
    return (m_sourceToDest.find(socket) != m_sourceToDest.end());
}