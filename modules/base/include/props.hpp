#pragma once

#if _WIN32
#define GET(slot, name) const inline decltype(slot) name() const { return slot; }
#define REF(slot, name) const inline decltype(slot)& name() const { return slot; }
#else
#define GET(slot, name) const inline __typeof(slot) name() const { return slot; }
#define REF(slot, name) const inline __typeof(slot)& name() const { return slot; }
#endif