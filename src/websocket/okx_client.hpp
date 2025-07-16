#pragma once

#include <libwebsockets.h>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../measurement/measurement.hpp"

typedef struct {
    std::vector<std::string> symbols;
    struct lws_context* context;
    struct lws* client_wsi;
    bool subscription_confirmed;  // Add this field
} okx_client_t;

namespace OkxClient {

// Buffer for receiving data
extern char rx_buffer[16384];
extern int rx_buffer_len;

okx_client_t create(const std::vector<std::string>& symbols);
void destroy(okx_client_t& client);
bool connect(okx_client_t& client);
void sendSubscription(okx_client_t& client);
bool isConnected(const okx_client_t& client);
lws_context* getContext(const okx_client_t& client);
int wsCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user,
               void* in, size_t len);
int waitForSubscriptions(okx_client_t& client);

}  // namespace OkxClient
