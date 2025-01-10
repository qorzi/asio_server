import socket
import struct
import json
import time
import select
import sys
import os

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

# 방향 선택지 (상, 하, 좌, 우 및 대각선)
DIRECTIONS = {
    'w': ('Up', (0, 1)),
    'a': ('Left', (-1, 0)),
    's': ('Down', (0, -1)),
    'd': ('Right', (1, 0)),
}

def clear_screen():
    """터미널 화면을 지우는 함수"""
    os.system('cls' if os.name == 'nt' else 'clear')

class ClientState:
    def __init__(self):
        self.maps = {}
        self.current_map = None
        self.position = (0, 0)
        self.room_id = None
        self.message = ""
        self.game_started = False
        self.seen_maps = {}  # 각 맵별로 본 영역을 추적

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
            self.refresh_screen()
            print(f"에러 발생: {error}")
        else:
            self.message = f"알 수 없는 메인 타입: {main_type}"
            self.refresh_screen()
            print(f"알 수 없는 메인 타입: {main_type}")

    def process_network(self, data):
        action = data.get('action', '')
        if action == 'join':
            self.message = "JOIN 성공!"
            self.refresh_screen()
            print("JOIN 성공!")
        elif action == 'left':
            self.message = "대기열에서 떠났습니다."
            self.refresh_screen()
            print("대기열에서 떠났습니다.")
        elif action == 'close':
            self.message = "연결이 종료되었습니다."
            self.refresh_screen()
            print("연결이 종료되었습니다.")
        else:
            self.message = f"알 수 없는 NETWORK 액션: {action}"
            self.refresh_screen()
            print(f"알 수 없는 NETWORK 액션: {action}")

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
            self.update_player_come_in_map(data)
        elif sub_type == GameSubType.PLAYER_COME_OUT_MAP:
            player_id = data.get('player_id', 'Unknown')
            self.message = f"플레이어가 맵에서 나갔습니다: {player_id}"
            self.refresh_screen()
            print(f"플레이어가 맵에서 나갔습니다: {player_id}")
        elif sub_type == GameSubType.PLAYER_FINISHED:
            player_id = data.get('player_id', 'Unknown')
            total_dist = data.get('total_dist', 'N/A')
            self.message = f"플레이어가 도착했습니다: {player_id}, 총 거리: {total_dist}"
            self.refresh_screen()
            print(f"플레이어가 도착했습니다: {player_id}, 총 거리: {total_dist}")
        elif sub_type == GameSubType.GAME_END:
            self.message = "게임이 종료되었습니다."
            self.refresh_screen()
            print("게임이 종료되었습니다.")
        else:
            self.message = f"알 수 없는 GAME 서브타입: {sub_type}"
            self.refresh_screen()
            print(f"알 수 없는 GAME 서브타입: {sub_type}")

    def update_room_create(self, data):
        maps = data.get('maps', [])
        for m in maps:
            self.maps[m['name']] = m
        if maps:
            self.current_map = maps[0]['name']
            self.position = tuple(maps[0]['start'].values())
            self.room_id = data.get('room_id', None)
            # 시야 초기화
            self.seen_maps[self.current_map] = set()
            self.update_seen_area(self.current_map, self.position)
            self.add_corners_to_seen(self.current_map)
        self.message = "방이 생성되었습니다."
        self.refresh_screen()
        print("방이 생성되었습니다.")

    def update_player_moved(self, data):
        self.position = (data.get('x', self.position[0]), data.get('y', self.position[1]))
        self.current_map = data.get('map', self.current_map)
        self.message = "플레이어가 이동했습니다."
        self.refresh_screen()
        print("플레이어가 이동했습니다.")
        # 시야 업데이트
        self.update_seen_area(self.current_map, self.position)

    def update_game_start(self, data):
        self.game_started = True
        self.message = "게임이 시작되었습니다!"
        self.refresh_screen()
        print("게임이 시작되었습니다!")

    def update_count_down(self, data):
        count = data.get('count', '0')
        self.message = f"카운트다운: {count}초"
        self.refresh_screen()
        print(f"카운트다운: {count}초")

    def update_player_come_in_map(self, data):
        player_id = data.get('player_id', 'Unknown')
        if player_id == "player1":  # 클라이언트 자신을 식별하는 조건
            new_map = data.get('map', self.current_map)
            new_x = data.get('x', self.position[0])
            new_y = data.get('y', self.position[1])

            # 현재 맵 업데이트
            self.current_map = new_map
            self.position = (new_x, new_y)

            self.message = f"맵이 변경되었습니다: {self.current_map}, 새로운 위치: ({new_x}, {new_y})"
            self.refresh_screen()
            print(f"맵이 변경되었습니다: {self.current_map}, 새로운 위치: ({new_x}, {new_y})")

            # 새로운 맵 정보가 있을 경우 시야 초기화 및 화면 업데이트
            if self.current_map in self.maps:
                if self.current_map not in self.seen_maps:
                    self.seen_maps[self.current_map] = set()
                self.update_seen_area(self.current_map, self.position)
                self.add_corners_to_seen(self.current_map)
                self.display_view()
            else:
                print("새로운 맵 정보가 클라이언트에 없습니다.")
        else:
            # 다른 플레이어가 맵에 입장한 경우 처리 (필요 시 확장 가능)
            self.message = f"플레이어가 맵에 입장했습니다: {player_id}"
            self.refresh_screen()
            print(f"플레이어가 맵에 입장했습니다: {player_id}")

    def update_seen_area(self, map_name, position):
        """플레이어의 현재 위치를 기준으로 시야 내 영역을 seen_maps에 추가"""
        x, y = position
        # 상하좌우 및 대각선 8방향
        for dx in [-1, 0, 1]:
            for dy in [-1, 0, 1]:
                nx, ny = x + dx, y + dy
                map_info = self.maps.get(map_name, {})
                width = map_info.get('width', 0)
                height = map_info.get('height', 0)
                if 0 <= nx < width and 0 <= ny < height:
                    self.seen_maps[map_name].add((nx, ny))

    def add_corners_to_seen(self, map_name):
        """맵의 네 모서리와 모든 가장자리 셀을 seen_maps에 추가"""
        map_info = self.maps.get(map_name, {})
        width = map_info.get('width', 0)
        height = map_info.get('height', 0)

        # 상단 및 하단 가장자리 셀 추가
        for i in range(width):
            # 상단 행 (j = 0)
            if 0 <= i < width and 0 <= 0 < height:
                self.seen_maps[map_name].add((i, 0))
            # 하단 행 (j = height - 1)
            if 0 <= i < width and 0 <= height - 1 < height:
                self.seen_maps[map_name].add((i, height - 1))

        # 좌측 및 우측 가장자리 셀 추가
        for j in range(height):
            # 좌측 열 (i = 0)
            if 0 <= 0 < width and 0 <= j < height:
                self.seen_maps[map_name].add((0, j))
            # 우측 열 (i = width - 1)
            if 0 <= width - 1 < width and 0 <= j < height:
                self.seen_maps[map_name].add((width - 1, j))


    def display_view(self):
        clear_screen()  # 추가: 화면 지우기
        if self.current_map and self.current_map in self.maps:
            map_info = self.maps[self.current_map]
            width = map_info['width']
            height = map_info['height']
            obstacles = { (obs['x'], obs['y']) for obs in map_info.get('obstacles', []) }
            portals = { (portal['x'], portal['y']): portal['name'] for portal in map_info.get('portals', []) }

            start = (map_info['start']['x'], map_info['start']['y'])
            end = (map_info.get('end', {}).get('x'), map_info.get('end', {}).get('y'))

            x, y = self.position

            print(f"현재 맵: {self.current_map} (Room ID: {self.room_id})")
            print(f"현재 위치: ({x}, {y})\n")

            for j in reversed(range(height)):
                row = []
                for i in range(width):
                    # 현재 맵의 seen_maps에서 해당 위치가 보이는지 확인
                    if self.current_map in self.seen_maps and (i, j) in self.seen_maps[self.current_map]:
                        # 모서리에 벽 추가
                        if (i == 0 and j == 0) or (i == width - 1 and j == 0) or \
                           (i == 0 and j == height - 1) or (i == width - 1 and j == height - 1):
                            row.append('+')
                        # 맨 위와 맨 아래에 가로선 추가
                        elif j == 0 or j == height - 1:
                            row.append('-')
                        # 맨 왼쪽과 맨 오른쪽에 세로선 추가
                        elif i == 0 or i == width - 1:
                            row.append('|')
                        # 플레이어 위치
                        elif (i, j) == self.position:
                            row.append('U')
                        # 시작점
                        elif (i, j) == start:
                            row.append('S')
                        # 종료점
                        elif end and (i, j) == end:
                            row.append('E')
                        # 포탈
                        elif (i, j) in portals:
                            row.append('P')
                        # 장애물
                        elif (i, j) in obstacles:
                            row.append('#')
                        # 빈 공간
                        else:
                            row.append('.')
                    else:
                        # 안개 영역 표시
                        row.append('?')
                print(' '.join(row))
            print()
            print("이동할 방향을 선택하세요 (w: Up, a: Left, s: Down, d: Right) 또는 'q'를 누르세요:")
            print()
        else:
            print("현재 맵 정보가 없습니다.")
            print()

    def refresh_screen(self):
        """화면을 새로 고치고 메시지를 표시하는 함수"""
        clear_screen()
        self.display_view()
        if self.message:
            print(self.message)
            self.message = ""

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
            state.refresh_screen()  # 메시지 표시를 위해 화면 새로 고침
            print(f"[Client] SEND => Move {direction_name}")
        except Exception as e:
            state.message = f"[Client] MOVE 전송 실패: {e}"
            state.refresh_screen()
            print(state.message)
    else:
        state.message = "이동할 수 없는 방향입니다. (맵의 경계를 벗어났습니다.)"
        state.refresh_screen()
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
            state.refresh_screen()
            print(state.message)
        except Exception as e:
            state.message = f"[Client] JOIN 전송 실패: {e}"
            state.refresh_screen()
            print(state.message)
            return
    else:
        state.message = "[Client] JOIN을 취소했습니다."
        state.refresh_screen()
        print("클라이언트를 종료합니다.")
        s.close()
        return

    running = True
    while running:
        # select.select에 소켓과 stdin을 등록합니다.
        readable, _, _ = select.select([s, sys.stdin], [], [], 0.1)

        for r in readable:
            if r == s:
                # 소켓에서 데이터가 들어온 경우
                packet = parse_packet(s)
                if packet is not None:
                    state.process_packet(packet)
                else:
                    print("서버와의 연결이 끊어졌습니다.")
                    running = False
                    break
            elif r == sys.stdin:
                # 사용자 입력이 들어온 경우
                if state.game_started:
                    choice = sys.stdin.readline().strip().lower()
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
                    else:
                        print("잘못된 입력입니다. 'w', 'a', 's', 'd' 또는 'q'를 입력해주세요.")
                else:
                    # 게임이 시작되지 않았을 때는 입력을 무시하거나 다른 처리를 할 수 있습니다.
                    print("게임이 시작되지 않았습니다. 잠시 기다려주세요.")

        # 게임이 시작되지 않았거나, 입력이 필요 없는 경우 잠시 대기
        time.sleep(0.1)  # CPU 점유율을 낮추기 위한 sleep

    s.close()


if __name__ == "__main__":
    main()
