#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_timer.h>

extern "C"
{
#include "mbedtls/rsa.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
}

const char *ssid = "---";
const char *password = "---";

const char *benchmarkUrl =
    "http://131.163.81.124:8000/benchmark";

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;

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

    const char *pers = "crypto_benchmark";

    mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char *)pers,
        strlen(pers));
}

void sendResult(
    const char *algorithm,
    const char *operation,
    int iterations,
    const Stats &s)
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
        "\"operation\":\"" + String(operation) + "\","
        "\"iterations\":" + String(iterations) + ","
        "\"avg_us\":" + String((uint32_t)s.avg) + ","
        "\"min_us\":" + String((uint32_t)s.min) + ","
        "\"max_us\":" + String((uint32_t)s.max) +
        "}";

    int code = http.POST(json);

    Serial.printf(
        "%s -> HTTP %d\n",
        algorithm,
        code);

    http.end();
}

Stats benchmarkECDH(int iterations)
{
    uint64_t total = 0;
    uint64_t minTime = UINT64_MAX;
    uint64_t maxTime = 0;

    for (int i = 0; i < iterations; i++)
    {
        mbedtls_ecdh_context ctx;

        mbedtls_ecdh_init(&ctx);

        uint64_t start = esp_timer_get_time();

        mbedtls_ecp_group_load(
            &ctx.grp,
            MBEDTLS_ECP_DP_SECP256R1);

        mbedtls_ecdh_gen_public(
            &ctx.grp,
            &ctx.d,
            &ctx.Q,
            mbedtls_ctr_drbg_random,
            &ctr_drbg);

        uint64_t end = esp_timer_get_time();

        uint64_t elapsed = end - start;

        total += elapsed;

        if (elapsed < minTime)
            minTime = elapsed;

        if (elapsed > maxTime)
            maxTime = elapsed;

        mbedtls_ecdh_free(&ctx);
    }

    return {
        total / iterations,
        minTime,
        maxTime};
}

Stats benchmarkX25519(int iterations)
{
    uint64_t total = 0;
    uint64_t minTime = UINT64_MAX;
    uint64_t maxTime = 0;

    for (int i = 0; i < iterations; i++)
    {
        mbedtls_ecdh_context ctx;

        mbedtls_ecdh_init(&ctx);

        uint64_t start = esp_timer_get_time();

        mbedtls_ecp_group_load(
            &ctx.grp,
            MBEDTLS_ECP_DP_CURVE25519);

        mbedtls_ecdh_gen_public(
            &ctx.grp,
            &ctx.d,
            &ctx.Q,
            mbedtls_ctr_drbg_random,
            &ctr_drbg);

        uint64_t end = esp_timer_get_time();

        uint64_t elapsed = end - start;

        total += elapsed;

        if (elapsed < minTime)
            minTime = elapsed;

        if (elapsed > maxTime)
            maxTime = elapsed;

        mbedtls_ecdh_free(&ctx);
    }

    return {
        total / iterations,
        minTime,
        maxTime};
}

Stats benchmarkRSA(int iterations)
{
    uint64_t total = 0;
    uint64_t minTime = UINT64_MAX;
    uint64_t maxTime = 0;

    for (int i = 0; i < iterations; i++)
    {
        mbedtls_rsa_context rsa;

        mbedtls_rsa_init(
            &rsa,
            MBEDTLS_RSA_PKCS_V15,
            0);

        uint64_t start = esp_timer_get_time();

        mbedtls_rsa_gen_key(
            &rsa,
            mbedtls_ctr_drbg_random,
            &ctr_drbg,
            2048,
            65537);

        uint64_t end = esp_timer_get_time();

        uint64_t elapsed = end - start;

        total += elapsed;

        if (elapsed < minTime)
            minTime = elapsed;

        if (elapsed > maxTime)
            maxTime = elapsed;

        mbedtls_rsa_free(&rsa);
    }

    return {
        total / iterations,
        minTime,
        maxTime};
}

void printStats(
    const char *name,
    const Stats &s)
{
    Serial.println();
    Serial.println(name);

    Serial.printf("AVG: %llu us\n", s.avg);
    Serial.printf("MIN: %llu us\n", s.min);
    Serial.printf("MAX: %llu us\n", s.max);
}

void setup()
{
    Serial.begin(115200);

    delay(3000);

    Serial.println();
    Serial.println("=== CRYPTO BENCHMARK ===");

    connectWiFi();

    initRandom();

    Stats ecdh = benchmarkECDH(20);

    printStats(
        "ECDH P-256",
        ecdh);

    sendResult(
        "ECDH_P256",
        "key_generation",
        20,
        ecdh);

    Stats x25519 = benchmarkX25519(20);

    printStats(
        "X25519",
        x25519);

    sendResult(
        "X25519",
        "key_generation",
        20,
        x25519);

    Stats rsa = benchmarkRSA(20);

    printStats(
        "RSA-2048",
        rsa);

    sendResult(
        "RSA_2048",
        "key_generation",
        20,
        rsa);

    Serial.println();
    Serial.println("BENCHMARK FINISHED");
}

void loop()
{
}