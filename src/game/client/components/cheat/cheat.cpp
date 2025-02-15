#include "cheat.h"
#include "game/client/gameclient.h"

void CCheat::SetMousePosition(vec2 Position, bool Silent)
{
	if(Silent)
	{
		m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)Position.x;
		m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)Position.y;
	}
	else
	{
		m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy].x = Position.x;
		m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy].y = Position.y;
	}
}

void CCheat::RenderDebug()
{
	if(!g_Config.m_ZrDebug)
		return;

	// Save screen mapping
	float Points[4];
	Graphics()->GetScreen(Points, Points+1, Points+2, Points+3);

	// Map screen coordinates using camera's view
	float Width = 1200.0f;
	float Height = 800.0f;
	float ScreenX0 = m_pClient->m_Camera.m_Center.x - Width*m_pClient->m_Camera.m_Zoom/2;
	float ScreenY0 = m_pClient->m_Camera.m_Center.y - Height*m_pClient->m_Camera.m_Zoom/2;
	float ScreenX1 = m_pClient->m_Camera.m_Center.x + Width*m_pClient->m_Camera.m_Zoom/2;
	float ScreenY1 = m_pClient->m_Camera.m_Center.y + Height*m_pClient->m_Camera.m_Zoom/2;
	
	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);

	// Render debug info for each player
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CGameClient::CClientData *pClientData = &m_pClient->m_aClients[i];
		if(!pClientData->m_Active)
			continue;

		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_apPlayerInfos[i];
		if(!pInfo)
			continue;

		const CNetObj_Character *pChar = &m_pClient->m_Snap.m_aCharacters[i].m_Cur;
		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		vec2 Position = m_pClient->m_aClients[i].m_RenderPos;
		float FontSize = 8.0f * m_pClient->m_Camera.m_Zoom;
		float LineHeight = FontSize * 1.2f;
		float YOffset = -80.0f;

		struct DebugField {
			const char* label;
			char value[64];
		};

		DebugField Fields[] = {
			{"Name", {}},
			{"Tick", {}},
			{"Pos", {}},
			{"Vel", {}},
			{"Angle", {}},
			{"Direction", {}},
			{"Jumped", {}},
			{"Hook", {}},
			{"HookPos", {}},
			{"HookVel", {}},
			{"State", {}},
			{"Flags", {}}
		};
		const int NumFields = sizeof(Fields) / sizeof(Fields[0]);

		// Format all values
		str_format(Fields[0].value, sizeof(Fields[0].value), "%s [%d] Team: %d", 
			pClientData->m_aName, i, pClientData->m_Team);
		str_format(Fields[1].value, sizeof(Fields[1].value), "%d", pChar->m_Tick);
		str_format(Fields[2].value, sizeof(Fields[2].value), "%.1f, %.1f", Position.x, Position.y);
		str_format(Fields[3].value, sizeof(Fields[3].value), "%.1f, %.1f", 
			pChar->m_VelX/256.0f, pChar->m_VelY/256.0f);
		str_format(Fields[4].value, sizeof(Fields[4].value), "%.1f", pClientData->m_Angle);
		str_format(Fields[5].value, sizeof(Fields[5].value), "%d", pChar->m_Direction);
		str_format(Fields[6].value, sizeof(Fields[6].value), "%d", pChar->m_Jumped);

		// Hook state
		const char* pHookState;
		switch(pChar->m_HookState)
		{
			case HOOK_RETRACTED: pHookState = "RETRACTED"; break;
			case HOOK_IDLE: pHookState = "IDLE"; break;
			case HOOK_RETRACT_START: pHookState = "RETRACT_START"; break;
			case HOOK_RETRACT_END: pHookState = "RETRACT_END"; break;
			case HOOK_FLYING: pHookState = "FLYING"; break;
			case HOOK_GRABBED: pHookState = "GRABBED"; break;
			default: pHookState = "UNKNOWN"; break;
		}
		str_format(Fields[7].value, sizeof(Fields[7].value), "%s%s:%d", 
			pHookState,
			pChar->m_HookedPlayer >= 0 ? " (Player)" : "",
			pChar->m_HookTick);
		str_format(Fields[8].value, sizeof(Fields[8].value), "%d, %d", 
			pChar->m_HookX, pChar->m_HookY);
		str_format(Fields[9].value, sizeof(Fields[9].value), "%d, %d", 
			pChar->m_HookDx, pChar->m_HookDy);

		// Player state
		str_format(Fields[10].value, sizeof(Fields[10].value), "%s%s%s%s%s", 
			pClientData->m_DeepFrozen ? "Deep " : "",
			pClientData->m_LiveFrozen ? "Live " : "",
			pClientData->m_Solo ? "Solo " : "",
			pClientData->m_Jetpack ? "Jet " : "",
			pClientData->m_FreezeEnd ? "Frozen" : "");

		// Player flags
		str_format(Fields[11].value, sizeof(Fields[11].value), "%s%s%s%s%s", 
			pClientData->m_Friend ? "Friend " : "",
			pClientData->m_Afk ? "AFK " : "",
			pClientData->m_Paused ? "Paused " : "",
			pClientData->m_Spec ? "Spec " : "",
			pClientData->m_Active ? "Active" : "");

		// Render all fields
		char aBuf[256];
		for(int j = 0; j < NumFields; j++)
		{
			str_format(aBuf, sizeof(aBuf), "%s: %s", Fields[j].label, Fields[j].value);
			Ui()->TextRender()->Text(Position.x, Position.y + YOffset, FontSize, aBuf, -1.0f);
			YOffset += LineHeight;
		}
	}

	// Restore original screen mapping
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
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

	RenderDebug();
}