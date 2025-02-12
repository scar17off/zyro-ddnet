#ifndef GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_SPECTATORLIST_H
#define GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_SPECTATORLIST_H

#include <game/client/component.h>
#include <base/color.h>
#include <vector>

struct CNetObj_PlayerInfo;

class CSpectatorList : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnRender() override;
    virtual void OnReset() override;

private:
    bool m_Active;
    float m_Width;
    float m_Height;
    float m_X;
    float m_Y;
    float m_Margin;
    float m_EntryHeight;
    float m_TextSize;
    void GetSpectators(std::vector<const CNetObj_PlayerInfo*>& vSpectators) const;
    void GetPausedPlayers(std::vector<const CNetObj_PlayerInfo*>& vPausedPlayers) const;
    void DrawPausedIcon(float X, float Y, float Size, const ColorRGBA& Color, bool IsSpectator) const;
};

#endif
