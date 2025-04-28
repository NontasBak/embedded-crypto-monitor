#include <unistd.h>  // For usleep

#include <csignal>
#include <iostream>
#include <vector>

#include "scheduler/scheduler.hpp"
#include "utils/setup.hpp"
#include "websocket/okx_client.hpp"

const std::vector<std::string> SYMBOLS = {"BTC-USDT",  "ADA-USDT", "ETH-USDT",
                                          "DOGE-USDT", "XRP-USDT", "SOL-USDT",
                                          "LTC-USDT",  "BNB-USDT"};

static bool running = true;
static okx_client_t* client_ptr = nullptr;

// Handle Ctrl+C signal
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..."
              << std::endl;
    running = false;
    exit(0);
}

int main() {
    signal(SIGINT, signalHandler);

    Setup::initializeFiles();

    // Create the WebSocket client
    okx_client_t client = OkxClient::create(SYMBOLS);
    client_ptr = &client;

    // Create the scheduler for periodic tasks
    scheduler_t* scheduler = Scheduler::create(SYMBOLS);

    // Connect to the WebSocket server
    if (!OkxClient::connect(client)) {
        std::cerr << "Failed to initial connection to WebSocket" << std::endl;
    }

    Scheduler::start(*scheduler);
    std::cout << "Crypto monitor is running. Press Ctrl+C to exit."
              << std::endl;

    // Main event loop with reconnection logic
    // since the connection is pretty unstable
    int reconnect_attempts = 0;
    const int max_reconnects = 10;
    const int reconnect_delay_ms = 5000;  // 5 seconds

    while (running) {
        if (!OkxClient::isConnected(client)) {
            if (reconnect_attempts < max_reconnects) {
                std::cout << "Connection lost. Attempting to reconnect ("
                          << reconnect_attempts + 1 << "/" << max_reconnects
                          << ")..." << std::endl;

                // Wait before reconnecting
                usleep(reconnect_delay_ms * 1000);

                if (OkxClient::connect(client)) {
                    std::cout << "Reconnection successful" << std::endl;
                    reconnect_attempts = 0;
                } else {
                    reconnect_attempts++;
                    std::cerr << "Reconnection attempt failed" << std::endl;
                    continue;
                }
            } else {
                std::cerr << "Maximum reconnection attempts reached. Exiting."
                          << std::endl;
                break;
            }
        }

        if (OkxClient::isConnected(client)) {
            lws_service(OkxClient::getContext(client), 100);
        }
    }

    Scheduler::stop(*scheduler);
    OkxClient::destroy(client);

    std::cout << "Crypto monitor has shut down." << std::endl;
    return 0;
}
