#include "game_manager.hpp"
#include <iostream>
#include <algorithm>

GameManager::GameManager(boost::asio::io_context& ioc)
    : ioc_(ioc)
{
    std::cout << "[GameManager] Constructor\n";
    waiting_timer_ = std::make_unique<boost::asio::steady_timer>(ioc_);
}

GameManager::~GameManager() {
    shutdown();
    std::cout << "[GameManager] Destructor\n";
}

void GameManager::initialize() {
    std::cout << "[GameManager] initialize() called.\n";
    // 초기화 시점에 특별히 할 일이 없으므로 생략
}

void GameManager::shutdown() {
    // 대기 타이머 정지
    stop_waiting_timer();

    // 룸 정리
    std::lock_guard<std::mutex> lock(room_mutex_);
    for (auto& r : rooms_) {
        // 룸에서 정리할 요소가 있다면, 처리
        std::cout << "[GameManager] Removing Room #" << r->id_ << "\n";
    }
    rooms_.clear();

    std::cout << "[GameManager] shutdown - all rooms cleared.\n";
}

/**
 * 1) 플레이어가 "게임 참가"를 요청하면, 여기로 들어옴.
 * 2) waiting_players_에 저장
 * 3) 대기 인원 안내
 * 4) 인원이 충분하면(Room threshold) 즉시 Room 생성 & 대기 타이머 정지
 */
void GameManager::join_game(std::shared_ptr<Player> player) {
    {
        std::lock_guard<std::mutex> lock(waiting_mutex_);
        waiting_players_.push_back(player);
    }

    // 대기 인원 안내
    std::string msg = "[WAITING] You are in queue. Current waiting: " 
                      + std::to_string(waiting_players_.size());
    player->send_message(msg);

    // 인원 충분한지 확인 (방 생성 시도)
    try_create_room_if_ready();
}

/**
 * 대기열에 "5명 이상"이 있으면, 한 번에 "5명"씩 방 생성
 * 예: 104명 대기시, 5명씩 20개의 방 + 4명 대기 (if >=5)
 */
void GameManager::try_create_room_if_ready() {
    std::lock_guard<std::mutex> lock(waiting_mutex_);

    // 방 생성이 1번으로 끝나지 않고,
    // 대기열에 5명 이상이면 계속 반복
    while (true) {
        size_t count = waiting_players_.size();
        if (count < WAITING_ROOM_THRESHOLD) {
            // 최소 인원(5) 미만 => 더 이상 방을 생성할 수 없음
            break;
        }

        // room_size만큼 잘라서 옮길 벡터
        std::vector<std::shared_ptr<Player>> to_move;
        to_move.reserve(WAITING_ROOM_THRESHOLD);

        // waiting_players_의 앞에서 room_size만큼 뽑아온다
        // (단순 예시로 vector 앞부분 사용)
        for (size_t i = 0; i < WAITING_ROOM_THRESHOLD; ++i) {
            to_move.push_back(waiting_players_[i]);
        }

        // 지운다
        waiting_players_.erase(waiting_players_.begin(), 
                               waiting_players_.begin() + WAITING_ROOM_THRESHOLD);

        // 방 생성
        create_room(to_move);

        // 남은 인원 다시 체크 -> while 루프 재진입
    }

    // 만약 아직 대기열에 5명 미만이 남아 있고,
    // 대기 타이머가 꺼져있으면 켠다. (대기 타이머로 30초 후 방 생성 or 취소)
    if (!waiting_players_.empty()) {
        if (!waiting_timer_active_) {
            start_waiting_timer();
        }

        // 남은 대기 인원에게 상황 알림
        size_t remaining = waiting_players_.size();
        for (auto& p : waiting_players_) {
            p->send_message("[WAITING] Currently " + std::to_string(remaining) 
                            + " players in queue. Need " 
                            + std::to_string(WAITING_ROOM_THRESHOLD - remaining) 
                            + " more or wait 30s.");
        }
    } else {
        // 아무도 안 남았으면 대기 타이머 필요 없음
        stop_waiting_timer();
    }
}

/**
 * 30초 타이머 시작
 */
void GameManager::start_waiting_timer() {
    waiting_timer_active_ = true;
    waiting_timer_->expires_after(std::chrono::milliseconds(WAITING_ROOM_TIMEOUT_MS));

    // async_wait 등록
    waiting_timer_->async_wait([this](const boost::system::error_code& ec){
        on_waiting_timer_expired(ec);
    });

    std::cout << "[GameManager] start_waiting_timer() - 30s.\n";
}

/**
 * 타이머 중단(취소)
 */
void GameManager::stop_waiting_timer() {
    if (!waiting_timer_active_) return;

    boost::system::error_code ec;
    waiting_timer_->cancel(ec);
    waiting_timer_active_ = false;

    if (ec) {
        std::cout << "[GameManager] stop_waiting_timer() error: " << ec.message() << "\n";
    } else {
        std::cout << "[GameManager] stop_waiting_timer() called.\n";
    }
}

/**
 * 30초 만료시
 *  - 현재까지 모인 인원만큼이라도 방 생성
 *  - 없으면 방을 만들지 않고, 그냥 대기만 초기화
 */
void GameManager::on_waiting_timer_expired(const boost::system::error_code& ec) {
    waiting_timer_active_ = false;

    if (ec) {
        // 타이머가 취소된 경우 등
        std::cout << "[GameManager] waiting_timer cancelled or error: " << ec.message() << "\n";
        return;
    }

    std::cout << "[GameManager] waiting_timer expired.\n";

    // 대기 인원 확인
    std::vector<std::shared_ptr<Player>> to_move;
    {
        std::lock_guard<std::mutex> lock(waiting_mutex_);
        if (!waiting_players_.empty()) {
            to_move = waiting_players_;
            waiting_players_.clear();
        }
    }

    if (!to_move.empty()) {
        // 모인 인원이 있다면 방 생성
        create_room(to_move);
    } else {
        // 아무도 없으면 그냥 끝
        std::cout << "[GameManager] No one was waiting.\n";
    }
}

/**
 * 실제로 새로운 Room을 만들고, 
 * to_move 플레이어를 그 Room으로 옮긴 뒤,
 * Room의 5초 카운트다운(start_game) 시작
 */
void GameManager::create_room(const std::vector<std::shared_ptr<Player>>& players_to_move) {
    // Room 생성
    auto new_room = std::make_shared<Room>(rooms_.size(), ioc_);
    {
        std::lock_guard<std::mutex> lock(room_mutex_);
        rooms_.push_back(new_room);
    }

    // to_move 플레이어를 Room에 추가
    for (auto& p : players_to_move) {
        bool ok = new_room->add_player(p);
        if (ok) {
            p->send_message("[ROOM] You have joined Room #" + std::to_string(new_room->id_));
        }
        else {
            p->send_message("[ROOM] Failed to join Room. (unexpected).");
        }
    }

    std::cout << "[GameManager] create_room - Room #" 
              << new_room->id_ << " with " << players_to_move.size() << " players.\n";

    // Room 내부에서 카운트다운 처리
    new_room->start_game();
}

void GameManager::delete_room(int room_id) {
    std::lock_guard<std::mutex> lock(room_mutex_);
    auto it = std::find_if(rooms_.begin(), rooms_.end(),
                           [room_id](auto& r){ return r->id_ == room_id; });
    if (it != rooms_.end()) {
        // 필요 시, 룸 만료 처리
        rooms_.erase(it);
        std::cout << "[GameManager] delete_room() - Room #" << room_id << " removed.\n";
    }
}

void GameManager::remove_player_from_room(std::shared_ptr<Player> player) {
    // 실제로 어떤 Room에 속해있는지 찾아서 제거
    if (auto room_ptr = player->room_.lock()) {
        room_ptr->remove_player(player);
    } else {
        // 대기 중인 플레이어일 수도 있으므로 waiting_players_에서도 제거
        std::lock_guard<std::mutex> lock(waiting_mutex_);
        auto it = std::find(waiting_players_.begin(), waiting_players_.end(), player);
        if (it != waiting_players_.end()) {
            waiting_players_.erase(it);
        } else {
            std::cerr << "[WARNING] remove_player_from_room: player not found.\n";
        }
    }
}