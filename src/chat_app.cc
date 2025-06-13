// #include "NetworkManager.hpp"
// #include <asio/io_context.hpp>
// #include <thread>
// #include <iostream>
// #include <string>
// #include <memory>
// #include <atomic>

// // Simple helper to convert string to NetworkPacket (vector<uint8_t>)
// NetworkPacket makePacket(const std::string& msg) {
//     return NetworkPacket(msg.begin(), msg.end());
// }

// int main() {
//     asio::io_context io_context;
//     unsigned short localPort, remotePort;
//     std::string remoteIp;

//     std::cout << "Enter local UDP port to bind: ";
//     std::cin >> localPort;
//     std::cout << "Enter remote IP: ";
//     std::cin >> remoteIp;
//     std::cout << "Enter remote UDP port: ";
//     std::cin >> remotePort;
//     std::cin.ignore(); // flush newline

//     auto incomingQueue = std::make_shared<ThreadSafeQueue<NetworkPacket>>();
//     NetworkManager net(io_context);
//     net.setIncomingQueue(incomingQueue);

//     if (!net.init(localPort)) {
//         std::cerr << "Failed to initialize NetworkManager.\n";
//         return 1;
//     }
//     net.setRemoteEndpoint(remoteIp, remotePort);
//     net.startReceive();

//     std::atomic<bool> running{true};

//     // Thread to run io_context (for async network ops)
//     std::thread netThread([&]() { io_context.run(); });

//     // Thread to print incoming messages
//     std::thread recvThread([&]() {
//         while (running) {
//             NetworkPacket pkt;
//             if (incomingQueue->pop(pkt)) {
//                 std::string msg(pkt.begin(), pkt.end());
//                 std::cout << "\n[Peer] " << msg << "\n> " << std::flush;
//             }
//         }
//     });

//     std::cout << "Type messages to send. Type '/quit' to exit.\n";
//     std::string line;
//     while (running) {
//         std::cout << "> " << std::flush;
//         if (!std::getline(std::cin, line)) break;
//         if (line == "/quit") {
//             running = false;
//             break;
//         }
//         net.sendPacket(makePacket(line));
//     }

//     net.stop();
//     io_context.stop();
//     running = false;
//     if (netThread.joinable()) netThread.join();
//     if (recvThread.joinable()) recvThread.join();

//     std::cout << "Chat app exited.\n";
//     return 0;
// }
