#include "ProxyManager.hpp"
#include <sys/socket.h>
#include "logger.h"
#include <algorithm>
#include <unistd.h>

using namespace Proxy;


std::pair<bool, bool> ProxyManager::handleStoredBuffers(int fd) noexcept
{

    if(m_storage.find(fd) == m_storage.end())
    {
        return {false, false};
    }


    int status = ::send(fd, m_storage.at(fd).data(), m_storage.at(fd).size(), MSG_DONTWAIT);
    LoggerLogStatusErrorWithLineAndFile("send: ", status);
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
    else if(status == (int)m_storage[fd].size())
    {
        m_storage.erase(m_storage.find(fd));
        return {true, false};
    }
    else
    {
        LoggerLogStatusErrorWithLineAndFile("something very strangeXD", status);
    }


    if(auto secondSoc = getSecondSocketIfEstablishedConnection(fd); ! secondSoc.has_value())
    {
        //because we know there wont be more data to this socket
        std::cout << "Closing socket\n";
        ::close(fd);
        return {true, true};
    }


    return {false, false};

}

bool ProxyManager::addEstablishedConnection(int source, int destination)
{
    return m_sourceToDest.insert( {source, destination} ).second && m_destToSource.insert( {destination, source} ).second;
}

void ProxyManager::addEndBodyMethod(int source, int destination, HttpRequest_t headers)
{
    std::pair<std::string, std::string> header;
    auto header_it = headers.find("Content-Length");
    if(header_it == headers.end()) {
        header_it = headers.find("Transfer-Encoding");
        if(header_it == headers.end())
            header = std::make_pair(std::string("None"), std::string("None"));
        else header = *header_it;
    }
    else header = *header_it;
    std::cout << "end body method: " << header.first << " " << header.second << std::endl;

    m_sockEndBody.insert( {source, header} ).second;
    m_sockEndBody.insert( {destination, header} ).second;
}

std::optional<std::pair<std::string, std::string>> getEndBodyMethod(int socket) {

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