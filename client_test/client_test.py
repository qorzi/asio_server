import socket
import struct
import json
import time

# 이벤트 타입 정의
class MainEventType:
    NETWORK = 1
    GAME = 2
    ERROR = 3

class NetworkSubType:
    JOIN = 101
    LEFT = 102
    CLOSE = 103

class GameSubType:
    ROOM_CREATE = 201
    GAME_COUNTDOWN = 202
    GAME_START = 203
    PLAYER_MOVED = 204
    PLAYER_COME_IN_MAP = 205
    PLAYER_COME_OUT_MAP = 206
    PLAYER_FINISHED = 207
    GAME_END = 208

class ErrorSubType:
    UNKNOWN = 301

# 방향 선택지 (상, 하, 좌, 우)
DIRECTIONS = {
    'w': ('Up', (0, 1)),
    'a': ('Left', (-1, 0)),
    's': ('Down', (0, -1)),
    'd': ('Right', (1, 0)),
}

class ClientState:
    def __init__(self):
        self.maps = {}
        self.current_map = None
        self.position = (0, 0)
        self.room_id = None
        self.message = ""
        self.game_started = False

    def process_packet(self, packet):
        main_type, sub_type, data = packet
        if main_type == MainEventType.NETWORK:
            self.process_network(data)
        elif main_type == MainEventType.GAME:
            self.process_game(sub_type, data)
            if sub_type == GameSubType.GAME_START or sub_type == GameSubType.PLAYER_MOVED:
                self.display_view()
        elif main_type == MainEventType.ERROR:
            error = data.get('error', 'Unknown error')
            self.message = f"에러 발생: {error}"
            print(f"\n에러 발생: {error}")
        else:
            self.message = f"알 수 없는 메인 타입: {main_type}"
            print(f"\n알 수 없는 메인 타입: {main_type}")

    def process_network(self, data):
        action = data.get('action', '')
        if action == 'join':
            self.message = "JOIN 성공!"
            print("\nJOIN 성공!")
        elif action == 'left':
            self.message = "대기열에서 떠났습니다."
            print("\n대기열에서 떠났습니다.")
        elif action == 'close':
            self.message = "연결이 종료되었습니다."
            print("\n연결이 종료되었습니다.")
        else:
            self.message = f"알 수 없는 NETWORK 액션: {action}"
            print(f"\n알 수 없는 NETWORK 액션: {action}")

    def process_game(self, sub_type, data):
        if sub_type == GameSubType.ROOM_CREATE:
            self.update_room_create(data)
        elif sub_type == GameSubType.GAME_COUNTDOWN:
            self.update_count_down(data)
        elif sub_type == GameSubType.GAME_START:
            self.update_game_start(data)
        elif sub_type == GameSubType.PLAYER_MOVED:
            self.update_player_moved(data)
        elif sub_type == GameSubType.PLAYER_COME_IN_MAP:
            player_id = data.get('player_id', 'Unknown')
            self.message = f"플레이어가 맵에 입장했습니다: {player_id}"
            print(f"\n플레이어가 맵에 입장했습니다: {player_id}")
        elif sub_type == GameSubType.PLAYER_COME_OUT_MAP:
            player_id = data.get('player_id', 'Unknown')
            self.message = f"플레이어가 맵에서 나갔습니다: {player_id}"
            print(f"\n플레이어가 맵에서 나갔습니다: {player_id}")
        elif sub_type == GameSubType.PLAYER_FINISHED:
            player_id = data.get('player_id', 'Unknown')
            total_dist = data.get('total_dist', 'N/A')
            self.message = f"플레이어가 도착했습니다: {player_id}, 총 거리: {total_dist}"
            print(f"\n플레이어가 도착했습니다: {player_id}, 총 거리: {total_dist}")
        elif sub_type == GameSubType.GAME_END:
            self.message = "게임이 종료되었습니다."
            print("\n게임이 종료되었습니다.")
        else:
            self.message = f"알 수 없는 GAME 서브타입: {sub_type}"
            print(f"\n알 수 없는 GAME 서브타입: {sub_type}")

    def update_room_create(self, data):
        maps = data.get('maps', [])
        for m in maps:
            self.maps[m['name']] = m
        if maps:
            self.current_map = maps[0]['name']
            self.position = tuple(maps[0]['start'].values())
            self.room_id = data.get('room_id', None)
        self.message = "방이 생성되었습니다."
        print("\n방이 생성되었습니다.")

    def update_player_moved(self, data):
        self.position = (data.get('x', self.position[0]), data.get('y', self.position[1]))
        self.current_map = data.get('map', self.current_map)
        self.message = "플레이어가 이동했습니다."
        print("\n플레이어가 이동했습니다.")

    def update_game_start(self, data):
        self.game_started = True
        self.message = "게임이 시작되었습니다!"
        print("\n게임이 시작되었습니다!")

    def update_count_down(self, data):
        count = data.get('count', '0')
        self.message = f"카운트다운: {count}초"
        print(f"\n카운트다운: {count}초")

    def display_view(self):
        if self.current_map and self.current_map in self.maps:
            map_info = self.maps[self.current_map]
            width = map_info['width']
            height = map_info['height']
            room_id = self.room_id
            x, y = self.position

            print(f"현재 맵: {self.current_map} (Room ID: {room_id})")
            print(f"현재 위치: ({x}, {y})")
            print()

            view_size = 3
            half_view = view_size // 2

            view_x = x - half_view
            view_y = y - half_view

            if view_x < 0:
                view_x = 0
            elif view_x + view_size > width:
                view_x = max(width - view_size, 0)

            if view_y < 0:
                view_y = 0
            elif view_y + view_size > height:
                view_y = max(height - view_size, 0)

            view = []
            for dy in range(view_y, view_y + view_size):
                row = []
                for dx in range(view_x, view_x + view_size):
                    if 0 <= dx < width and 0 <= dy < height:
                        if (dx, dy) == (x, y):
                            row.append('P')
                        else:
                            row.append('.')
                    else:
                        row.append('X')
                view.append(' '.join(row))

            print("현재 시야:")
            for row in view:
                print(row)
            print()
        else:
            print("현재 맵 정보가 없습니다.")
            print()

        print(self.message)
        print()

def build_packet(main_type, sub_type, data_dict):
    body_str = json.dumps(data_dict, ensure_ascii=False)
    body_bytes = body_str.encode('utf-8')
    body_len = len(body_bytes)

    header = struct.pack('<HHI', main_type, sub_type, body_len)
    padded_len = ((body_len + 7) // 8) * 8
    body_bytes_padded = body_bytes + b'\x00' * (padded_len - body_len)

    return header + body_bytes_padded

def parse_packet(sock):
    header_data = recv_all(sock, 8)
    if not header_data:
        return None
    main_type, sub_type, body_len = struct.unpack('<HHI', header_data)
    print(f"[Debug] Received packet: main_type={main_type}, sub_type={sub_type}, body_len={body_len}")

    padded_len = ((body_len + 7) // 8) * 8

    body_data = b''
    if padded_len > 0:
        body_data = recv_all(sock, padded_len)
        if not body_data:
            return None

    actual_data = body_data[:body_len]
    actual_data = actual_data.rstrip(b'\x00')

    body_str = actual_data.decode('utf-8', errors='replace')
    try:
        data_json = json.loads(body_str) if body_str else {}
    except:
        data_json = {"raw": body_str}

    print(f"[Debug] Received body: {data_json}")
    return (main_type, sub_type, data_json)

def recv_all(sock, length):
    buf = b''
    while len(buf) < length:
        try:
            chunk = sock.recv(length - len(buf))
            if not chunk:
                return None
            buf += chunk
        except:
            return None
    return buf

def send_move_command(sock, direction, state: ClientState):
    if direction not in DIRECTIONS:
        return
    direction_name, (dx, dy) = DIRECTIONS[direction]
    current_x, current_y = state.position
    map_info = state.maps.get(state.current_map, {})
    width = map_info.get('width', 0)
    height = map_info.get('height', 0)
    new_x = current_x + dx
    new_y = current_y + dy

    if 0 <= new_x < width and 0 <= new_y < height:
        move_data = {
            "x": new_x,
            "y": new_y
        }
        pkt = build_packet(MainEventType.GAME, GameSubType.PLAYER_MOVED, move_data)
        try:
            sock.sendall(pkt)
            state.message = f"[Client] SEND => Move {direction_name}"
            print(f"[Client] SEND => Move {direction_name}")
        except Exception as e:
            state.message = f"[Client] MOVE 전송 실패: {e}"
            print(state.message)
    else:
        state.message = "이동할 수 없는 방향입니다. (맵의 경계를 벗어났습니다.)"
        print(state.message)

def prompt_join():
    while True:
        choice = input("JOIN 하시겠습니까? (y/n): ").strip().lower()
        if choice == 'y':
            return True
        elif choice == 'n':
            return False
        else:
            print("잘못된 입력입니다. 'y' 또는 'n'을 입력해주세요.")

def main():
    state = ClientState()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect(("127.0.0.1", 12345))  # 서버 주소/포트
    except Exception as e:
        print(f"서버에 연결할 수 없습니다: {e}")
        return

    if prompt_join():
        join_pkt = build_packet(MainEventType.NETWORK, NetworkSubType.JOIN,
                                {"player_id": "player1", "player_name": "TestUser1"})
        try:
            s.sendall(join_pkt)
            state.message = "[Client] JOIN 패킷 전송 완료."
            print(state.message)
        except Exception as e:
            state.message = f"[Client] JOIN 전송 실패: {e}"
            print(state.message)
            return
    else:
        state.message = "[Client] JOIN을 취소했습니다."
        print("클라이언트를 종료합니다.")
        s.close()
        return

    running = True
    while running:
        packet = parse_packet(s)
        if packet is not None:
            state.process_packet(packet)

        if state.game_started:
            while True:
                print("이동할 방향을 선택하세요 (w: Up, a: Left, s: Down, d: Right) 또는 'q'를 누르세요:")
                choice = input("선택: ").strip().lower()
                if choice == 'q':
                    print("클라이언트를 종료합니다.")
                    leave_pkt = build_packet(MainEventType.NETWORK, NetworkSubType.LEFT,
                                             {"player_id": "player1", "player_name": "TestUser1"})
                    try:
                        s.sendall(leave_pkt)
                        print("[Client] LEFT 패킷 전송 완료.")
                    except:
                        print("[Client] LEFT 패킷 전송 실패.")
                    running = False
                    break
                elif choice in DIRECTIONS:
                    send_move_command(s, choice, state)
                    break  # Move on after a valid command
                else:
                    print("잘못된 입력입니다. 'w', 'a', 's', 'd' 또는 'q'를 입력해주세요.")

        time.sleep(0.1)  # CPU 점유율을 낮추기 위한 sleep

    s.close()

if __name__ == "__main__":
    main()