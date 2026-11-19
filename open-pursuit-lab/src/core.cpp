#include "opl/core.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace opl {
namespace {
// RFC-1982-style half-range comparison. Sessions must reset before tick wrap.
bool newer(std::uint32_t a, std::uint32_t b) {
    const auto distance = a - b;
    return distance != 0 && distance < 0x80000000u;
}
void mix(std::uint64_t& hash, std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        hash ^= (value >> (8 * i)) & 0xffu;
        hash *= 1099511628211ull;
    }
}
} // namespace

std::uint32_t Random::next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
}

const char* to_string(PursuitState state) {
    switch (state) {
    case PursuitState::Patrol: return "Patrol";
    case PursuitState::Investigate: return "Investigate";
    case PursuitState::Chase: return "Chase";
    case PursuitState::Search: return "Search";
    case PursuitState::Cooldown: return "Cooldown";
    }
    return "Invalid";
}

void Pursuit::enter(PursuitState state) {
    state_ = state;
    elapsed_ = 0;
    unseen_ = 0;
}

void Pursuit::step(bool visible, std::uint8_t severity) {
    if (severity > 3) throw std::invalid_argument("incident severity must be 0..3");
    if (severity != 0) {
        heat_ = std::max(heat_, severity);
        if (visible) { enter(PursuitState::Chase); return; }
        if (state_ == PursuitState::Patrol || state_ == PursuitState::Cooldown) {
            enter(PursuitState::Investigate);
            return;
        }
    }
    // Saturation avoids overflow in arbitrarily long idle sessions.
    if (elapsed_ < std::numeric_limits<std::uint32_t>::max()) ++elapsed_;
    switch (state_) {
    case PursuitState::Patrol: break;
    case PursuitState::Investigate:
        if (visible) enter(PursuitState::Chase);
        else if (elapsed_ >= kInvestigateTicks) enter(PursuitState::Search);
        break;
    case PursuitState::Chase:
        if (visible) unseen_ = 0;
        else if (++unseen_ >= kLostSightTicks) enter(PursuitState::Search);
        break;
    case PursuitState::Search:
        if (visible) enter(PursuitState::Chase);
        else if (elapsed_ >= kSearchTicks) enter(PursuitState::Cooldown);
        break;
    case PursuitState::Cooldown:
        if (visible && heat_ != 0) enter(PursuitState::Chase);
        else if (elapsed_ >= kCooldownTicks) { heat_ = 0; enter(PursuitState::Patrol); }
        break;
    }
}

Traffic::Traffic(std::uint32_t seed, std::uint32_t count) {
    if (count == 0 || count > 100) throw std::invalid_argument("traffic count must be 1..100");
    Random random(seed);
    for (std::uint32_t i = 0; i < count; ++i) {
        cars_.push_back({i, static_cast<std::int32_t>((std::int64_t{i} * kLoopMm) / count),
                         0, 250 + static_cast<std::int32_t>(random.next() % 201)});
    }
}

void Traffic::step(bool green) {
    const auto previous = cars_; // Read-old/write-new; stable logical IDs.
    for (std::size_t i = 0; i < cars_.size(); ++i) {
        auto& next = cars_[i];
        const auto& car = previous[i];
        std::int32_t distance = kLoopMm;
        for (std::size_t j = 0; j < previous.size(); ++j) {
            if (i == j) continue;
            const auto gap = (previous[j].position_mm - car.position_mm + kLoopMm) % kLoopMm;
            distance = std::min(distance, gap);
        }
        auto speed = std::min({car.cruise_mm_tick, car.speed_mm_tick + 5,
                               std::max(0, distance - kSafeGapMm)});
        // Fixed single stop line; cars past the line may clear the junction.
        if (!green && car.position_mm <= kStopLineMm) {
            speed = std::min(speed, kStopLineMm - car.position_mm);
        }
        next.speed_mm_tick = speed;
        next.position_mm = (car.position_mm + speed) % kLoopMm;
    }
}

bool Traffic::safe_spacing() const {
    for (std::size_t i = 0; i < cars_.size(); ++i) {
        for (std::size_t j = 0; j < cars_.size(); ++j) {
            if (i == j) continue;
            const auto gap = (cars_[j].position_mm - cars_[i].position_mm + kLoopMm) % kLoopMm;
            if (gap < kSafeGapMm) return false;
        }
    }
    return true;
}

void Race::advance(std::int64_t old_mm, std::int64_t new_mm) {
    if (old_mm < 0 || new_mm < old_mm || new_mm - old_mm > 500) {
        throw std::invalid_argument("race requires bounded forward server movement (<=500 mm/tick)");
    }
    if (finished()) return;
    const std::int64_t next = (static_cast<std::int64_t>(passed_) + 1) * 250'000;
    if (old_mm < next && new_mm >= next) ++passed_;
}

const char* to_string(Reject result) {
    switch (result) {
    case Reject::None: return "accepted";
    case Reject::WrongOwner: return "wrong_owner";
    case Reject::InvalidRange: return "invalid_range";
    case Reject::WrongTick: return "wrong_tick";
    case Reject::StaleSequence: return "stale_sequence";
    case Reject::AlreadyQueued: return "already_queued";
    }
    return "invalid";
}

Reject Server::submit(std::uint32_t peer, const Command& command) {
    if (peer != owner_) return Reject::WrongOwner;
    if (command.throttle < 0 || command.throttle > 1000 || command.brake < 0 || command.brake > 1000)
        return Reject::InvalidRange;
    if (tick_ == std::numeric_limits<std::uint32_t>::max() || command.target_tick != tick_ + 1)
        return Reject::WrongTick;
    if (has_sequence_ && !newer(command.sequence, last_sequence_)) return Reject::StaleSequence;
    if (queued_) return Reject::AlreadyQueued;
    pending_ = command;
    last_sequence_ = command.sequence;
    has_sequence_ = true;
    queued_ = true;
    return Reject::None;
}

void Server::step(bool visible, std::uint8_t severity) {
    if (severity > 3) throw std::invalid_argument("incident severity must be 0..3");
    if (tick_ == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("session tick limit");
    ++tick_;
    // Missing command = neutral throttle plus drag; never reuse stale throttle.
    const auto throttle = queued_ ? pending_.throttle : 0;
    const auto brake = queued_ ? pending_.brake : 0;
    const auto acceleration = (throttle * 8) / 1000 - (brake * 20) / 1000 - 1;
    speed_ = std::clamp(speed_ + acceleration, 0, 500);
    const auto old = travel_mm_;
    travel_mm_ += speed_;
    race_.advance(old, travel_mm_);
    pursuit_.step(visible, severity);
    queued_ = false;
}

Snapshot Server::snapshot() const {
    return {tick_, travel_mm_, speed_, pursuit_.state(), pursuit_.heat(), race_.passed()};
}

bool Observer::apply(const Snapshot& snapshot) {
    if (has_state_ && !newer(snapshot.tick, state_.tick)) return false;
    state_ = snapshot;
    has_state_ = true;
    return true;
}

SnapshotLink::SnapshotLink(std::uint32_t seed, LinkConfig config) : random_(seed), config_(config) {
    if (config.loss_per_mille > 1000 || config.duplicate_per_mille > 1000 ||
        config.delay_ticks > 600 || config.jitter_ticks > 600)
        throw std::invalid_argument("link probabilities 0..1000 and delays 0..600 ticks");
}

void SnapshotLink::send(std::uint32_t now, const Snapshot& snapshot) {
    ++sent_;
    if (random_.next() % 1000 < config_.loss_per_mille) { ++dropped_; return; }
    const auto jitter = random_.next() % (config_.jitter_ticks + 1);
    const auto due = std::uint64_t{now} + config_.delay_ticks + jitter;
    packets_.push_back({due, snapshot});
    if (random_.next() % 1000 < config_.duplicate_per_mille) {
        ++duplicated_;
        packets_.push_back({due + 1, snapshot});
    }
}

std::vector<Snapshot> SnapshotLink::receive(std::uint32_t now) {
    std::vector<Snapshot> result;
    auto i = packets_.begin();
    while (i != packets_.end()) {
        if (i->due <= now) { result.push_back(i->snapshot); i = packets_.erase(i); }
        else ++i;
    }
    return result;
}

std::uint64_t hash_snapshot(const Snapshot& state) {
    std::uint64_t hash = 14695981039346656037ull;
    mix(hash, state.tick); mix(hash, static_cast<std::uint64_t>(state.travel_mm));
    mix(hash, static_cast<std::uint32_t>(state.speed_mm_tick));
    mix(hash, static_cast<std::uint8_t>(state.pursuit)); mix(hash, state.heat); mix(hash, state.checkpoints);
    return hash;
}

std::uint64_t hash_traffic(const Traffic& traffic) {
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto& car : traffic.cars()) {
        mix(hash, car.id); mix(hash, static_cast<std::uint32_t>(car.position_mm));
        mix(hash, static_cast<std::uint32_t>(car.speed_mm_tick));
        mix(hash, static_cast<std::uint32_t>(car.cruise_mm_tick));
    }
    return hash;
}

std::uint64_t replay_digest(std::uint32_t seed, std::uint32_t ticks) {
    Traffic traffic(seed);
    Server server;
    std::uint64_t digest = 14695981039346656037ull;
    for (std::uint32_t t = 0; t < ticks; ++t) {
        const Command command{t + 1, t + 1, static_cast<std::int16_t>((t % 300 < 240) ? 1000 : 0), 0};
        if (server.submit(7, command) != Reject::None) throw std::logic_error("fixture input rejected");
        server.step(t >= 60 && t < 300, t == 60 ? 3 : 0);
        traffic.step(t % 600 < 420);
        mix(digest, hash_snapshot(server.snapshot()));
        mix(digest, hash_traffic(traffic));
    }
    return digest;
}

} // namespace opl
