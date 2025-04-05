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
        std::cerr << "Failed to connect to WebSocket" << std::endl;
        return 1;
    }

    Scheduler::start(*scheduler);
    std::cout << "Crypto monitor is running. Press Ctrl+C to exit."
              << std::endl;

    // Main event loop
    while (running && OkxClient::isConnected(client)) {
        lws_service(OkxClient::getContext(client), 100);
    }

    Scheduler::stop(*scheduler);
    OkxClient::destroy(client);

    std::cout << "Crypto monitor has shut down." << std::endl;
    return 0;
}
