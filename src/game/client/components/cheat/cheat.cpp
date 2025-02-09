#include "cheat.h"

#include "game/client/gameclient.h"

void CCheat::SetMousePosition(vec2 Position)
{
    if(g_Config.m_ZrAimbotMode == 0)
    {
        m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy].x = Position.x;
        m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy].y = Position.y;
    }
    else if(g_Config.m_ZrAimbotMode == 1)
    {
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)Position.x;
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)Position.y;
    }
}