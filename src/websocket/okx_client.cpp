#include "okx_client.hpp"

#include <iostream>

#include "../measurement/measurement.hpp"

char OkxClient::rx_buffer[16384];
int OkxClient::rx_buffer_len = 0;

static okx_client_t* current_client = nullptr;

static const struct lws_protocols protocols[] = {
    {
        "okx-protocol",
        OkxClient::wsCallback,
        0,  // per_session_data_size
        16384,
    },
    {NULL, NULL, 0, 0}  // terminator
};

okx_client_t OkxClient::create(const std::vector<std::string>& symbols) {
    okx_client_t client;
    client.symbols = symbols;
    client.context = nullptr;
    client.client_wsi = nullptr;
    client.subscription_confirmed = false;
    return client;
}

void OkxClient::destroy(okx_client_t& client) {
    if (client.context) {
        lws_context_destroy(client.context);
        client.context = nullptr;
        client.client_wsi = nullptr;
    }
}

bool OkxClient::connect(okx_client_t& client) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    // Add these options for better connection stability
    info.retry_and_idle_policy = NULL;  // Use defaults
    // info.connect_timeout_secs = 30;     // Increase from default
    info.ka_time = 30;      // Keep-alive time in seconds
    info.ka_interval = 10;  // Keep-alive interval
    info.ka_probes = 3;     // Number of keep-alive probes

    client.context = lws_create_context(&info);
    if (!client.context) {
        std::cerr << "lws init failed" << std::endl;
        return false;
    }

    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof ccinfo);

    ccinfo.context = client.context;
    ccinfo.address = "ws.okx.com";
    ccinfo.port = 8443;
    ccinfo.path = "/ws/v5/public";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;

    // Add retry settings
    ccinfo.retry_and_idle_policy = NULL;  // Use defaults

    std::cout << "Connecting to OKX WebSocket..." << std::endl;
    client.client_wsi = lws_client_connect_via_info(&ccinfo);

    // Store the pointer after establishing connection attempt
    if (client.client_wsi != nullptr) {
        current_client = &client;
    }

    return client.client_wsi != nullptr;
}

void OkxClient::sendSubscription(okx_client_t& client) {
    // Create a JSON subscription message
    nlohmann::json subscription = nlohmann::json::object();
    nlohmann::json args = nlohmann::json::array();

    // Add each symbol to the subscription
    for (const std::string& symbol : client.symbols) {
        nlohmann::json arg = {{"channel", "trades"}, {"instId", symbol}};
        args.push_back(arg);
    }

    subscription["op"] = "subscribe";
    subscription["args"] = args;

    // Send the message via WebSocket
    std::string message = subscription.dump();

    unsigned char* buf = (unsigned char*)malloc(LWS_PRE + message.length());
    memcpy(&buf[LWS_PRE], message.c_str(), message.length());

    lws_write(client.client_wsi, &buf[LWS_PRE], message.length(),
              LWS_WRITE_TEXT);
    free(buf);
}

int OkxClient::wsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* user, void* in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "WebSocket connection established" << std::endl;
            if (current_client) {
                current_client->subscription_confirmed = false;
                sendSubscription(*current_client);
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            // Add the received data to our buffer
            memcpy(&rx_buffer[rx_buffer_len], in, len);
            rx_buffer_len += len;

            // Process the message if it's complete
            if (lws_is_final_fragment(wsi)) {
                rx_buffer[rx_buffer_len] = '\0';

                try {
                    nlohmann::json response = nlohmann::json::parse(rx_buffer);

                    if (response.contains("event") &&
                        response["event"] == "subscribe") {
                        if (current_client) {
                            current_client->subscription_confirmed = true;
                            // std::cout << "Subscription confirmed" <<
                            // std::endl;
                        }
                    } else if (response.contains("data")) {
                        response = response["data"][0];

                        measurement_t measurement = Measurement::create(
                            response["instId"],
                            std::stod(response["px"].get<std::string>()),
                            std::stod(response["sz"].get<std::string>()),
                            std::stol(response["ts"].get<std::string>()));

                        // Measurement::displayMeasurement(measurement);
                        Measurement::storeMeasurement(measurement);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing JSON: " << e.what()
                              << std::endl;
                }

                rx_buffer_len = 0;
            }
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "WebSocket connection closed" << std::endl;
            if (current_client) {
                current_client->client_wsi = nullptr;
            }
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "WebSocket connection error: ";
            if (in) {
                std::cerr << (char*)in;
            } else {
                std::cerr << "(no error message)";
            }
            std::cerr << std::endl;

            if (current_client) {
                current_client->client_wsi = nullptr;
            }
            break;

        case LWS_CALLBACK_WSI_DESTROY:
            std::cout << "WebSocket interface destroyed" << std::endl;
            break;

        default:
            break;
    }

    return 0;
}

bool OkxClient::isConnected(const okx_client_t& client) {
    return client.client_wsi != nullptr && client.subscription_confirmed;
}

lws_context* OkxClient::getContext(const okx_client_t& client) {
    return client.context;
}

int OkxClient::waitForSubscriptions(okx_client_t& client) {
    int wait_attempts = 0;
    const int max_wait_attempts = 100;  // 10 seconds total

    while (wait_attempts < max_wait_attempts && !isConnected(client)) {
        lws_service(getContext(client), 100);
        usleep(100 * 1000);  // 100ms
        wait_attempts++;
        // std::cout << "Waiting for subscriptions..." << std::endl;
    }

    if (isConnected(client)) {
        std::cerr << "Subscriptions confirmed" << std::endl;
        return 0;
    } else {
        std::cout << "Failed to create subscriptions" << std::endl;
        return 1;
    }
}
