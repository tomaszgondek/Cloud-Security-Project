# Lightweight ECC Cryptographic Protocols for Cloud-Based IoT Systems

This final project explores lightweight cryptographic protocols based on
elliptic-curve cryptography (ECC). Its goal is to evaluate whether ECC is
suitable for secure data sharing between resource-constrained IoT devices and
cloud services.

The repository contains an implementation of communication between an ESP32-CAM
device and a server running on an Azure virtual machine. It also includes
benchmarks comparing:

- ECDH P-256,
- X25519,
- RSA-2048 as a reference algorithm.

## Project Goals

IoT devices have limited processing power, memory, and energy resources. This
project investigates whether ECC algorithms can provide secure key exchange at
a lower computational cost than traditional RSA.

The analyzed operations include:

- key-pair generation,
- shared-secret calculation,
- a full handshake between the ESP32-CAM and the server,
- average, minimum, and maximum execution-time measurements.

All benchmark results are measured in microseconds.

## Architecture

```text
ESP32-CAM                         Azure VM
+-------------------+            +---------------------------+
| mbedTLS           |   HTTP     | FastAPI                   |
| P-256 / X25519    |----------->| ECDH key exchange         |
| time measurements |            | benchmark result receiver |
| result uploads    |----------->| CSV result storage        |
+-------------------+            +---------------------------+
```

The device generates a cryptographic key pair and sends its public key to the
API. The server generates its own key pair, calculates the shared secret, and
returns its public key. The ESP32 completes the exchange, measures the operation
time, and uploads the resulting statistics to the server.

> The current implementation is intended for performance research. HTTP
> communication does not provide transport security or authenticate either
> party. Do not use it in production without additional security measures.

## Repository Structure

```text
.
|-- Azure VM/
|   |-- benchmark_full.py             # FastAPI server and ECDH endpoints
|   |-- main_benchmarks.py            # API for saving results to CSV
|   |-- keygen_benchmark.py            # key-generation benchmark
|   `-- benchmark_shared_secrets.py    # shared-secret benchmark
`-- ESP32/
    |-- benchmark_full_handshake.cpp   # full ESP32 <-> Azure VM exchange
    |-- keygen_benchmark.cpp           # key-generation benchmark
    `-- shared_secret.cpp              # cryptographic operation benchmark
```

## Requirements

### Azure VM

- Python 3.10 or newer,
- `fastapi`,
- `uvicorn`,
- `cryptography`,
- TCP port `8000` accessible from the ESP32 device.

### ESP32

- ESP32 or ESP32-CAM board,
- Arduino IDE or PlatformIO,
- `ArduinoJson` library,
- ESP32 built-in libraries: `WiFi`, `HTTPClient`, and `mbedTLS`.

## Running the Server

Navigate to the server application directory:

```bash
cd "Azure VM"
```

Create and activate a virtual environment:

```bash
python -m venv .venv
```

Windows:

```powershell
.\.venv\Scripts\Activate.ps1
```

Linux:

```bash
source .venv/bin/activate
```

Install the dependencies:

```bash
pip install fastapi uvicorn cryptography
```

Start the API responsible for the full handshake:

```bash
uvicorn benchmark_full:app --host 0.0.0.0 --port 8000
```

Check whether the server is available:

```text
GET http://<SERVER_ADDRESS>:8000/ping
```

Swagger UI documentation is available at:

```text
http://<SERVER_ADDRESS>:8000/docs
```

## Running Benchmarks on the Azure VM

Run the P-256, X25519, and RSA-2048 key-generation benchmark:

```bash
python keygen_benchmark.py
```

Run the shared-secret calculation benchmark:

```bash
python benchmark_shared_secrets.py
```

## Running Benchmarks on the ESP32

1. Open the selected file from the `ESP32` directory in Arduino IDE or
   PlatformIO.
2. Configure the Wi-Fi network name and password.
3. Replace the API addresses in `benchmarkUrl`, `handshakeUrlP256`, and
   `handshakeUrlX25519` with the address of your virtual machine.
4. Install the `ArduinoJson` library.
5. Upload the program to the device.
6. Open the serial monitor at `115200` baud.

After completing a test, the device uploads its results to:

```text
POST /benchmark
```

The server saves the data to `results.csv` using the following format:

```text
timestamp,device,algorithm,operation,iterations,avg_us,min_us,max_us
```

## API Endpoints

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/` | Check the API status |
| `GET` | `/ping` | Test server availability |
| `POST` | `/benchmark` | Save benchmark results to a CSV file |
| `POST` | `/ecdh/p256` | Perform an ECDH P-256 key exchange |
| `POST` | `/ecdh/x25519` | Perform an X25519 key exchange |

Example result sent to `/benchmark`:

```json
{
  "device": "ESP32-CAM",
  "algorithm": "ECDH_P256",
  "operation": "full_handshake",
  "iterations": 20,
  "avg_us": 120000,
  "min_us": 110000,
  "max_us": 140000
}
```

## Security Considerations

Before publishing or deploying this project:

- remove Wi-Fi credentials and public IP addresses from the source code,
- store configuration outside the repository,
- replace HTTP with HTTPS/TLS,
- authenticate both devices and the server,
- use a KDF to derive encryption keys from the shared secret,
- encrypt data with an authenticated encryption algorithm such as AES-GCM or
  ChaCha20-Poly1305,
- protect private keys appropriately.

## Possible Improvements

- measure RAM usage and energy consumption,
- compare additional IoT devices,
- add data encryption after the handshake,
- authenticate public keys,
- visualize results stored in the CSV file,
- automate experiments and report generation.

## Status

Research project under active development.
