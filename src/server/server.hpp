#pragma once

#include <httplib.h>

#include <atomic>
#include <string>
#include <thread>

#include "../data_collector/data_collector.hpp"

class HTTPServer {
   public:
    HTTPServer(int port = 8080);
    ~HTTPServer();

    void start();
    void stop();

   private:
    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    httplib::Server server_;

    void setupRoutes();
    void run();

    // Endpoint handlers
    void handleSMA(const httplib::Request& req, httplib::Response& res);
    void handleEMA(const httplib::Request& req, httplib::Response& res);
    void handleMACD(const httplib::Request& req, httplib::Response& res);
    void handleSignal(const httplib::Request& req, httplib::Response& res);
    void handleDistance(const httplib::Request& req, httplib::Response& res);
    void handleClosingPrice(const httplib::Request& req,
                            httplib::Response& res);

    // Utility functions
    std::string valueToJson(const value_t& data);
    std::string createErrorResponse(const std::string& message);
    long getCurrentTimestamp();
    bool validateParameters(const httplib::Request& req,
                            const std::vector<std::string>& required_params);
    value_t filterDataPoints(const value_t& data, size_t maxPoints = 200);
};
