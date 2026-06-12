from fastapi import FastAPI
from pydantic import BaseModel
from datetime import datetime

import csv
import os

from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric import x25519
from cryptography.hazmat.primitives import serialization

app = FastAPI()

# =====================================================
# MODELE
# =====================================================

class BenchmarkResult(BaseModel):
    device: str
    algorithm: str
    operation: str
    iterations: int
    avg_us: int
    min_us: int
    max_us: int


class PublicKeyRequest(BaseModel):
    public_key: str


# =====================================================
# BASIC
# =====================================================

@app.get("/")
def root():
    return {"status": "ok"}


@app.get("/ping")
def ping():
    return {"message": "pong"}


# =====================================================
# BENCHMARK
# =====================================================

@app.post("/benchmark")
def benchmark(result: BenchmarkResult):

    file_exists = os.path.isfile("results.csv")

    with open("results.csv", "a", newline="") as f:

        writer = csv.writer(f)

        if not file_exists:
            writer.writerow([
                "timestamp",
                "device",
                "algorithm",
                "operation",
                "iterations",
                "avg_us",
                "min_us",
                "max_us"
            ])

        writer.writerow([
            datetime.now().isoformat(),
            result.device,
            result.algorithm,
            result.operation,
            result.iterations,
            result.avg_us,
            result.min_us,
            result.max_us
        ])

    return {"status": "saved"}


# =====================================================
# ECDH P-256
# =====================================================

@app.post("/ecdh/p256")
def ecdh_p256(req: PublicKeyRequest):

    client_public_key = (
        ec.EllipticCurvePublicKey.from_encoded_point(
            ec.SECP256R1(),
            bytes.fromhex(req.public_key)
        )
    )

    server_private_key = ec.generate_private_key(
        ec.SECP256R1()
    )

    server_public_key = (
        server_private_key.public_key()
    )

    _ = server_private_key.exchange(
        ec.ECDH(),
        client_public_key
    )

    server_public_bytes = (
        server_public_key.public_bytes(
            encoding=serialization.Encoding.X962,
            format=serialization.PublicFormat.UncompressedPoint
        )
    )

    return {
        "public_key": server_public_bytes.hex()
    }


# =====================================================
# X25519
# =====================================================

@app.post("/ecdh/x25519")
def ecdh_x25519(req: PublicKeyRequest):

    client_public_key = (
        x25519.X25519PublicKey.from_public_bytes(
            bytes.fromhex(req.public_key)
        )
    )

    server_private_key = (
        x25519.X25519PrivateKey.generate()
    )

    server_public_key = (
        server_private_key.public_key()
    )

    _ = server_private_key.exchange(
        client_public_key
    )

    return {
        "public_key": server_public_key.public_bytes(
            encoding=serialization.Encoding.Raw,
            format=serialization.PublicFormat.Raw
        ).hex()
    }