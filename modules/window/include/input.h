#include "event.h"

inline float get_scroll_y(const ButtonEvent *pEvent) {
	if(pEvent->button == 4) {
		return 1.0f;
	} else if(pEvent->button == 5) {
		return -1.0f;
	} else {
		return 0;
	}
}
