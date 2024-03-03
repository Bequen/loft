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

    static const uint64_t NS_IN_SECOND = 1000000000UL;

public:
    explicit FrameLock(uint64_t maxFramesPerSecond) :
    m_prevTimeInNS(0), m_maxFrameTimeInNS(NS_IN_SECOND / maxFramesPerSecond) {

    }

    void update() {
        timespec currentTimeSpec = {};
        if(clock_gettime(CLOCK_MONOTONIC_RAW, &currentTimeSpec) == -1) {}
        uint64_t currentTimeInNS = (currentTimeSpec.tv_sec * NS_IN_SECOND) + currentTimeSpec.tv_nsec;

        uint64_t deltaTime = currentTimeInNS - m_prevTimeInNS;
        if(deltaTime < m_maxFrameTimeInNS) {
            auto sleepTimeInNS = m_maxFrameTimeInNS - deltaTime;
            const timespec tim {
                    .tv_sec = 0,
                    .tv_nsec = (long)sleepTimeInNS
            };
            struct timespec tim2{};
            nanosleep(&tim, &tim2);
        }

        m_prevTimeInNS = currentTimeInNS;
    }
};