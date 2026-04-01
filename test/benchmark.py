import json

data = {
    "name": ["test1", "test2", "test3"],
    "metric1": ["val11", "val12", "val13"],
    "metric2": ["val21", "val22", "val23"]
}

print(json.dumps(data))
