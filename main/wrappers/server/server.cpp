#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "server.h"
#include "img_converters.h"

uint8_t *Server::imageBuff = (uint8_t*)malloc(sizeof(uint8_t));;
httpd_handle_t Server::server = nullptr;

bool Server::isBusy = false;
esp_err_t Server::handleRoot(httpd_req_t *req)
{
    Server::isBusy = true;
    const char *resp_str = R"(
    <html>
        <body>
            <h1>ESP32-CAM</h1>
            <img id="currentImage" src='/current.jpg'/>
            <script>
                function updateImage() {
                    var imgElement = document.getElementById("currentImage");
                    imgElement.src = "/current.jpg?" + new Date().getTime();
                }
                // Update the image every 3000 milliseconds
                setInterval(updateImage, 3000);
            </script>
        </body>
    </html>
)";

    httpd_resp_send(req, resp_str, strlen(resp_str));
    Server::isBusy = false;
    return ESP_OK;
}

esp_err_t Server::getCurrentImage(httpd_req_t *req)
{
    Server::isBusy = true;
    if (Server::imageBuff == nullptr)
    {
        // send no face detected text
        httpd_resp_send(req, "No face detected", strlen("No face detected"));
        Server::isBusy = false;
        return ESP_OK;
    }
    // set content type
    httpd_resp_set_type(req, "image/jpeg");

    // rgb565 to jpeg
    size_t out_len;
    uint8_t *out_buf;
    fmt2jpg((uint8_t *)Server::imageBuff, sizeof(Server::imageBuff), 320, 240, PIXFORMAT_RGB565,  60, &out_buf, &out_len);
    httpd_resp_send(req, (const char *)out_buf, out_len);
    free(out_buf);
    Server::isBusy = false;
    return ESP_OK;
}

void Server::startServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&Server::server, &config) == ESP_OK)
    {
        // Set URI handlers
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = Server::handleRoot,
            .user_ctx = nullptr};

        httpd_uri_t jpg_uri = {
            .uri = "/current.jpg",
            .method = HTTP_GET,
            .handler = Server::getCurrentImage,
            .user_ctx = nullptr};

        httpd_register_uri_handler(Server::server, &root_uri);
        httpd_register_uri_handler(Server::server, &jpg_uri);
    }

    ESP_LOGI(TAG, "Web server started");
}

const char *Server::TAG = "server";
