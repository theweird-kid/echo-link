#include "NetworkManager.hpp"
#include <asio/system_error.hpp>
#include <iostream>

NetworkManager::NetworkManager(asio::io_context& context)
    : m_Context(context), m_Socket(context), a_IsRunning(false)
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

void NetworkManager::setIncomingQueue(std::shared_ptr<ThreadSafeQueue<NetworkPacket>> queue)
{
    m_IncomingQueue = queue;
    if (!m_IncomingQueue) {
        std::cerr << "[NetworkManager] Warning: Incoming queue set to nullptr." << std::endl;
    }
}

void NetworkManager::sendPacket(const NetworkPacket& packet)
{
    if(!a_IsRunning.load() || !m_Socket.is_open()) {
        std::cerr << "[NetworkManager] Error sending packet: Manager not running or Socket not open." << std::endl;
        return;
    }

    // Create a shared_ptr to the packet data. This ensures the data buffer
    // remains valid until the asynchronous send operation completes.
    // Asio's async functions typically require the buffer to be stable.
    std::shared_ptr<NetworkPacket> packet_ptr = std::make_shared<NetworkPacket>(packet);

    m_Context.post([this, packet_ptr](){
        if(!m_Socket.is_open()) {
            return;
        }

        m_Socket.async_send_to(asio::buffer(*packet_ptr),
            m_Remote,
            std::bind(&NetworkManager::handleSend, this,
                std::placeholders::_1, std::placeholders::_2, packet_ptr));
    });

}

void NetworkManager::startReceive() {
    if (!a_IsRunning.load()) {
        std::cerr << "[NetworkManager] Cannot start receive: Manager not running." << std::endl;
        return;
    }
    if (!m_IncomingQueue) {
        std::cerr << "[NetworkManager] Incoming queue not set. Receive will not store data." << std::endl;
        // Still allow receive, but warn
    }
    if (!m_Socket.is_open()) {
        std::cerr << "[NetworkManager] Socket not open for receiving. Cannot start receive." << std::endl;
        return;
    }

    // Start an asynchronous receive operation. The callback `handleReceive` will be called
    // when data arrives or an error occurs.
    m_Socket.async_receive_from(
        asio::buffer(m_RecvBuffer), // Buffer to store incoming data
        m_SenderEndpoint,           // To store the sender's endpoint
        std::bind(&NetworkManager::handleReceive, this,
                  std::placeholders::_1, std::placeholders::_2));
    std::cout << "[NetworkManager] Waiting for incoming UDP packets..." << std::endl;
}


// CALLBACKS TO HANDLE RECEIVE AND SEND OPERATIONS

void NetworkManager::handleReceive(const asio::error_code& error, std::size_t bytesRecieved)
{
    if(!error) {
        if(m_IncomingQueue) {
            m_IncomingQueue->push(NetworkPacket(m_RecvBuffer.data(), m_RecvBuffer.data() + bytesRecieved));
        }

        // Immediately start another receive operation to keep listening for more data.
        // This forms a continuous receive loop.
        if (a_IsRunning.load()) { // Only continue if not shutting down
            startReceive();
        }
    } else if(error == asio::error::operation_aborted) {
        std::cout << "[NetworkManager] Receive operation aborted." << std::endl;
    } else {
        std::cerr << "[NetworkManager] Error receiving data: " << error.message() << std::endl;

        if (a_IsRunning.load()) { // If not shutting down, try to restart receive
            startReceive();
        }
    }
}

void NetworkManager::handleSend(const asio::error_code& error, std::size_t bytesTransferred, std::shared_ptr<NetworkPacket> packet_ptr) {
    // The `packet_ptr` shared_ptr ensures the `NetworkPacket` data remains valid
    // until this handler is executed, then it will be automatically released.
    if (!error) {
        // Packet sent successfully!
        std::cout << "[NetworkManager] Sent " << bytesTransferred << " bytes to "<< m_Remote << std::endl;
    } else if (error == asio::error::operation_aborted) {
        // Send operation cancelled, e.g., socket closed during shutdown.
        std::cout << "[NetworkManager] Send operation aborted (socket closed)." << std::endl;
    } else {
        // Other send errors
        std::cerr << "[NetworkManager] Error on send: " << error.message() << std::endl;
    }
}

void NetworkManager::stop() {
    if (a_IsRunning.load())
    {
        a_IsRunning.store(false); // Set flag to indicate shutdown
        if (m_Socket.is_open())
        {
            asio::error_code ec;
            m_Socket.cancel(ec); // Cancel any pending async operations
            m_Socket.close(ec);  // Close the socket
            if (ec) {
                std::cerr << "[NetworkManager] Error closing socket: " << ec.message() << std::endl;
            } else {
                std::cout << "[NetworkManager] Socket closed." << std::endl;
            }
        }
    }
}
