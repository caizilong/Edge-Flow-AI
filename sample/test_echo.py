import socket
import json

def send_json(sock, data):
    sock.sendall((json.dumps(data, ensure_ascii=False) + "\n").encode("utf-8"))

def recv_json(sock):
    buf = ""
    while "\n" not in buf:
        buf += sock.recv(4096).decode("utf-8")
    return json.loads(buf.strip())

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("localhost", 10001))

send_json(sock, {
    "request_id": "echo_setup_001",
    "work_id": "echo",
    "action": "setup",
    "object": "echo.setup",
    "data": {}
})

setup_resp = recv_json(sock)
print("setup:", setup_resp)

echo_work_id = setup_resp["work_id"]

send_json(sock, {
    "request_id": "echo_infer_001",
    "work_id": echo_work_id,
    "action": "inference",
    "object": "echo.input",
    "data": {
        "text": "hello echo"
    }
})

infer_resp = recv_json(sock)
print("inference:", infer_resp)

send_json(sock, {
    "request_id": "echo_exit_001",
    "work_id": echo_work_id,
    "action": "exit",
    "object": "echo.exit",
    "data": {}
})

print("exit:", recv_json(sock))
sock.close()