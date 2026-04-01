import json
import sys

data = [
    {
        "name": "test1",
        "metric1": "val11",
        "metric2": "val12"
    }, {
        "name": "test1",
        "metric1": "val11",
        "metric2": "val12"
    }, {
        "name": "test1",
        "metric1": "val11",
        "metric2": "val12"
    }
]

BenchmarkingTests = [
        "Fibbonacci"
    ]

json.dump(data, sys.stdout)
