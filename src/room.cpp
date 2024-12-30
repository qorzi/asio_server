#include "room.hpp"
#include "player.hpp"
#include <thread>

Room::Room(int id) : id(id) {
    initialize_maps(); // 맵 초기화 호출출
}

void Room::initialize_maps() {
    // 맵 생성
    auto mapA = std::make_shared<Map>("A", 300, 300);
    mapA->start_point = {1, 1};

    auto mapB = std::make_shared<Map>("B", 300, 300);
    mapB->start_point = {1, 1};

    auto mapC = std::make_shared<Map>("C", 300, 300);
    mapC->start_point = {1, 1};
    mapC->end_point = {299, 299};

    maps = {mapA, mapB, mapC};

    // 포탈 생성 및 연결
    portal_links[mapA->generate_random_portal(mapB->name)] = mapB;
    portal_links[mapB->generate_random_portal(mapC->name)] = mapC;
}

bool Room::add_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_); // 뮤텍스 잠금금
    if (is_full()) {
        return false;
    }
    players.push_back(player);
    player->room = shared_from_this();
    player->current_map = maps[0]; // 첫 번째 맵으로 설정정
    player->position = maps[0]->start_point;
    return true;
}

void Room::remove_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_); // 뮤텍스 잠금
    players.erase(std::remove(players.begin(), players.end(), player), players.end());
}

bool Room::is_full() const {
    return players.size() >= 30;
}

const std::vector<std::shared_ptr<Player>>& Room::get_players() const {
    return players;
}

void Room::start_timer(std::function<void(int)> on_timer_expired, 
                       int wait_time_seconds, 
                       int check_interval_seconds)
{
    // 1) `timer_active`를 원자적으로 체크 및 업데이트
    bool expected = false;
    if (!timer_active.compare_exchange_strong(expected, true)) {
        // 다른 스레드에서 이미 실행 중
        return;
    }

    should_stop_ = false; // 타이머 중지 플래그 초기화

    // 2) 타이머 스레드 실행
    std::thread([this, on_timer_expired, wait_time_seconds, check_interval_seconds]() {
        std::unique_lock<std::mutex> lock(mutex_);

        int elapsed_time = 0;

        while (true) {
            // `should_stop_`이 true가 되거나, 타임아웃까지 대기
            cv_.wait_for(lock, 
                         std::chrono::seconds(check_interval_seconds), 
                         [this]() { return should_stop_; });

            // 중지 요청이 들어오면 루프 종료
            if (should_stop_) {
                break;
            }

            // 경과 시간 업데이트
            elapsed_time += check_interval_seconds;

            // `players` 상태 확인 - 방에 플레이어가 없으면 만료 처리리
            if (players.empty()) {
                on_timer_expired(id); // 타이머 만료 처리
                break;
            }

            // 타이머 초과 시 처리
            if (elapsed_time >= wait_time_seconds) {
                on_timer_expired(id); // 타이머 만료 처리
                break;
            }
        }

        // 타이머 비활성화
        timer_active.store(false, std::memory_order_relaxed);
    }).detach();
}

void Room::stop_timer() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        should_stop_ = true; // 타이머 중지 플래그 설정
    }
    cv_.notify_all(); // 대기 중인 스레드 깨우기
}

const bool Room::is_timer_active() const {
    return timer_active.load(std::memory_order_relaxed); // 원자적 읽기
}
