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

void CCheat::OnRender()
{
	if(!g_Config.m_ZrPlrBox && !g_Config.m_ZrPlrTracer)
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// Draw esp visuals on players
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_aClients[i].m_Active || i == m_pClient->m_Snap.m_LocalClientId)
			continue;

		vec2 Position = m_pClient->m_aClients[i].m_RenderPos;
		float Size = CCharacterCore::PhysicalSize() * 2;

		// Red for aimbot target, yellow for others
		ColorRGBA Color = (i == m_pClient->m_Aimbot.GetCurrentTarget()) 
			? ColorRGBA(1,0,0,0.3f)   // Red
			: ColorRGBA(1,1,0,0.3f);  // Yellow

		// Draw box
		if(g_Config.m_ZrPlrBox)
		{
			m_pClient->m_Visuals.RenderBox(
				Position.x - Size/2, // x
				Position.y - Size/2, // y
				Size,                // width
				Size,                // height
				true,                // fill
				false,               // stroke
				Color,               // fill color
				Color                // stroke color
			);
		}

		// Draw tracer
		if(g_Config.m_ZrPlrTracer)
		{
			vec2 LocalPos = m_pClient->m_LocalCharacterPos;
			m_pClient->m_Visuals.DrawLine(
				LocalPos.x, LocalPos.y, // from
				Position.x, Position.y, // to
				1.0f,                   // width
				Color                   // color
			);
		}
	}
}