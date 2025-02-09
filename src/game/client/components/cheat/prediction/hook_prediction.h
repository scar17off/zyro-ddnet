#ifndef GAME_CLIENT_COMPONENTS_CHEAT_PREDICTION_HOOK_PREDICTION_H
#define GAME_CLIENT_COMPONENTS_CHEAT_PREDICTION_HOOK_PREDICTION_H

#include <base/vmath.h>
#include <game/client/component.h>

class CHookPrediction : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }

    bool PredictHook(vec2 &myPos, vec2 myVel, vec2 &targetPos, vec2 targetVel);
    float GetPing() const;
};

#endif