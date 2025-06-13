#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <vector>

#include "thread_safe_queue.hpp"

using NetworkPacket = std::vector<char>;

class NetworkManager
{
public:
    explicit NetworkManager(asio::io_context& io_context);

    ~NetworkManager();

    // Initialize the UDP socket and bind it to a local port
    // returns true on success, false on failure
    bool init(unsigned short localPort);

    // Sets the IP Address and port for the remote peer to senb data to
    void setRemoteEndpoint(const std::string& ipAddress, unsigned short port);

    // Sets the queue to recieve Network Packets into
    void setIncomingQueue(std::shared_ptr<ThreadSafeQueue<NetworkPacket>> queue);

    // [ASYNC] send a network packet asynchronously
    // This method will push the packet to an internal queue and then initiate an async send.
    // It's designed to be called by a dedicated "network send thread" in VoiceChatApplication.
    void sendPacket(const NetworkPacket& data);

    // [ASYNC] Starts the asynchronous receive operations.
    // Call this once after initialization to begin listening for incoming data.
    void startReceive();

    // Gracefully stops all network operations and closes the socket.
    // Should be called during application shutdown.
    void stop();

private:
    asio::io_context& m_Context;        // Reference to the application's IO context
    asio::ip::udp::socket m_Socket;     // UDP socket

    asio::ip::udp::endpoint m_Remote;   // Remote endpoint to send packets to

    std::array<char, 2048> m_RecvBuffer; // temporary buffer for ASYNC recieve operations

    asio::ip::udp::endpoint m_SenderEndpoint; // Endpoint of the sender for recieved packets

    std::shared_ptr<ThreadSafeQueue<NetworkPacket>> m_IncomingQueue;    // Queue to store incoming packets from the network

    // --- [ASYNC] Callbacks ---
    // Callback for when an asynchronous receive operation completes
    void handleReceive(const asio::error_code& error, std::size_t bytesTransferred);

    // Callback for when an asynchronous send operation completes
    // The `std::shared_ptr<NetworkPacket> packet_ptr` ensures the data stays alive
    // until the send operation is finished.
    void handleSend(const asio::error_code& error, std::size_t bytesTransferred, std::shared_ptr<NetworkPacket> packet_ptr);

    // Flag to indicate if the manager is actively running/receiving
    std::atomic_bool a_IsRunning;
};

#endif // NETWORK_MANAGER_HPP
