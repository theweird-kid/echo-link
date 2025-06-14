#include "Application.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Usage: ./VoiceChatApp <mode> <frame_size_samples> [local_port] [remote_ip] [remote_port]
    // Mode options: --loopback (local mic test), --network (P2P network chat)

    if (argc < 3) {
        std::cerr << "Usage for Live Mic Loopback: " << argv[0] << " --loopback <frame_size_samples>" << std::endl;
        std::cerr << "Usage for Network Chat: " << argv[0] << " --network <local_port> <remote_ip> <remote_port> <frame_size_samples>" << std::endl;
        std::cerr << "       (For network, microphone is always used. Specify 'self' for remote_ip to test self-connection)" << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  Live mic loopback:   " << argv[0] << " --loopback 480" << std::endl;
        std::cerr << "  Network client 1:    " << argv[0] << " --network 12345 127.0.0.1 54321 480" << std::endl;
        std::cerr << "  Network client 2:    " << argv[0] << " --network 54321 127.0.0.1 12345 480" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    bool enableNetworking = false;
    int frameSize = 0;
    unsigned short localPort = 0;
    std::string remoteIp = "";
    unsigned short remotePort = 0;

    // Common audio parameters (Opus preferred)
    const int sampleRate = 48000;
    const int channels = 2; // Stereo

    try {
        if (mode == "--loopback") {
            if (argc != 3) { // Expecting mode and frame_size
                std::cerr << "Error: Incorrect arguments for loopback mode." << std::endl;
                return 1;
            }
            frameSize = std::stoi(argv[2]);
            enableNetworking = false; // No networking in loopback mode
            std::cout << "Running in LOCAL MIC LOOPBACK mode." << std::endl;
        } else if (mode == "--network") {
            if (argc != 6) { // Expecting mode, local_port, remote_ip, remote_port, frame_size
                std::cerr << "Error: Incorrect arguments for network mode." << std::endl;
                return 1;
            }
            enableNetworking = true;
            localPort = std::stoi(argv[2]);
            remoteIp = argv[3];
            remotePort = std::stoi(argv[4]);
            frameSize = std::stoi(argv[5]);
            std::cout << "Running in NETWORK CHAT mode." << std::endl;
        } else {
            std::cerr << "Invalid mode: " << mode << std::endl;
            return 1;
        }

        Application app(sampleRate, channels, frameSize,
            enableNetworking, localPort, remoteIp, remotePort);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
