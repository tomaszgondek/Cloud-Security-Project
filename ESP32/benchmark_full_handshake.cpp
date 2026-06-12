#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_timer.h>

extern "C"
{
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
}

const char *ssid = "141D84";
const char *password = "YugoKoral45";

const char *handshakeUrlP256 =
    "http://131.163.81.124:8000/ecdh/p256";

const char *handshakeUrlX25519 =
    "http://131.163.81.124:8000/ecdh/x25519";

const char *benchmarkUrl =
    "http://131.163.81.124:8000/benchmark";

mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;

struct Stats
{
    uint64_t avg;
    uint64_t min;
    uint64_t max;
};

void connectWiFi()
{
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected");
}

void initRandom()
{
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    const char *pers = "full_handshake";

    mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char *)pers,
        strlen(pers));
}

String bytesToHex(
    const unsigned char *data,
    size_t len)
{
    String result;
    char buf[3];

    for (size_t i = 0; i < len; i++)
    {
        sprintf(buf, "%02x", data[i]);
        result += buf;
    }

    return result;
}

void hexToBytes(
    const String &hex,
    uint8_t *out,
    size_t &outLen)
{
    outLen = hex.length() / 2;

    for (size_t i = 0; i < outLen; i++)
    {
        String byteString =
            hex.substring(i * 2, i * 2 + 2);

        out[i] =
            strtoul(
                byteString.c_str(),
                nullptr,
                16);
    }
}

void sendBenchmark(
    const char *algorithm,
    const Stats &s,
    int iterations)
{
    HTTPClient http;

    http.begin(benchmarkUrl);

    http.addHeader(
        "Content-Type",
        "application/json");

    String json =
        "{"
        "\"device\":\"ESP32-CAM\","
        "\"algorithm\":\"" + String(algorithm) + "\","
        "\"operation\":\"full_handshake\","
        "\"iterations\":" + String(iterations) + ","
        "\"avg_us\":" + String((uint32_t)s.avg) + ","
        "\"min_us\":" + String((uint32_t)s.min) + ","
        "\"max_us\":" + String((uint32_t)s.max) +
        "}";

    int code = http.POST(json);

    Serial.printf(
        "Benchmark upload HTTP=%d\n",
        code);

    http.end();
}

Stats benchmarkHandshake(
    const char *name,
    const char *url,
    mbedtls_ecp_group_id groupId,
    int iterations)
{
    uint64_t total = 0;
    uint64_t minTime = UINT64_MAX;
    uint64_t maxTime = 0;

    Serial.println();
    Serial.println(name);

    for (int i = 0; i < iterations; i++)
    {
        uint64_t start =
            esp_timer_get_time();

        mbedtls_ecdh_context ctx;
        mbedtls_ecdh_init(&ctx);

        int ret =
            mbedtls_ecp_group_load(
                &ctx.grp,
                groupId);

        if (ret != 0)
        {
            Serial.printf(
                "group_load=%d\n",
                ret);
            continue;
        }

        ret =
            mbedtls_ecdh_gen_public(
                &ctx.grp,
                &ctx.d,
                &ctx.Q,
                mbedtls_ctr_drbg_random,
                &ctr_drbg);

        if (ret != 0)
        {
            Serial.printf(
                "gen_public=%d\n",
                ret);
            continue;
        }

        unsigned char publicKey[100];
        size_t publicKeyLen = 0;

        ret =
            mbedtls_ecp_point_write_binary(
                &ctx.grp,
                &ctx.Q,
                MBEDTLS_ECP_PF_UNCOMPRESSED,
                &publicKeyLen,
                publicKey,
                sizeof(publicKey));

        if (ret != 0)
        {
            Serial.printf(
                "write_binary=%d\n",
                ret);
            continue;
        }

        String publicKeyHex =
            bytesToHex(
                publicKey,
                publicKeyLen);

        HTTPClient http;

        http.begin(url);

        http.addHeader(
            "Content-Type",
            "application/json");

        String json =
            "{\"public_key\":\"" +
            publicKeyHex +
            "\"}";

        int code =
            http.POST(json);

        if (code != 200)
        {
            Serial.printf(
                "HTTP=%d\n",
                code);

            http.end();
            continue;
        }

        String response =
            http.getString();

        http.end();

        DynamicJsonDocument doc(1024);

        deserializeJson(
            doc,
            response);

        String serverPublicHex =
            doc["public_key"]
                .as<String>();

        uint8_t serverKeyBytes[100];
        size_t serverKeyLen;

        hexToBytes(
            serverPublicHex,
            serverKeyBytes,
            serverKeyLen);

        mbedtls_ecp_point Qp;
        mbedtls_ecp_point_init(
            &Qp);

        ret =
            mbedtls_ecp_point_read_binary(
                &ctx.grp,
                &Qp,
                serverKeyBytes,
                serverKeyLen);

        Serial.printf(
            "read_binary=%d\n",
            ret);

        if (ret != 0)
        {
            mbedtls_ecp_point_free(
                &Qp);

            mbedtls_ecdh_free(
                &ctx);

            continue;
        }

        mbedtls_mpi shared;
        mbedtls_mpi_init(
            &shared);

        ret =
            mbedtls_ecdh_compute_shared(
                &ctx.grp,
                &shared,
                &Qp,
                &ctx.d,
                mbedtls_ctr_drbg_random,
                &ctr_drbg);

        Serial.printf(
            "compute_shared=%d\n",
            ret);

        if (ret != 0)
        {
            mbedtls_mpi_free(
                &shared);

            mbedtls_ecp_point_free(
                &Qp);

            mbedtls_ecdh_free(
                &ctx);

            continue;
        }

        uint64_t end =
            esp_timer_get_time();

        uint64_t elapsed =
            end - start;

        total += elapsed;

        if (elapsed < minTime)
            minTime = elapsed;

        if (elapsed > maxTime)
            maxTime = elapsed;

        mbedtls_mpi_free(
            &shared);

        mbedtls_ecp_point_free(
            &Qp);

        mbedtls_ecdh_free(
            &ctx);

        Serial.printf(
            "Iteration %d/%d -> %llu us\n",
            i + 1,
            iterations,
            elapsed);
    }

    return {
        total / iterations,
        minTime,
        maxTime};
}

void setup()
{
    Serial.begin(115200);

    delay(3000);

    connectWiFi();
    initRandom();

    Stats p256 =
        benchmarkHandshake(
            "=== ECDH P256 FULL HANDSHAKE ===",
            handshakeUrlP256,
            MBEDTLS_ECP_DP_SECP256R1,
            20);

    Serial.printf(
        "\nP256 AVG=%llu us\n",
        p256.avg);

    sendBenchmark(
        "ECDH_P256",
        p256,
        20);

    Stats x25519 =
        benchmarkHandshake(
            "=== X25519 FULL HANDSHAKE ===",
            handshakeUrlX25519,
            MBEDTLS_ECP_DP_CURVE25519,
            20);

    Serial.printf(
        "\nX25519 AVG=%llu us\n",
        x25519.avg);

    sendBenchmark(
        "X25519",
        x25519,
        20);

    Serial.println();
    Serial.println(
        "BENCHMARK FINISHED");
}

void loop()
{
}