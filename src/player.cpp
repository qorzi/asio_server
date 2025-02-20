#include "player.hpp"
#include "connection_manager.hpp"
#include <cmath>

std::atomic<uint64_t> Player::id_counter_{0};

Player::Player(const std::string& name)
    : name_(name)
    , total_distance_(0)
{
    uint64_t new_id = id_counter_.fetch_add(1, std::memory_order_relaxed) + 1;

    std::ostringstream oss;
    oss << std::setw(12) << std::setfill('0') << new_id; // 12자리, 앞에 0 채우기
    id_ = oss.str();
}

/**
 * 플레이어 위치 업데이트
 */
void Player::update_position(const Point& new_position)
{
    position_ = new_position;
    total_distance_++;
}


/**
 * 플레이어 이동 범위 확인
 * 플레이어는 한번에 1칸만 이동할 수 있음
 */
bool Player::is_valid_position(const Point& pos) const {
    // x와 y 좌표의 차이를 계산
    int dx = std::abs(pos.x - position_.x);
    int dy = std::abs(pos.y - position_.y);

    // 상하좌우 한 칸만 이동했는지 확인
    return dx + dy == 1;
}


/**
 * 플레이어 전용 브로드캐스트
 * 플레이어의 커넥션을 찾아, 메시지 전송
 */
void Player::send_message(const std::string& message)
{
    auto conn = ConnectionManager::get_instance().get_connection_for_player(shared_from_this());
    if(conn){
        conn->async_write(message);
        std::cout << "[Player:" << id_ << "] => message:" << message << std::endl;
    } else {
        std::cerr << "[Player:" << id_ << "] No connection found, cannot send.\n";
    }
}
