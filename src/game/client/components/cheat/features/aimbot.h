#ifndef GAME_CLIENT_COMPONENTS_CHEAT_AIMBOT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_AIMBOT_H

#include <game/client/component.h>

class CAimbot : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;

    float GetWeaponReach(int Weapon);
    int SearchTarget();
    bool InFoV(vec2 Position);

    int GetCurrentTarget() const { return m_CurrentTarget; }
    void ResetTarget() { m_CurrentTarget = -1; }

private:
    int m_CurrentTarget = -1;
};

#endif