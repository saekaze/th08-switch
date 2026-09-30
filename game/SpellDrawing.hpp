#pragma once
#include "EclVm.hpp"
#include "AnmExecutor.hpp"
#include "AnmRenderer.hpp"
namespace th08 {
// Original 004178c0: overlapping portraits, name panels and the spell bonus /
// capture-history digits use the same VMs advanced by SpellSystem.
class SpellDrawing {
    EclGlobals& state;const SpellRecord* records;AnmRenderer& renderer;
    bool digit(i32 value);
public:
    AnmLoaded* digits=nullptr;
    SpellDrawing(EclGlobals& s,const SpellRecord* r,AnmRenderer& a):state(s),records(r),renderer(a){}
    bool draw();
};
}
