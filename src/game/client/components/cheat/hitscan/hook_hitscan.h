#ifndef GAME_CLIENT_COMPONENTS_CHEAT_HITSCAN_HOOK_HITSCAN_H
#define GAME_CLIENT_COMPONENTS_CHEAT_HITSCAN_HOOK_HITSCAN_H

#include <base/vmath.h>
#include <game/client/component.h>
#include <vector>

class CHookHitscan : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }

    vec2 EdgeScan(int ClientId);

private:
    bool HitScanHook(vec2 initPos, vec2 targetPos, vec2 scanDir);
    bool IntersectCharacter(vec2 hookPos, vec2 targetPos, vec2 &newPos);
};

#endif