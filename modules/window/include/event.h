#pragma once

#if __cplusplus
extern "C" {
#endif

typedef enum EventType {
	NONE_EVENT_TYPE,
	KEY_EVENT_TYPE,
	BUTTON_EVENT_TYPE,
	MOTION_EVENT_TYPE,
	TRANSFORM_EVENT_TYPE
} EventType;

typedef enum KeyState {
	STATE_RELEASED = 0,
	STATE_PRESSED = 1,
} KeyState;

typedef struct KeyEvent {
	EventType type;
	KeyState state;
	int key;
	char *txt;
} KeyEvent;

typedef struct ButtonEvent {
	EventType type;
	KeyState state;
	int button;
	int x, y;
} ButtonEvent;

typedef struct MotionEvent {
	EventType type;
	int x, y;
} MotionEvent;

typedef struct TransformEvent {
	EventType type;
	int x, y;
	int width, height;
} TransformEvent;

typedef union WindowEvent {
	EventType type;
	KeyEvent key;
	ButtonEvent button;
	MotionEvent motion;
	TransformEvent transform;
} WindowEvent;

#if __cplusplus
}
#endif
