import socket
import json
import struct
import time
import threading

def recv_exact(s, length):
    """정확히 length 바이트를 수신합니다."""
    data = b""
    while len(data) < length:
        chunk = s.recv(length - len(data))
        if not chunk:
            raise ConnectionError("Connection closed while receiving data")
        data += chunk
    return data

def send_player_data(player_id):
    server_host = "127.0.0.1"
    server_port = 12345

    try:
        time.sleep(player_id)  # 1초 간격으로 실행

        # 서버에 연결
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((server_host, server_port))
            print(f"Player {player_id} connected.")

            # JSON 바디 생성
            body_json = {
                "player_id": str(player_id),
                "player_name": f"player{player_id}"
            }
            body_data = json.dumps(body_json).encode("utf-8")

            # 본문 데이터 패딩 처리 (8바이트 배수로 맞춤)
            padded_body_length = ((len(body_data) + 7) // 8) * 8
            padded_body_data = body_data.ljust(padded_body_length, b'\x00')

            # 헤더 생성 (8바이트, 리틀 엔디언 명시)
            header = struct.pack("<B I 3x", 1, len(body_data))  # < : Little-endian

            # 디버깅: 전송하는 헤더 출력
            print(f"Header being sent: {[hex(b) for b in header]}")

            # 데이터 전송 (헤더 + 패딩된 본문 데이터)
            s.sendall(header + padded_body_data)
            print(f"Player {player_id} sent data.")

            # 데이터 수신
            while True:
                try:
                    # 헤더 읽기 (8바이트)
                    header_data = recv_exact(s, 8)
                    if not header_data:
                        break

                    # 헤더 디코딩
                    request_type, body_length = struct.unpack("<B I 3x", header_data)  # 리틀 엔디언으로 디코딩
                    # 디버깅: 수신한 헤더 출력
                    print(f"Raw header received: {[hex(b) for b in header_data]}")
                    print(f"Player {player_id} received header: RequestType={request_type}, BodyLength={body_length}")

                    # 본문 읽기 (패딩 포함)
                    padded_body_length = ((body_length + 7) // 8) * 8
                    body_data = recv_exact(s, padded_body_length)

                    # 디버깅: 수신한 본문 데이터 출력
                    print(f"Player {player_id} received raw body data: {body_data}")

                    # 패딩 제거 및 JSON 디코딩
                    try:
                        body_json = body_data[:body_length].decode("utf-8")
                        print(f"Player {player_id} received body: {body_json}")
                    except Exception as e:
                        print(f"Error decoding JSON for Player {player_id}: {e}")
                        break
                except Exception as e:
                    print(f"Error for player {player_id} during reception: {e}")
                    break

    except Exception as e:
        print(f"Error for player {player_id}: {e}")


# 플레이어 5명을 1초 간격으로 입장
threads = []
for i in range(1, 6):
    thread = threading.Thread(target=send_player_data, args=(i,))
    threads.append(thread)
    thread.start()

for thread in threads:
    thread.join()
