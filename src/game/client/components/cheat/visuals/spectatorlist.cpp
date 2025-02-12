#include "spectatorlist.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>

void CSpectatorList::OnInit()
{
    OnReset();
}

void CSpectatorList::OnReset()
{
    m_Width = 150.0f;
    m_X = 5.0f;
    m_Margin = 3.0f;
    m_EntryHeight = 16.0f;
    m_TextSize = 10.0f;
}

void CSpectatorList::GetSpectators(std::vector<const CNetObj_PlayerInfo*>& vSpectators) const
{
    for(const CNetObj_PlayerInfo *pInfo : m_pClient->m_Snap.m_apInfoByName)
    {
        if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
            continue;
        vSpectators.push_back(pInfo);
    }
}

void CSpectatorList::GetPausedPlayers(std::vector<const CNetObj_PlayerInfo*>& vPausedPlayers) const
{
    for(const CNetObj_PlayerInfo *pInfo : m_pClient->m_Snap.m_apInfoByName)
    {
        if(!pInfo || pInfo->m_Team == TEAM_SPECTATORS)
            continue;
            
        if(m_pClient->m_aClients[pInfo->m_ClientId].m_Paused)
            vPausedPlayers.push_back(pInfo);
    }
}

void CSpectatorList::DrawPausedIcon(float X, float Y, float Size, const ColorRGBA& Color, bool IsSpectator) const
{
    const char *pText = IsSpectator ? "S" : "P";
    const float TextSize = Size;
    const float BarHeight = Size * 0.7f;
    const float TextWidth = Ui()->TextRender()->TextWidth(TextSize, pText, -1, -1.0f);
    const float BarWidth = Size * 0.2f;
    
    // Draw letter left of the left bar
    Ui()->TextRender()->Text(X - BarWidth*1.2f - TextWidth - 2.0f, Y - TextSize/2, TextSize, pText, -1.0f);

    // Pause icon (two vertical bars)
    Graphics()->TextureClear();
    Graphics()->QuadsBegin();
    Graphics()->SetColor(Color);
    
    // Left bar
    IGraphics::CQuadItem QuadLeft(X - BarWidth*1.2f, Y - BarHeight/2, BarWidth, BarHeight);
    Graphics()->QuadsDrawTL(&QuadLeft, 1);
    
    // Right bar
    IGraphics::CQuadItem QuadRight(X + BarWidth*0.2f, Y - BarHeight/2, BarWidth, BarHeight);
    Graphics()->QuadsDrawTL(&QuadRight, 1);
    
    Graphics()->QuadsEnd();
}

void CSpectatorList::OnRender()
{
    if(!g_Config.m_ZrSpecList)
        return;

    // Save current screen mapping
    float Points[4];
    Graphics()->GetScreen(Points, Points+1, Points+2, Points+3);

    // Map screen coordinates
    float ScreenWidth = Graphics()->ScreenWidth();
    float ScreenHeight = Graphics()->ScreenHeight();
    Graphics()->MapScreen(0.0f, 0.0f, 300.0f * Graphics()->ScreenAspect(), 300.0f);

    // Get UI scale
    float Scale = Ui()->PixelSize();
    float Width = m_Width * Scale;
    float EntryHeight = m_EntryHeight * Scale;
    float X = m_X * Scale;
    float Margin = m_Margin * Scale;
    float TextSize = m_TextSize * Scale;

    // Count total players
    std::vector<const CNetObj_PlayerInfo*> vSpectators;
    std::vector<const CNetObj_PlayerInfo*> vPausedPlayers;
    GetSpectators(vSpectators);
    GetPausedPlayers(vPausedPlayers);
    int TotalPlayers = vSpectators.size() + vPausedPlayers.size();
    if(TotalPlayers == 0) TotalPlayers = 1; // For "No spectators" text

    // Calculate height based on entries
    float TitleHeight = EntryHeight;
    float ContentHeight = TotalPlayers * EntryHeight;
    float MaxContentHeight = Graphics()->ScreenHeight() * 0.5f - TitleHeight;
    float Height = TitleHeight + std::min(ContentHeight, MaxContentHeight);
    float Y = 150.0f - Height/2;

    // Background
    CUIRect MainView = {X, Y, Width, Height};
    MainView.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f), IGraphics::CORNER_ALL, 3.0f);

    // Title
    CUIRect Title;
    MainView.HSplitTop(TitleHeight, &Title, &MainView);
    Title.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_T, 3.0f);
    Ui()->TextRender()->Text(Title.x + Margin, Title.y + (Title.h - TextSize) / 2, TextSize, "Spectators", -1.0f);

    // List spectators and paused players
    CUIRect Entry;
    int NumPlayers = 0;

    for(const CNetObj_PlayerInfo *pInfo : vSpectators)
    {
        MainView.HSplitTop(EntryHeight, &Entry, &MainView);
        Entry.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, NumPlayers % 2 ? 0.2f : 0.25f), IGraphics::CORNER_NONE, 0.0f);

        // Render spectator name
        Ui()->TextRender()->Text(Entry.x + Margin, Entry.y + (Entry.h - TextSize) / 2, TextSize, 
            m_pClient->m_aClients[pInfo->m_ClientId].m_aName, -1.0f);

        // Draw paused icon on right side
        DrawPausedIcon(Entry.x + Entry.w - Margin - TextSize/2, Entry.y + Entry.h/2, TextSize, ColorRGBA(1,1,1,0.5f), true);
        NumPlayers++;
    }

    for(const CNetObj_PlayerInfo *pInfo : vPausedPlayers)
    {
        MainView.HSplitTop(EntryHeight, &Entry, &MainView);
        Entry.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, NumPlayers % 2 ? 0.2f : 0.25f), IGraphics::CORNER_NONE, 0.0f);

        // Render paused player name
        Ui()->TextRender()->Text(Entry.x + Margin, Entry.y + (Entry.h - TextSize) / 2, TextSize,
            m_pClient->m_aClients[pInfo->m_ClientId].m_aName, -1.0f);

        // Draw paused icon on right side
        DrawPausedIcon(Entry.x + Entry.w - Margin - TextSize/2, Entry.y + Entry.h/2, TextSize, ColorRGBA(1,1,1,0.5f), false);
        NumPlayers++;
    }

    // Empty list text
    if(NumPlayers == 0)
    {
        MainView.HSplitTop(EntryHeight, &Entry, &MainView);
        Ui()->TextRender()->Text(Entry.x + Margin, Entry.y + (Entry.h - TextSize) / 2, TextSize, "No spectators", -1.0f);
    }

    // Restore original screen mapping
    Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}
