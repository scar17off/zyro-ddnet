#ifndef GAME_CLIENT_COMPONENTS_CHEAT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_H

#include <game/client/component.h>
#include <engine/console.h>

class CCheat : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;
    virtual void OnConsoleInit() override;

    void SetMousePosition(vec2 Position, bool Silent = false);
    void RenderDebug();

    static void ConToggle(IConsole::IResult *pResult, void *pUserData);
    static void ConSetPass(IConsole::IResult *pResult, void *pUserData);
    bool m_Active = true;
    std::string m_UnloadPassword = "zyro";

private:
};

#endif