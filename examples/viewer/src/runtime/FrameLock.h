#pragma once

#include <cstdint>
#include <chrono>

/**
 * Utilities to lock maximum number of frames per second
 */
struct FrameLock {
private:
    std::chrono::time_point<std::chrono::system_clock> m_prevTimeInNS;
    uint64_t m_maxFrameTimeInNS;

    uint64_t m_deltaTime;

    static const uint64_t NS_IN_SECOND = 1000000000UL;

public:
    inline const uint64_t delta_time() const {
        return m_deltaTime;
    }

    inline constexpr uint64_t fps() const {
        return NS_IN_SECOND / m_deltaTime;
    }

    explicit FrameLock(uint64_t maxFramesPerSecond) :
    m_maxFrameTimeInNS(maxFramesPerSecond == 0 ? 0 : NS_IN_SECOND / maxFramesPerSecond) {
        
    }

    void update() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = duration_cast<std::chrono::nanoseconds>(now - m_prevTimeInNS);
        m_deltaTime = duration.count();

        if(m_deltaTime < m_maxFrameTimeInNS) {
            auto sleepTimeInNS = m_maxFrameTimeInNS - m_deltaTime;
            const timespec tim {
                    .tv_sec = 0,
                    .tv_nsec = (long)sleepTimeInNS
            };
            struct timespec tim2{};
            nanosleep(&tim, &tim2);
        }

        m_prevTimeInNS = now;
    }
};