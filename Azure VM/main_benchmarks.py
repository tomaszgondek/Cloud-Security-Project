from fastapi import FastAPI
from pydantic import BaseModel
import csv
import os
from datetime import datetime

app = FastAPI()


class BenchmarkResult(BaseModel):
    device: str
    algorithm: str
    operation: str
    iterations: int
    avg_us: int
    min_us: int
    max_us: int


@app.get("/ping")
def ping():
    return {"message": "pong"}


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