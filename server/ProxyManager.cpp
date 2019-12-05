#include "ProxyManager.hpp"
#include <sys/socket.h>
#include "logger.h"
#include <algorithm>

using namespace Proxy;


void ProxyManager::handleStoredBuffers() noexcept
{
    std::vector< int > socketsToErase;

    for(auto&[socket, buffer] : m_storage)
    {
        //licze na to ze nie przekroczy kernelowskiego sajzu na wyslanie pakietu
        int status = ::send(socket, buffer.data(), buffer.size(), MSG_DONTWAIT);
        if(status == -1)
        {
            if( ! (errno == EAGAIN || errno == EWOULDBLOCK) )
            {
                LoggerLogStatusErrorWithLineAndFile("unexpected sending error", status);
                socketsToErase.push_back(socket);
            }
            else
            {
                LoggerLogStatusErrorWithLineAndFile("'might happend' sending error", status);
            }
        }
        else if(status == buffer.size())
        {
            socketsToErase.push_back(socket);
        }
        else
        {
            LoggerLogStatusErrorWithLineAndFile("something very strangeXD", status);
        }

    }

    std::for_each(socketsToErase.begin(), socketsToErase.end(),
        [&](const auto& sock)
        {
            auto searchRes = m_storage.find(sock);
            if(searchRes != m_storage.end()){ m_storage.erase(searchRes); }
        });
    
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