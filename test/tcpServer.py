import socket

SRV_HOST = "127.0.0.1"
SRV_PORT = 45678
SRV_CONNECTION = (SRV_HOST, SRV_PORT)

BUFFER_SIZE = 1024


def main():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    client.connect(SRV_CONNECTION)
    print(f"[TcpClient] Connected to {SRV_HOST}:{SRV_PORT}")

    try:
        while True:
            msg = input("Enter a message (or 'quit' to exit): ")

            if msg.lower() == "quit":
                break

            client.sendall(msg.encode("utf-8"))

            response = client.recv(BUFFER_SIZE)
            print(f"[TcpClient] Received from Server: {response.decode("utf-8")}")

    finally:
        client.close()
        print("[TcpClient] Connection closed")


if __name__ == "__main__":
    main()
