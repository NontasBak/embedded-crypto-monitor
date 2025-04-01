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

// Handle Ctrl+C signal
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..."
              << std::endl;
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);

    Setup::initializeFiles();

    // Create the WebSocket client
    OkxClient client(SYMBOLS);

    // Create the scheduler for periodic tasks
    Scheduler scheduler;

    // Connect to the WebSocket server
    if (!client.connect()) {
        std::cerr << "Failed to connect to WebSocket" << std::endl;
        return 1;
    }

    scheduler.start();
    std::cout << "Crypto monitor is running. Press Ctrl+C to exit."
              << std::endl;

    // Main event loop
    while (running && client.isConnected()) {
        lws_service(client.getContext(), 100);
    }

    scheduler.stop();

    std::cout << "Crypto monitor has shut down." << std::endl;
    return 0;
}
