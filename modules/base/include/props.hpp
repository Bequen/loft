#pragma once

#define GET(slot, name) const inline __typeof(slot) name() const { return slot; }
#define REF(slot, name) const inline __typeof(slot)& name() const { return slot; }
