#ifndef GAME_CLIENT_COMPONENTS_CHEAT_AIMBOT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_AIMBOT_H

#include <game/client/component.h>

class CAimbot : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;

    int SearchTarget();
    bool InFoV(vec2 Position);
    float GetWeaponReach(int Weapon);

private:
};

#endif