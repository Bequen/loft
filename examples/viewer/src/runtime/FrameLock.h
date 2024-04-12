#pragma once

#include <cstdint>
#include <ctime>

/**
 * Utilities to lock maximum number of frames per second
 */
struct FrameLock {
private:
    uint64_t m_prevTimeInNS;
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
    m_prevTimeInNS(0), m_maxFrameTimeInNS(maxFramesPerSecond == 0 ? 0 : NS_IN_SECOND / maxFramesPerSecond) {
        
    }

    void update() {
        /* timespec currentTimeSpec = {};
        if(clock_gettime(CLOCK_MONOTONIC_RAW, &currentTimeSpec) == -1) {}
        uint64_t currentTimeInNS = (currentTimeSpec.tv_sec * NS_IN_SECOND) + currentTimeSpec.tv_nsec;

        m_deltaTime = currentTimeInNS - m_prevTimeInNS;
        if(m_deltaTime < m_maxFrameTimeInNS) {
            auto sleepTimeInNS = m_maxFrameTimeInNS - m_deltaTime;
            const timespec tim {
                    .tv_sec = 0,
                    .tv_nsec = (long)sleepTimeInNS
            };
            struct timespec tim2{};
            nanosleep(&tim, &tim2);
        }

        m_prevTimeInNS = currentTimeInNS; */
    }
};