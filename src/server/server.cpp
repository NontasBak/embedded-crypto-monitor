#include "server.hpp"

#include <chrono>
#include <iostream>
#include <sstream>

HTTPServer::HTTPServer(int port) : port_(port), running_(false) {}

HTTPServer::~HTTPServer() { stop(); }

void HTTPServer::start() {
    if (running_) {
        return;
    }

    setupRoutes();
    running_ = true;
    server_thread_ = std::thread(&HTTPServer::run, this);
    std::cout << "HTTP Server started on port " << port_ << std::endl;
}

void HTTPServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    server_.stop();
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    std::cout << "HTTP Server stopped" << std::endl;
}

void HTTPServer::setupRoutes() {
    // Enable CORS for all routes
    server_.set_post_routing_handler([](const httplib::Request& req,
                                        httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    });

    // Simple Moving Average endpoint
    server_.Get("/sma",
                [this](const httplib::Request& req, httplib::Response& res) {
                    handleSMA(req, res);
                });

    // Exponential Moving Average endpoint
    server_.Get("/ema",
                [this](const httplib::Request& req, httplib::Response& res) {
                    handleEMA(req, res);
                });

    // MACD endpoint
    server_.Get("/macd",
                [this](const httplib::Request& req, httplib::Response& res) {
                    handleMACD(req, res);
                });

    // Signal endpoint
    server_.Get("/signal",
                [this](const httplib::Request& req, httplib::Response& res) {
                    handleSignal(req, res);
                });

    // Distance endpoint
    server_.Get("/distance",
                [this](const httplib::Request& req, httplib::Response& res) {
                    handleDistance(req, res);
                });

    // Closing price endpoint
    server_.Get("/close",
                [this](const httplib::Request& req, httplib::Response& res) {
                    handleClosingPrice(req, res);
                });
}

void HTTPServer::run() { server_.listen("0.0.0.0", port_); }

void HTTPServer::handleSMA(const httplib::Request& req,
                           httplib::Response& res) {
    if (!validateParameters(req, {"symbol", "window"})) {
        res.status = 400;
        res.set_content(
            createErrorResponse("Missing symbol or window parameter"),
            "application/json");
        return;
    }

    try {
        std::string symbol = req.get_param_value("symbol");
        int window = std::stoi(req.get_param_value("window"));
        long timestamp = getCurrentTimestamp();

        value_t averages =
            DataCollector::getRecentAverages(symbol, timestamp, window);

        if (averages.values.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("No data found for symbol"),
                            "application/json");
            return;
        }

        value_t filteredAverages = filterDataPoints(averages);
        std::string json = valueToJson(filteredAverages);
        res.set_content(json, "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse("Invalid window parameter"),
                        "application/json");
    }
}

void HTTPServer::handleEMA(const httplib::Request& req,
                           httplib::Response& res) {
    if (!validateParameters(req, {"symbol", "window", "type"})) {
        res.status = 400;
        res.set_content(
            createErrorResponse("Missing symbol, window, or type parameter"),
            "application/json");
        return;
    }

    try {
        std::string symbol = req.get_param_value("symbol");
        int window = std::stoi(req.get_param_value("window"));
        std::string type = req.get_param_value("type");
        long timestamp = getCurrentTimestamp();

        value_t emas =
            DataCollector::getRecentEMA(symbol, timestamp, window, type);

        if (emas.values.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("No data found for symbol"),
                            "application/json");
            return;
        }

        value_t filteredEmas = filterDataPoints(emas);
        std::string json = valueToJson(filteredEmas);
        res.set_content(json, "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse("Invalid window parameter"),
                        "application/json");
    }
}

void HTTPServer::handleMACD(const httplib::Request& req,
                            httplib::Response& res) {
    if (!validateParameters(req, {"symbol", "window"})) {
        res.status = 400;
        res.set_content(
            createErrorResponse("Missing symbol or window parameter"),
            "application/json");
        return;
    }

    try {
        std::string symbol = req.get_param_value("symbol");
        int window = std::stoi(req.get_param_value("window"));
        long timestamp = getCurrentTimestamp();

        value_t macd = DataCollector::getRecentMACD(symbol, timestamp, window);

        if (macd.values.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("No data found for symbol"),
                            "application/json");
            return;
        }

        value_t filteredMacd = filterDataPoints(macd);
        std::string json = valueToJson(filteredMacd);
        res.set_content(json, "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse("Invalid window parameter"),
                        "application/json");
    }
}

void HTTPServer::handleSignal(const httplib::Request& req,
                              httplib::Response& res) {
    if (!validateParameters(req, {"symbol", "window"})) {
        res.status = 400;
        res.set_content(
            createErrorResponse("Missing symbol or window parameter"),
            "application/json");
        return;
    }

    try {
        std::string symbol = req.get_param_value("symbol");
        int window = std::stoi(req.get_param_value("window"));
        long timestamp = getCurrentTimestamp();

        value_t signals =
            DataCollector::getRecentSignal(symbol, timestamp, window);

        if (signals.values.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("No data found for symbol"),
                            "application/json");
            return;
        }

        value_t filteredSignals = filterDataPoints(signals);
        std::string json = valueToJson(filteredSignals);
        res.set_content(json, "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse("Invalid window parameter"),
                        "application/json");
    }
}

void HTTPServer::handleDistance(const httplib::Request& req,
                                httplib::Response& res) {
    if (!validateParameters(req, {"symbol", "window"})) {
        res.status = 400;
        res.set_content(
            createErrorResponse("Missing symbol or window parameter"),
            "application/json");
        return;
    }

    try {
        std::string symbol = req.get_param_value("symbol");
        int window = std::stoi(req.get_param_value("window"));
        long timestamp = getCurrentTimestamp();

        value_t distances =
            DataCollector::getRecentDistance(symbol, timestamp, window);

        if (distances.values.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("No data found for symbol"),
                            "application/json");
            return;
        }

        value_t filteredDistances = filterDataPoints(distances);
        std::string json = valueToJson(filteredDistances);
        res.set_content(json, "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse("Invalid window parameter"),
                        "application/json");
    }
}

void HTTPServer::handleClosingPrice(const httplib::Request& req,
                                    httplib::Response& res) {
    if (!validateParameters(req, {"symbol", "window"})) {
        res.status = 400;
        res.set_content(
            createErrorResponse("Missing symbol or window parameter"),
            "application/json");
        return;
    }

    try {
        std::string symbol = req.get_param_value("symbol");
        int window = std::stoi(req.get_param_value("window"));
        long timestamp = getCurrentTimestamp();

        value_t closingPrices =
            DataCollector::getRecentClosingPrices(symbol, timestamp, window);

        if (closingPrices.values.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("No data found for symbol"),
                            "application/json");
            return;
        }

        value_t filteredClosingPrices = filterDataPoints(closingPrices);
        std::string json = valueToJson(filteredClosingPrices);
        res.set_content(json, "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse("Invalid window parameter"),
                        "application/json");
    }
}

std::string HTTPServer::valueToJson(const value_t& data) {
    std::ostringstream json;
    json << "{\"values\": [";

    for (size_t i = 0; i < data.values.size(); ++i) {
        json << data.values[i];
        if (i < data.values.size() - 1) {
            json << ", ";
        }
    }

    json << "], \"timestamps\": [";

    for (size_t i = 0; i < data.timestamps.size(); ++i) {
        json << data.timestamps[i];
        if (i < data.timestamps.size() - 1) {
            json << ", ";
        }
    }

    json << "]}";
    return json.str();
}

std::string HTTPServer::createErrorResponse(const std::string& message) {
    std::ostringstream json;
    json << "{\"error\": \"" << message << "\"}";
    return json.str();
}

long HTTPServer::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch())
                         .count();
    long currentMinuteTimestamp = (timestamp / 60000) * 60000;
    return currentMinuteTimestamp;
}

bool HTTPServer::validateParameters(
    const httplib::Request& req,
    const std::vector<std::string>& required_params) {
    for (const auto& param : required_params) {
        if (!req.has_param(param.c_str())) {
            return false;
        }
    }
    return true;
}

value_t HTTPServer::filterDataPoints(const value_t& data, size_t maxPoints) {
    if (data.values.size() <= maxPoints) {
        return data;
    }

    value_t filtered;
    size_t step = data.values.size() / maxPoints;
    size_t remainder = data.values.size() % maxPoints;

    // Calculate indices to sample evenly across the dataset
    for (size_t i = 0; i < maxPoints; ++i) {
        size_t index = i * step;

        // Distribute the remainder points evenly
        if (i < remainder) {
            index += i;
        } else {
            index += remainder;
        }

        // Ensure we don't go out of bounds
        if (index >= data.values.size()) {
            index = data.values.size() - 1;
        }

        filtered.values.push_back(data.values[index]);
        filtered.timestamps.push_back(data.timestamps[index]);
    }

    return filtered;
}
