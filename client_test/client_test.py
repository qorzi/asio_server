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
        self.self_id = None  # 자신의 플레이어 ID
        self.player_name = None  # 자신의 플레이어 이름
        self.maps = {}
        self.current_map = None
        self.position = (0, 0)
        self.room_id = None
        self.message = ""
        self.game_started = False
        self.show_leaderboard = False
        self.seen_maps = {}  # 각 맵별로 본 영역을 추적
        self.players = {}
        self.leaderboard_results = None  # 리더보드 데이터를 저장

    def process_packet(self, packet):
        main_type, sub_type, data = packet
        if main_type == MainEventType.NETWORK:
            self.process_network(data)
        elif main_type == MainEventType.GAME:
            self.process_game(sub_type, data)
            if sub_type == GameSubType.GAME_START or sub_type == GameSubType.PLAYER_MOVED:
                # 게임 중에는 display_view() 호출 (단, finish 시에는 호출하지 않음)
                if self.game_started:
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
            result = data.get('result', False)
            if result:
                self_id = data.get('player_id', None)
                if self_id:
                    self.self_id = self_id
                    self.player_name = data.get('player_name', 'Unknown')
                    self.players[self.self_id] = {
                        'name': self.player_name,
                        'position': self.position  # 초기 위치 설정
                    }
                    self.message = f"JOIN 성공! 플레이어 ID: {self.self_id}"
                    self.refresh_screen()
                    print(f"JOIN 성공! 플레이어 ID: {self.self_id}")
                else:
                    self.message = "JOIN 응답에 player_id가 없습니다."
                    self.refresh_screen()
                    print("JOIN 응답에 player_id가 없습니다.")
            else:
                self.message = "JOIN 실패!"
                self.refresh_screen()
                print("JOIN 실패!")
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
            if player_id in self.players:
                del self.players[player_id]
            self.refresh_screen()
            print(f"플레이어가 맵에서 나갔습니다: {player_id}")
        elif sub_type == GameSubType.PLAYER_FINISHED:
            self.update_player_finished(data)
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
            # 플레이어 정보 초기화
            players = data.get('players', [])
            self.players = {}
            for p in players:
                pid = p.get('id')
                pname = p.get('name')
                pos = p.get('position', {})
                self.players[pid] = {
                    'name': pname,
                    'position': (pos.get('x', 0), pos.get('y', 0))
                }
        self.message = "방이 생성되었습니다."
        self.refresh_screen()
        print("방이 생성되었습니다.")

    def update_player_moved(self, data):
        player_id = data.get('player_id', 'Unknown')
        new_x = data.get('x', self.position[0])
        new_y = data.get('y', self.position[1])
        new_map = data.get('map', self.current_map)

        # 맵이 현재 맵과 동일한지 확인
        if new_map != self.current_map:
            # 같은 맵에 대한 이벤트만 처리
            return

        if player_id == self.self_id:
            self.position = (new_x, new_y)
            self.message = "플레이어가 이동했습니다."
            print("플레이어가 이동했습니다.")
        else:
            # 다른 플레이어의 이동 처리
            if player_id in self.players:
                self.players[player_id]['position'] = (new_x, new_y)
                self.message = f"{self.players[player_id]['name']}가 이동했습니다."
                print(f"{self.players[player_id]['name']}가 이동했습니다.")
            else:
                # 새로운 플레이어의 이동 이벤트를 받은 경우 players에 추가
                pname = data.get('name', 'Unknown')
                self.players[player_id] = {
                    'name': pname,
                    'position': (new_x, new_y)
                }
                self.message = f"새 플레이어가 이동했습니다: {player_id}"
                print(f"새 플레이어가 이동했습니다: {player_id}")

        self.refresh_screen()
        # 시야 업데이트는 자신 위치 기준으로만 수행
        self.update_seen_area(self.current_map, self.position)
        # 이동 이벤트 후 display_view() 호출 (game_started가 True일 때만)
        if self.game_started:
            self.display_view()

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
        if player_id == self.self_id:
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

                # 플레이어 정보 업데이트
                players = data.get('players', [])
                self.players = {}
                for p in players:
                    pid = p.get('id')
                    if pid == self.self_id: continue
                    pname = p.get('name')
                    pos = p.get('position', {})
                    self.players[pid] = {
                        'name': pname,
                        'position': (pos.get('x', 0), pos.get('y', 0))
                    }
            else:
                print("새로운 맵 정보가 클라이언트에 없습니다.")

            # 다른 플레이어 위치 정보 업데이트
            players = data.get('players', [])
            for p in players:
                pid = p.get('player_id')
                pname = p.get('name', 'Unknown')
                new_x = p.get('x', 0)
                new_y = p.get('y', 0)
                # 해당 플레이어가 이미 존재하면 위치 정보를 업데이트하고, 없으면 새로 추가합니다.
                self.players[pid] = {
                    'name': pname,
                    'position': (new_x, new_y)
                }

        else:
            # 다른 플레이어가 맵에 입장한 경우 처리 (필요 시 확장 가능)
            self.message = f"플레이어가 맵에 입장했습니다: {player_id}"
            self.refresh_screen()
            print(f"플레이어가 맵에 입장했습니다: {player_id}")

            name = data.get('name', 'Unknown')
            pos = data.get('position', {})
            self.players[player_id] = {
                'name': name,
                'position': (pos.get('x', 0), pos.get('y', 0))
            }

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

    def update_player_finished(self, data):
        player_id = data.get("player_id", "Unknown")
        results = data.get("results", [])

        if player_id == self.self_id:
            # 내가 finish 한 경우
            self.message = "게임을 완료하셨습니다! 순위표를 확인하세요."
            self.game_started = False  # 이후 이동 입력은 받지 않음.
            self.show_leaderboard = True  # 리더보드 모드로 전환
            self.leaderboard_results = results  # 리더보드 데이터를 저장
            self.display_leaderboard(results)
        else:
            # 다른 플레이어 완료 케이스 처리
            if self.game_started:
                finished_player = next((p for p in results if p["player_id"] == player_id), {})
                player_name = finished_player.get("player_name", "Unknown")
                self.message = f"{player_name} 님이 게임을 완료했습니다!"
                self.refresh_screen()
                print(self.message)
            else:
                self.leaderboard_results = results
                self.display_leaderboard(results)

    def display_leaderboard(self, results):
        """순위표를 출력"""
        clear_screen()
        print("🏆 순위표 🏆")
        print("-" * 30)
        for result in results:
            rank = result.get("rank", "N/A")
            player_id = result.get("player_id", "Unknown")
            player_name = result.get("player_name", "Unknown")
            total_distance = result.get("total_distance", "N/A")
            print(f"{rank}위: {player_name} (ID: {player_id}, 총 이동 거리: {total_distance})")
        print("-" * 30)
        print("게임이 종료되었습니다. 수고하셨습니다!")
        print("종료하려면 'q'를 입력하세요.")

    def display_view(self):
        clear_screen()
        if self.current_map and self.current_map in self.maps:
            map_info = self.maps[self.current_map]
            width = map_info['width']
            height = map_info['height']
            obstacles = { (obs['x'], obs['y']) for obs in map_info.get('obstacles', []) }
            portals = { (portal['x'], portal['y']): portal['name'] for portal in map_info.get('portals', []) }

            # 시작, 종료 지점 좌표 추출
            start = (map_info['start']['x'], map_info['start']['y'])
            end = (map_info.get('end', {}).get('x'), map_info.get('end', {}).get('y'))

            print(f"현재 맵: {self.current_map} (Room ID: {self.room_id})")
            print(f"현재 위치: {self.position}\n")

            for j in reversed(range(height)):
                row = []
                for i in range(width):
                    # 시야 내 셀인지 확인
                    if self.current_map in self.seen_maps and (i, j) in self.seen_maps[self.current_map]:
                        # 가장자리(모서리, 상단/하단, 좌측/우측) 처리
                        if (i == 0 and j == 0) or (i == width - 1 and j == 0) or \
                           (i == 0 and j == height - 1) or (i == width - 1 and j == height - 1):
                            row.append('+')
                        elif j == 0 or j == height - 1:
                            row.append('-')
                        elif i == 0 or i == width - 1:
                            row.append('|')
                        # 플레이어 위치 체크: 다른 모든 것보다 우선
                        elif (i, j) == self.position:
                            row.append('U')
                        # 시작점, 종료점, 포탈, 장애물 등의 순서
                        elif (i, j) == start:
                            row.append('S')
                        elif end and (i, j) == end:
                            row.append('E')
                        elif (i, j) in portals:
                            row.append('P')
                        elif (i, j) in obstacles:
                            row.append('#')
                        else:
                            # 다른 플레이어 체크
                            player_here = False
                            for pid, pdata in self.players.items():
                                if pdata['position'] == (i, j):
                                    row.append('O')
                                    player_here = True
                                    break
                            if not player_here:
                                row.append('.')
                    else:
                        row.append('?')  # 안개 영역
                print(' '.join(row))
            print()
            print("이동할 방향을 선택하세요 (w: Up, a: Left, s: Down, d: Right) 또는 'q'를 누르세요:")
            print()
        else:
            print("현재 맵 정보가 없습니다.\n")

    def refresh_screen(self):
        """화면을 새로 고치고 메시지를 표시하는 함수"""
        clear_screen()
        if self.show_leaderboard and self.leaderboard_results is not None:
            self.display_leaderboard(self.leaderboard_results)
        else:
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
        if not state.self_id:
            state.message = "자신의 player_id가 설정되지 않았습니다."
            state.refresh_screen()
            print("자신의 player_id가 설정되지 않았습니다.")
            return

        move_data = {
            "player_id": state.self_id,  # 자신의 player_id 포함
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
        player_name = "TestUser"  # 원하는 플레이어 이름으로 설정 가능
        join_pkt = build_packet(MainEventType.NETWORK, NetworkSubType.JOIN,
                                {"player_name": player_name})
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
                # 만약 game_started가 False라면, 리더보드 화면 상태이므로 'q' 입력만 허용합니다.
                choice = sys.stdin.readline().strip().lower()
                if choice == 'q':
                    print("클라이언트를 종료합니다.")
                    if state.self_id:
                        leave_pkt = build_packet(MainEventType.NETWORK, NetworkSubType.LEFT,
                                                 {"player_id": state.self_id, "player_name": state.player_name})
                        try:
                            s.sendall(leave_pkt)
                            print("[Client] LEFT 패킷 전송 완료.")
                        except:
                            print("[Client] LEFT 패킷 전송 실패.")
                    else:
                        print("자신의 player_id를 알 수 없어 LEFT 패킷을 보낼 수 없습니다.")
                    running = False
                    break
                else:
                    if state.game_started:
                        if choice in DIRECTIONS:
                            send_move_command(s, choice, state)
                        else:
                            print("잘못된 입력입니다. 'w', 'a', 's', 'd' 또는 'q'를 입력해주세요.")
                    else:
                        # finish 상태에서는 다른 입력은 무시하고, 'q' 입력만 가능
                        print("게임이 종료되었습니다. 종료하려면 'q'를 입력하세요.")


        # 게임이 시작되지 않았거나, 입력이 필요 없는 경우 잠시 대기
        time.sleep(0.1)  # CPU 점유율을 낮추기 위한 sleep

    s.close()


if __name__ == "__main__":
    main()
