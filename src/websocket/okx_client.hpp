#pragma once

#include <libwebsockets.h>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../measurement/measurement.hpp"

// Forward declaration of callback function
typedef void (*MeasurementCallback)(const Measurement& measurement);

class OkxClient {
   private:
    std::vector<std::string> symbols;
    struct lws_context* context;
    struct lws* client_wsi;

    // Buffer for receiving data
    static char rx_buffer[16384];
    static int rx_buffer_len;

    static MeasurementCallback measurement_callback;
    void sendSubscription();

   public:
    OkxClient(const std::vector<std::string>& symbols);
    ~OkxClient();

    bool connect();

    void setCallback(MeasurementCallback callback);

    lws_context* getContext() const { return context; }

    bool isConnected() const { return client_wsi != nullptr; }

    static int wsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* user, void* in, size_t len);
};
