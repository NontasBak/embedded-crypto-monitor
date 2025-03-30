#include <libwebsockets.h>

#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

const std::vector<std::string> SYMBOLS = {"BTC-USDT",  "ADA-USDT", "ETH-USDT",
                                          "DOGE-USDT", "XRP-USDT", "SOL-USDT",
                                          "LTC-USDT",  "BNB-USDT"};

static struct lws_context* context;
static struct lws* client_wsi;
static int interrupted = 0;

#define MAX_PAYLOAD 16384
static char rx_buffer[MAX_PAYLOAD];
static int rx_buffer_len = 0;

struct Measurement {
    std::string instId;
    double px;
    double sz;
    long ts;
};

static int callback_okx(struct lws* wsi, enum lws_callback_reasons reason,
                        void* user, void* in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            std::cout << "Websocket connection established" << std::endl;

            json subscription = json::object();
            json args = json::array();

            for (const std::string& symbol : SYMBOLS) {
                json arg = {{"channel", "trades"}, {"instId", symbol}};
                args.push_back(arg);
            }

            subscription["op"] = "subscribe";
            subscription["args"] = args;

            std::string message = subscription.dump();

            unsigned char* buf =
                (unsigned char*)malloc(LWS_PRE + message.length());
            memcpy(&buf[LWS_PRE], message.c_str(), message.length());

            lws_write(wsi, &buf[LWS_PRE], message.length(), LWS_WRITE_TEXT);
            free(buf);
            break;
        }

        case LWS_CALLBACK_CLIENT_RECEIVE:
            memcpy(&rx_buffer[rx_buffer_len], in, len);
            rx_buffer_len += len;

            if (lws_is_final_fragment(wsi)) {
                rx_buffer[rx_buffer_len] = '\0';

                try {
                    json response = json::parse(rx_buffer);

                    if (response.contains("data")) {
                        response = response["data"][0];

                        Measurement measurement = {
                            response["instId"],
                            std::stod(response["px"].get<std::string>()),
                            std::stod(response["sz"].get<std::string>()),
                            std::stol(response["ts"].get<std::string>())};

                        std::cout << measurement.instId << " " << measurement.px
                                  << " " << measurement.sz << " "
                                  << measurement.ts << std::endl;
                        // addMeasurement(measurement);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing JSON: " << e.what()
                              << std::endl;
                }
            }
            rx_buffer_len = 0;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "Websocket connection closed" << std::endl;
            client_wsi = nullptr;
            break;

        default:
            break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        "okx-protocol",
        callback_okx,
        0,  // per_session_data_size
        MAX_PAYLOAD,
    },
    {NULL, NULL, 0, 0}  // terminator
};

int main() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    context = lws_create_context(&info);
    if (!context) {
        std::cerr << "lws init failed" << std::endl;
        return 1;
    }

    // Setup connection details
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof ccinfo);

    ccinfo.context = context;
    ccinfo.address = "ws.okx.com";
    ccinfo.port = 8443;
    ccinfo.path = "/ws/v5/public";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;

    std::cout << "Connecting to OKX WebSocket..." << std::endl;
    client_wsi = lws_client_connect_via_info(&ccinfo);
    if (!client_wsi) {
        std::cerr << "WebSocket connection failed" << std::endl;
        return 1;
    }

    // Start a thread to run tasks every minute
    std::thread minuteTask([]() {
        while (!interrupted) {
            // Get current time
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm* now_tm = std::localtime(&now_time_t);

            // Calculate time until next minute
            int seconds_to_next_minute = 60 - now_tm->tm_sec;
            if (seconds_to_next_minute == 60) seconds_to_next_minute = 0;

            std::this_thread::sleep_for(
                std::chrono::seconds(seconds_to_next_minute));

            // Run tasks at the start of each minute
            // runEveryMinute();
        }
    });

    // Main event loop
    while (!interrupted) {
        lws_service(context, 100);
    }

    // Clean up
    minuteTask.join();
    lws_context_destroy(context);

    return 0;
}
