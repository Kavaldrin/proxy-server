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
    EndBodyParameters endBodyParams;

    auto header_it = headers.find("Content-Length");
    if(header_it != headers.end()) {
        endBodyParams.endBodyMethod = EndBodyMethod::CONTENT_LENGTH;
        endBodyParams.contentLength = std::stoi(header_it->second);
    }
    else if(header_it = headers.find("Transfer-Encoding");
            header_it != headers.end() and header_it->second == "chunked"){
        endBodyParams.endBodyMethod = EndBodyMethod::CHUNK;
    }
    else {
        endBodyParams.endBodyMethod = EndBodyMethod::NONE;
    }

    std::cout << "adding end body method: " << endBodyParams.endBodyMethod << std::endl;

    auto endBodyParams_it = m_sockEndBody.find({source, destination});
    if(endBodyParams_it != m_sockEndBody.end())
        endBodyParams_it->second = endBodyParams;
}

void ProxyManager::createEndBodyParams(int source, int destination) {
    m_sockEndBody.insert({{source, destination}, {}});
}

void ProxyManager::deleteEndBodyParams(int sock1, int sock2) {
    auto endBodyParams = getEndBodyParams(sock1, sock2);

    if(endBodyParams.has_value())
        m_sockEndBody.erase(*endBodyParams);
}

std::optional<std::_Rb_tree_iterator<std::pair<const std::pair<int, int>, Proxy::ProxyManager::EndBodyParameters>>> 
        ProxyManager::getEndBodyParams(int sock1, int sock2) {
    auto endBodyParams_it = m_sockEndBody.find({sock1, sock2});
    if(endBodyParams_it != m_sockEndBody.end())
        return std::make_optional(endBodyParams_it);
    
    endBodyParams_it = m_sockEndBody.find({sock2, sock1});
    if(endBodyParams_it != m_sockEndBody.end())
        return std::make_optional(endBodyParams_it);

    return std::nullopt;
}

void ProxyManager::incrementMessagesFromWebBrowser(int sock1, int sock2) {
    auto endBodyParams = getEndBodyParams(sock1, sock2);

    if(endBodyParams.has_value())
        (*endBodyParams)->second.numMessagesFromWebBrowser++;
}

void ProxyManager::incrementMessagesFromServer(int sock1, int sock2) {
    auto endBodyParams = getEndBodyParams(sock1, sock2);

    if(endBodyParams.has_value())
        (*endBodyParams)->second.numMessagesFromServer++;
}

bool ProxyManager::destroyEstablishedConnectionBySource(int source)
{
    auto searchResult = m_sourceToDest.find(source);
    if(searchResult != m_sourceToDest.end())
    {
        int dest = searchResult->second;
        deleteEndBodyParams(source, dest);

        m_sourceToDest.erase(searchResult);
        auto searchResult = m_destToSource.find(dest);
        if(searchResult != m_destToSource.end())
        {
            m_destToSource.erase(searchResult);
            std::cout << "destroy: " << source << " " << dest << std::endl;
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
        deleteEndBodyParams(src, destination);

        m_destToSource.erase(searchResult);
        auto searchResult = m_sourceToDest.find(src);
        if(searchResult != m_sourceToDest.end())
        {
            m_sourceToDest.erase(searchResult);
            std::cout << "destroy: " << src << " " << destination << std::endl;
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