#include "../include/network_manager.hpp"
#include <asio/system_error.hpp>
#include <iostream>
#include <stdexcept>

NetworkManager::NetworkManager(asio::io_context& context) : m_Context(context), m_Socket(context), a_IsRunning(false)
{}

NetworkManager::~NetworkManager()
{
    stop(); // Close socket and release resources
}

bool NetworkManager::init(unsigned short localPort)
{
    try
    {
        m_Socket.open(asio::ip::udp::v4());
        m_Socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), localPort));
        a_IsRunning.store(true);
        std::cout << "[NetworkManager] UDP socket bound to port " << localPort << std::endl;
        return true;
    }
    catch (const asio::system_error& e)
    {
        std::cerr << "[NetworkManager] Error binding socket: " << e.what() << std::endl;
        return false;
    }
}

void NetworkManager::setRemoteEndpoint(const std::string& ipAddress, unsigned short port)
{
    try
    {
        m_Remote = asio::ip::udp::endpoint(asio::ip::address::from_string(ipAddress), port);
        std::cout << "[NetworkManager] Remote endpoint set to " << ipAddress << ":" << port << std::endl;
    }
    catch(const asio::system_error& e)
    {
        std::cerr << "[NetworkManager] Error setting remote endpoint: " << e.what() << std::endl;
    }
}
