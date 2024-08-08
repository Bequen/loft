//
// Created by martin on 7/21/24.
//

#ifndef LOFT_RENDERGRAPHEVENT_H
#define LOFT_RENDERGRAPHEVENT_H

#include <string>

/**
 * Acts as a validity dependency for render graph command buffer recording.
 * When instance is invalidated, the render graph should rebuild all the render passes that depend on the instance.
 */
class RecordingDependency {
public:
    RecordingDependency(std::string name);
};

#endif //LOFT_RENDERGRAPHEVENT_H
