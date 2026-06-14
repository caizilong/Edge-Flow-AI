import json
import socket
import time


HOST = "127.0.0.1"
PORT = 10001
INFERENCE_TIMEOUT_SECONDS = 300


class JsonLineClient:
    def __init__(self, host, port):
        self.sock = socket.create_connection((host, port), timeout=10)
        self.buffer = b""

    def send(self, message):
        payload = json.dumps(message, ensure_ascii=False) + "\n"
        self.sock.sendall(payload.encode("utf-8"))

    def receive(self):
        while b"\n" not in self.buffer:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("unit-manager closed the connection")
            self.buffer += chunk

        line, self.buffer = self.buffer.split(b"\n", 1)
        return json.loads(line.decode("utf-8"))

    def close(self):
        self.sock.close()


def ensure_success(response, action):
    error = response.get("error", {})

    # The current C++ channel may return error as either an object or a JSON string.
    if isinstance(error, str):
        try:
            error = json.loads(error)
        except json.JSONDecodeError:
            error = {"code": -1, "message": error}

    if error.get("code", 0) != 0:
        raise RuntimeError(f"{action} failed: {error}")


def main():
    client = JsonLineClient(HOST, PORT)
    work_id = None

    try:
        client.send(
            {
                "request_id": "rkllm_setup_001",
                "work_id": "rkllm",
                "action": "setup",
                "object": "rkllm.setup",
                "data": {},
            }
        )

        setup_response = client.receive()
        print("setup:", json.dumps(setup_response, ensure_ascii=False, indent=2))
        ensure_success(setup_response, "setup")

        work_id = setup_response["work_id"]

        # Give the ZMQ subscriber time to finish connecting before the first input.
        time.sleep(0.2)

        client.sock.settimeout(INFERENCE_TIMEOUT_SECONDS)
        client.send(
            {
                "request_id": "rkllm_infer_001",
                "work_id": work_id,
                "action": "inference",
                "object": "rkllm.input",
                "data": {
                    "prompt": "三句话介绍自己。",
                },
            }
        )

        # 非流式
        # inference_response = client.receive()
        # print(
        #     "inference:",
        #     json.dumps(inference_response, ensure_ascii=False, indent=2),
        # )
        # ensure_success(inference_response, "inference")

        # if inference_response.get("object") != "rkllm.result":
        #     raise RuntimeError(
        #         f"unexpected response object: {inference_response.get('object')}"
        #     )

        # 流式
        full_text = ""
        while True:
            response = client.receive()
            ensure_success(response, "inference")

            if response.get("object") != "rkllm.result.stream":
                raise RuntimeError(
                    f"unexpected object: {response.get('object')}"
                )

            data = response.get("data", {})
            delta = data.get("delta", "")
            finish = data.get("finish", False)

            if delta:
                print(delta, end="", flush=True)
                full_text += delta

            if finish:
                print()
                break

        print("full result:", full_text)
    finally:
        if work_id is not None:
            client.sock.settimeout(10)
            client.send(
                {
                    "request_id": "rkllm_exit_001",
                    "work_id": work_id,
                    "action": "exit",
                    "object": "rkllm.exit",
                    "data": {},
                }
            )
            exit_response = client.receive()
            print("exit:", json.dumps(exit_response, ensure_ascii=False, indent=2))

        client.close()


if __name__ == "__main__":
    main()
