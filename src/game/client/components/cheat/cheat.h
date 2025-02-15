#ifndef GAME_CLIENT_COMPONENTS_CHEAT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_H

#include <game/client/component.h>

class CCheat : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;

    void SetMousePosition(vec2 Position, bool Silent = false);

private:
};

#endif