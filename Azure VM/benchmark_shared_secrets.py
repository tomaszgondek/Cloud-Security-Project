import time
from statistics import mean

from cryptography.hazmat.primitives.asymmetric import ec, x25519


def benchmark(name, func, iterations=100):
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


def ecdh_shared_secret():

    private_a = ec.generate_private_key(ec.SECP256R1())
    private_b = ec.generate_private_key(ec.SECP256R1())

    public_b = private_b.public_key()

    private_a.exchange(
        ec.ECDH(),
        public_b
    )


def x25519_shared_secret():

    private_a = x25519.X25519PrivateKey.generate()
    private_b = x25519.X25519PrivateKey.generate()

    public_b = private_b.public_key()

    private_a.exchange(public_b)


benchmark(
    "ECDH_P256 shared_secret",
    ecdh_shared_secret,
    100
)

benchmark(
    "X25519 shared_secret",
    x25519_shared_secret,
    100
)
