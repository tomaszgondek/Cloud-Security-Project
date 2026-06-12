import time
from statistics import mean
from cryptography.hazmat.primitives.asymmetric import ec, rsa, x25519


def benchmark(name, func, iterations):
    results = []

    for _ in range(iterations):
        start = time.perf_counter_ns()

        func()

        end = time.perf_counter_ns()

        results.append((end - start) / 1000)  # µs

    print()
    print(name)
    print(f"AVG: {mean(results):.0f} us")
    print(f"MIN: {min(results):.0f} us")
    print(f"MAX: {max(results):.0f} us")


def ecdh_keygen():
    ec.generate_private_key(ec.SECP256R1())


def x25519_keygen():
    x25519.X25519PrivateKey.generate()


def rsa_keygen():
    rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048
    )


benchmark(
    "ECDH_P256 key_generation",
    ecdh_keygen,
    100
)

benchmark(
    "X25519 key_generation",
    x25519_keygen,
    100
)

benchmark(
    "RSA_2048 key_generation",
    rsa_keygen,
    100
)
