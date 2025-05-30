#include <game/localization.h>

#include <game/client/components/cheat/features/aimbot.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>

#include "menus.h"

static int s_CurTab = 0;
static int s_CurAimbotTab = -1;
enum
{
	ZYRO_TAB_PAGE_1 = 0,
	NUMBER_OF_ZYRO_TABS
};

enum
{
	WEAPON_TAB_HAMMER = 0,
	WEAPON_TAB_GUN,
	WEAPON_TAB_SHOTGUN,
	WEAPON_TAB_GRENADE,
	WEAPON_TAB_LASER,
	WEAPON_TAB_NINJA,
	WEAPON_TAB_HOOK,
	NUM_WEAPON_TABS
};

int CMenus::RenderTablist(CUIRect TabBar)
{
	// Tab button containers
	static CButtonContainer s_aPageTabs[NUMBER_OF_ZYRO_TABS] = {};
	static int s_CurZyroTab = 0;

	// Calculate tab width
	float TabWidth = TabBar.w / NUMBER_OF_ZYRO_TABS;
	CUIRect Tab;

	// Render tab buttons
	for(int i = 0; i < NUMBER_OF_ZYRO_TABS; i++)
	{
		TabBar.VSplitLeft(TabWidth, &Tab, &TabBar);

		// Calculate corners for rounded edges
		int Corners = 0;
		if(i == 0)
			Corners |= IGraphics::CORNER_L;
		if(i == NUMBER_OF_ZYRO_TABS - 1)
			Corners |= IGraphics::CORNER_R;

		// Get tab text
		const char *pTabText = "";
		switch(i)
		{
		case ZYRO_TAB_PAGE_1: pTabText = "Page 1"; break;
		}

		if(DoButton_MenuTab(&s_aPageTabs[i], Localize(pTabText), s_CurZyroTab == i, &Tab, Corners))
			s_CurZyroTab = i;
	}

	return s_CurZyroTab;
}

void CMenus::RenderTabPage1(CUIRect MainView)
{
	// Split MainView to use only 1/4 of width
	CUIRect SettingsView;
	MainView.VSplitLeft(MainView.w / 4, &SettingsView, nullptr);

	CUIRect Button, Right, Row, Label;
	const float LineSize = 20.0f;
	const float Spacing = 2.0f;
	const float CheckboxWidth = 80.0f;
	const float SliderWidth = 150.0f;

	// Calculate weapon section height
	const float MaxWeaponHeight = 96.0f * 0.3f;
	const float CheckboxSize = 20.0f;
	const float CheckboxSpacing = 5.0f;
	const float TotalWeaponSectionHeight = MaxWeaponHeight + CheckboxSize + CheckboxSpacing;

	// Create weapons section
	CUIRect WeaponsSection;
	SettingsView.HSplitTop(MaxWeaponHeight, &WeaponsSection, &SettingsView);

	// Calculate weapon widths and spacing
	const int NumWeapons = 7;
	float WeaponWidths[7];
	float TotalWeaponsWidth = 0;
	const float MinSpacingFactor = 0.2f;

	for(int i = 0; i < NumWeapons; i++)
	{
		float Width;
		switch(i)
		{
		case 0: Width = 128.0f; break; // Hook
		case 1: Width = 128.0f; break; // Hammer
		case 2: Width = 128.0f; break; // Gun
		case 3: Width = 256.0f; break; // Shotgun
		case 4: Width = 256.0f; break; // Grenade
		case 5: Width = 224.0f; break; // Laser
		case 6: Width = 256.0f; break; // Ninja
		}
		WeaponWidths[i] = Width * 0.3f;
		TotalWeaponsWidth += WeaponWidths[i];
	}

	// Calculate minimum spacing based on widest weapon
	float MaxWeaponWidth = 0;
	for(int i = 0; i < NumWeapons; i++)
		MaxWeaponWidth = std::max(MaxWeaponWidth, WeaponWidths[i]);

	float MinSpacing = MaxWeaponWidth * MinSpacingFactor;
	float DesiredSpacing = (WeaponsSection.w - TotalWeaponsWidth) / (NumWeapons + 1);
	float WeaponSpacing = MinSpacing;
	float CurrentX = WeaponsSection.x;

	CUIRect WeaponRect = WeaponsSection;

	for(int i = 0; i < NumWeapons; i++)
	{
		WeaponRect.x = CurrentX;
		WeaponRect.w = WeaponWidths[i];

		switch(i)
		{
		case 0: RenderWeaponSection(WeaponRect, 6, WEAPON_TAB_HOOK, "Hook"); break;
		case 1: RenderWeaponSection(WeaponRect, WEAPON_HAMMER, WEAPON_TAB_HAMMER, "Hammer"); break;
		case 2: RenderWeaponSection(WeaponRect, WEAPON_GUN, WEAPON_TAB_GUN, "Gun"); break;
		case 3: RenderWeaponSection(WeaponRect, WEAPON_SHOTGUN, WEAPON_TAB_SHOTGUN, "Shotgun"); break;
		case 4: RenderWeaponSection(WeaponRect, WEAPON_GRENADE, WEAPON_TAB_GRENADE, "Grenade"); break;
		case 5: RenderWeaponSection(WeaponRect, WEAPON_LASER, WEAPON_TAB_LASER, "Laser"); break;
		case 6: RenderWeaponSection(WeaponRect, WEAPON_NINJA, WEAPON_TAB_NINJA, "Ninja"); break;
		}

		CurrentX += WeaponWidths[i] + WeaponSpacing;
	}

	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);

	// Show weapon-specific settings when a weapon is selected
	if(s_CurAimbotTab != -1)
	{
		int WeaponId = s_CurAimbotTab;
		const WeaponConfig *pConfig = m_pClient->m_Aimbot.GetWeaponConfig(WeaponId);

		// Create row for aim
		SettingsView.HSplitTop(LineSize, &Row, &SettingsView);

		// Aim
		Row.VSplitLeft(CheckboxWidth, &Button, &Right);
		if(DoButton_CheckBox(pConfig->m_pEnabled, Localize("aim"), *pConfig->m_pEnabled, &Button))
			*pConfig->m_pEnabled ^= 1;

		// Silent
		Right.VSplitLeft(5.0f, nullptr, &Right);
		Right.VSplitLeft(CheckboxWidth, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ZrAimbotSilent, Localize("silent"), g_Config.m_ZrAimbotSilent, &Button))
			g_Config.m_ZrAimbotSilent ^= 1;

		// Draw FOV
		Right.VSplitLeft(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ZrAimbotDrawFov, "", g_Config.m_ZrAimbotDrawFov, &Button))
			g_Config.m_ZrAimbotDrawFov ^= 1;

		// Global FOV slider
		Right.VSplitLeft(5.0f, nullptr, &Right);
		Right.VSplitLeft(SliderWidth, &Button, &Right);
		Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotFoV, &g_Config.m_ZrAimbotFoV, &Button, 
			Localize("fov"), 1, 360, &CUi::ms_LinearScrollbarScale, 
			CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "°");

		// Aimbot mode dropdown
		Right.VSplitLeft(20.0f, nullptr, &Right);
		Right.VSplitLeft(80.0f, &Button, &Right);
		const char *apAimbotModes[] = {"Plain", "Smooth"};
		static CUi::SDropDownState s_AimbotModeDropDownState;
		static CScrollRegion s_AimbotModeDropDownScrollRegion;
		s_AimbotModeDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_AimbotModeDropDownScrollRegion;
		g_Config.m_ZrAimbotMode = Ui()->DoDropDown(&Button, g_Config.m_ZrAimbotMode, apAimbotModes, std::size(apAimbotModes), s_AimbotModeDropDownState);

		// Show smooth factor slider if smooth mode is selected
		if(g_Config.m_ZrAimbotMode == 1)
		{
			Right.VSplitLeft(20.0f, nullptr, &Right);
			Right.VSplitLeft(SliderWidth, &Button, &Right);
			Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotSmooth, &g_Config.m_ZrAimbotSmooth, &Button, 
				Localize("smooth"), 1, 100, &CUi::ms_LinearScrollbarScale,
				CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");
		}

		// Accuracy settings for specific weapons
		if(WeaponId == WEAPON_TAB_HOOK) // Hook
		{
			SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
			SettingsView.HSplitTop(LineSize, &Row, &SettingsView);

			// Accuracy
			Row.VSplitLeft(SliderWidth, &Button, &Right);
			Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotHookAccuracy, &g_Config.m_ZrAimbotHookAccuracy, &Button,
				Localize("accuracy"), 1, 200, &CUi::ms_LinearScrollbarScale,
				CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");
		}
		else if(WeaponId == WEAPON_GRENADE)
		{
			SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
			SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
			
			// Accuracy slider
			Row.VSplitLeft(SliderWidth, &Button, &Right);
			Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotGrenadeAccuracy, &g_Config.m_ZrAimbotGrenadeAccuracy, &Button,
				Localize("accuracy"), 1, 200, &CUi::ms_LinearScrollbarScale,
				CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");

			// Splash checkbox
			Right.VSplitLeft(20.0f, nullptr, &Right);
			Right.VSplitLeft(60.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ZrAimbotGrenadeSplash, Localize("splash"), g_Config.m_ZrAimbotGrenadeSplash, &Button))
				g_Config.m_ZrAimbotGrenadeSplash ^= 1;

			// Path selection dropdown
			Right.VSplitLeft(20.0f, nullptr, &Right);
			Right.VSplitLeft(80.0f, &Button, &Right);
			const char *apGrenadePaths[] = {"closest", "furthest", "random"};
			static CUi::SDropDownState s_GrenadePathDropDownState;
			static CScrollRegion s_GrenadePathDropDownScrollRegion;
			s_GrenadePathDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_GrenadePathDropDownScrollRegion;
			g_Config.m_ZrAimbotGrenadePath = Ui()->DoDropDown(&Button, g_Config.m_ZrAimbotGrenadePath, apGrenadePaths, std::size(apGrenadePaths), s_GrenadePathDropDownState);

			// Length selection dropdown
			Right.VSplitLeft(20.0f, nullptr, &Right);
			Right.VSplitLeft(80.0f, &Button, &Right);
			const char *apGrenadeLengths[] = {"shortest", "longest"};
			static CUi::SDropDownState s_GrenadeLengthDropDownState;
			static CScrollRegion s_GrenadeLengthDropDownScrollRegion;
			s_GrenadeLengthDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_GrenadeLengthDropDownScrollRegion;
			g_Config.m_ZrAimbotGrenadeLength = Ui()->DoDropDown(&Button, g_Config.m_ZrAimbotGrenadeLength, apGrenadeLengths, std::size(apGrenadeLengths), s_GrenadeLengthDropDownState);
		}
		else if(WeaponId == WEAPON_LASER)
		{
			SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
			SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
			
			// Accuracy slider
			Row.VSplitLeft(SliderWidth, &Button, &Right);
			Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotLaserAccuracy, &g_Config.m_ZrAimbotLaserAccuracy, &Button,
				Localize("accuracy"), 1, 200, &CUi::ms_LinearScrollbarScale,
				CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");

			// Bounce count slider
			Right.VSplitLeft(20.0f, nullptr, &Right);
			Right.VSplitLeft(SliderWidth, &Button, &Right);
			Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotLaserBounces, &g_Config.m_ZrAimbotLaserBounces, &Button,
				Localize("bounces"), 0, 10, &CUi::ms_LinearScrollbarScale,
				CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

			// Path selection dropdown
			Right.VSplitLeft(20.0f, nullptr, &Right);
			Right.VSplitLeft(80.0f, &Button, &Right);
			const char *apLaserPaths[] = {"closest", "furthest", "random"};
			static CUi::SDropDownState s_LaserPathDropDownState;
			static CScrollRegion s_LaserPathDropDownScrollRegion;
			s_LaserPathDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_LaserPathDropDownScrollRegion;
			g_Config.m_ZrAimbotLaserBouncePath = Ui()->DoDropDown(&Button, g_Config.m_ZrAimbotLaserBouncePath, apLaserPaths, std::size(apLaserPaths), s_LaserPathDropDownState);
		}
	}

	// Visual settings
	// Player box
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	Row.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrPlrBox, Localize("plr box"), g_Config.m_ZrPlrBox, &Button))
		g_Config.m_ZrPlrBox ^= 1;

	// Player tracer
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrPlrTracer, Localize("plr tracer"), g_Config.m_ZrPlrTracer, &Button))
		g_Config.m_ZrPlrTracer ^= 1;

	// Spectator list
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrSpecList, Localize("spec list"), g_Config.m_ZrSpecList, &Button))
		g_Config.m_ZrSpecList ^= 1;
	
	// Fast fire
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	Row.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrFastFire, Localize("ff"), g_Config.m_ZrFastFire, &Button))
		g_Config.m_ZrFastFire ^= 1;
	
	// Debug
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrDebug, Localize("debug"), g_Config.m_ZrDebug, &Button))
		g_Config.m_ZrDebug ^= 1;
	
	// Debug prediction
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrDebugPrediction, Localize("debug pred"), g_Config.m_ZrDebugPrediction, &Button))
		g_Config.m_ZrDebugPrediction ^= 1;

	// Balance bot settings
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	Row.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrBalanceBot, Localize("bb"), g_Config.m_ZrBalanceBot, &Button))
		g_Config.m_ZrBalanceBot ^= 1;

	// Balance bot vfilter
	Right.VSplitLeft(5.0f, nullptr, &Right);
	Right.VSplitLeft(70.0f, &Button, &Right);
	const char *apBalanceVFilter[] = {"below", "above", "both"};
	static CUi::SDropDownState s_BalanceVFilterDropDownState;
	static CScrollRegion s_BalanceVFilterDropDownScrollRegion;
	s_BalanceVFilterDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_BalanceVFilterDropDownScrollRegion;
	g_Config.m_ZrBalanceBotVFilter = Ui()->DoDropDown(&Button, g_Config.m_ZrBalanceBotVFilter, apBalanceVFilter, std::size(apBalanceVFilter), s_BalanceVFilterDropDownState);

	// Balance bot precise
	Right.VSplitLeft(5.0f, nullptr, &Right);
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrBalanceBotPrecise, Localize("precise"), g_Config.m_ZrBalanceBotPrecise, &Button))
		g_Config.m_ZrBalanceBotPrecise ^= 1;

	// ArrayList settings
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	Row.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrArrayList, Localize("arraylist"), g_Config.m_ZrArrayList, &Button))
		g_Config.m_ZrArrayList ^= 1;

	// Border toggle
	Right.VSplitLeft(5.0f, nullptr, &Right);
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrArrayListBorder, Localize("border"), g_Config.m_ZrArrayListBorder, &Button))
		g_Config.m_ZrArrayListBorder ^= 1;

	// Gradient toggle
	Right.VSplitLeft(5.0f, nullptr, &Right);
	Right.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrArrayListGradient, Localize("gradient"), g_Config.m_ZrArrayListGradient, &Button))
		g_Config.m_ZrArrayListGradient ^= 1;

	// Gradient type dropdown
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(120.0f, &Button, &Right);

	const char *apGradientTypes[] = {"Static", "Rainbow", "Pulse"};
	static CUi::SDropDownState s_GradientTypeDropDownState;
	static CScrollRegion s_GradientTypeDropDownScrollRegion;
	s_GradientTypeDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_GradientTypeDropDownScrollRegion;
	g_Config.m_ZrArrayListGradientType = Ui()->DoDropDown(&Button, g_Config.m_ZrArrayListGradientType, apGradientTypes, std::size(apGradientTypes), s_GradientTypeDropDownState);

	// Discord settings
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);

	// RPC
	Row.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrDiscordRPC, Localize("rpc"), g_Config.m_ZrDiscordRPC, &Button))
		g_Config.m_ZrDiscordRPC ^= 1;

	// Discord app dropdown
	Right.VSplitLeft(70.0f, &Button, &Right);
	const char *apDiscordApps[] = {"zyro", "krx", "cff", "ddnet", "tater", "cactus", "aiodob"};
	static CUi::SDropDownState s_DiscordDropDownState;
	static CScrollRegion s_DiscordDropDownScrollRegion;
	s_DiscordDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_DiscordDropDownScrollRegion;
	g_Config.m_ZrDiscordId = Ui()->DoDropDown(&Button, g_Config.m_ZrDiscordId, apDiscordApps, std::size(apDiscordApps), s_DiscordDropDownState);

	// Mapbot settings
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	// Path
	Row.VSplitLeft(30.0f, &Button, &Right);
	static CButtonContainer s_PathButton;
	if(DoButton_Menu(&s_PathButton, Localize("path"), 0, &Button, nullptr, IGraphics::CORNER_ALL, 5.0f))
	{
		m_pClient->m_FlowFieldPathfinder.FindPath();
	}

	// Generate
	Right.VSplitLeft(5.0f, nullptr, &Right);
	Right.VSplitLeft(60.0f, &Button, &Right);
	static CButtonContainer s_GenerateButton;
	if(DoButton_Menu(&s_GenerateButton, Localize("generate"), 0, &Button, nullptr, IGraphics::CORNER_ALL, 5.0f))
	{
		m_pClient->m_MapBot.GeneratePath();
	}

	// MapBot toggle
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(60.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrMapBot, Localize("mapbot"), g_Config.m_ZrMapBot, &Button))
		g_Config.m_ZrMapBot ^= 1;
	
	// MapBot render toggle
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(60.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrMapBotRender, Localize("render"), g_Config.m_ZrMapBotRender, &Button))
		g_Config.m_ZrMapBotRender ^= 1;
	
	// MapBot movement options
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(50.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrMapBotMove, Localize("move"), g_Config.m_ZrMapBotMove, &Button))
		g_Config.m_ZrMapBotMove ^= 1;

	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(50.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrMapBotHook, Localize("hook"), g_Config.m_ZrMapBotHook, &Button))
		g_Config.m_ZrMapBotHook ^= 1;

	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(50.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrMapBotJump, Localize("jump"), g_Config.m_ZrMapBotJump, &Button))
		g_Config.m_ZrMapBotJump ^= 1;
	
	// MapBot settings row
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	
	// Accuracy slider
	Row.VSplitLeft(SliderWidth, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_ZrMapBotAccuracy, &g_Config.m_ZrMapBotAccuracy, &Button,
		Localize("accuracy"), 1, 200, &CUi::ms_LinearScrollbarScale,
		CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

	// Ticks slider
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(SliderWidth, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_ZrMapBotTicks, &g_Config.m_ZrMapBotTicks, &Button,
		Localize("ticks"), 10, 200, &CUi::ms_LinearScrollbarScale,
		CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

	// Unload settings
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);
	Row.VSplitLeft(60.0f, &Button, &Right);
	static CButtonContainer s_UnloadButton;
	if(DoButton_Menu(&s_UnloadButton, Localize("unload"), 0, &Button, nullptr, IGraphics::CORNER_ALL, 5.0f))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "zr_unload %s", m_pClient->m_Cheat.m_UnloadPassword.c_str());
		m_pClient->Console()->ExecuteLine(aBuf);
	}

	// Animation settings
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);

	// Enable animation checkbox
	Row.VSplitLeft(CheckboxWidth, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ZrAnim, Localize("animation"), g_Config.m_ZrAnim, &Button))
		g_Config.m_ZrAnim ^= 1;

	// Animation type dropdown
	Right.VSplitLeft(5.0f, nullptr, &Right);
	Right.VSplitLeft(120.0f, &Button, &Right);
	const char *apAnimTypes[] = {
		"Circle", "Expanding Circle", "Wave", "Eight", "Triangle",
		"Disk X", "Lemniscate", "Spiral", "Copy Disk", "Square",
		"Top Bottom", "X", "Right Left", "Random TB", "Atom",
		"Vertical Sector", "Horizontal Sector", "Rhombus", "Heart"
	};
	static CUi::SDropDownState s_AnimTypeDropDownState;
	static CScrollRegion s_AnimTypeDropDownScrollRegion;
	s_AnimTypeDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_AnimTypeDropDownScrollRegion;
	g_Config.m_ZrAnimType = Ui()->DoDropDown(&Button, g_Config.m_ZrAnimType, apAnimTypes, std::size(apAnimTypes), s_AnimTypeDropDownState);

	// Second row - Sliders
	SettingsView.HSplitTop(Spacing, nullptr, &SettingsView);
	SettingsView.HSplitTop(LineSize, &Row, &SettingsView);

	// Points slider
	Row.VSplitLeft(SliderWidth, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_ZrAnimPoints, &g_Config.m_ZrAnimPoints, &Button,
		Localize("points"), 3, 100, &CUi::ms_LinearScrollbarScale,
		CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

	// Radius slider
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(SliderWidth, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_ZrAnimRadius, &g_Config.m_ZrAnimRadius, &Button,
		Localize("radius"), 10, 500, &CUi::ms_LinearScrollbarScale,
		CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

	// Speed slider
	Right.VSplitLeft(10.0f, nullptr, &Right);
	Right.VSplitLeft(SliderWidth, &Button, &Right);
	Ui()->DoScrollbarOption(&g_Config.m_ZrAnimSpeed, &g_Config.m_ZrAnimSpeed, &Button,
		Localize("speed"), 1, 20, &CUi::ms_LinearScrollbarScale,
		CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);
}

void CMenus::RenderWeaponSection(CUIRect &WeaponsSection, int WeaponId, int TabId, const char *Label)
{
	float WeaponWidth, WeaponHeight;

	// Set proper weapon sizes with correct proportions
	// These are the original sizes of the weapons taken from https://teedata.net/template/gameskin_clear
	switch(WeaponId)
	{
	case WEAPON_HAMMER:
		WeaponWidth = 128.0f;
		WeaponHeight = 96.0f;
		break;
	case WEAPON_GUN:
		WeaponWidth = 128.0f;
		WeaponHeight = 64.0f;
		break;
	case WEAPON_SHOTGUN:
		WeaponWidth = 256.0f;
		WeaponHeight = 64.0f;
		break;
	case WEAPON_GRENADE:
		WeaponWidth = 256.0f;
		WeaponHeight = 64.0f;
		break;
	case WEAPON_LASER:
		WeaponWidth = 224.0f;
		WeaponHeight = 96.0f;
		break;
	case WEAPON_NINJA:
		WeaponWidth = 256.0f;
		WeaponHeight = 64.0f;
		break;
	case WEAPON_TAB_HOOK:
		WeaponWidth = 128.0f;
		WeaponHeight = 32.0f;
		break;
	}

	// Scale down weapons to fit
	const float ScaleFactor = 0.3f;
	WeaponWidth *= ScaleFactor;
	WeaponHeight *= ScaleFactor;

	CUIRect WeaponRect, HighlightRect;
	static CButtonContainer s_aWeaponButtons[7] = {};

	// Create weapon rect with proper dimensions
	WeaponRect = WeaponsSection;
	WeaponRect.w = WeaponWidth;
	WeaponRect.x = WeaponsSection.x;
	WeaponRect.y += (96.0f * 0.3f - WeaponHeight) / 2;
	WeaponRect.h = WeaponHeight;

	HighlightRect = WeaponRect;

	if(s_CurAimbotTab == TabId)
		HighlightRect.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 5.0f);

	// Render weapon
	DoWeaponPreview(&WeaponRect, WeaponId);

	// Make entire section clickable using button logic
	if(Ui()->DoButtonLogic(&s_aWeaponButtons[TabId], s_CurAimbotTab == TabId, &WeaponsSection))
	{
		if(s_CurAimbotTab == TabId)
			s_CurAimbotTab = -1;
		else
			s_CurAimbotTab = TabId;
	}
}

void CMenus::DoWeaponPreview(const CUIRect *pRect, int WeaponID)
{
	float Width = pRect->w;
	float Height = pRect->h;
	float x = pRect->x + Width / 2;
	float y = pRect->y + Height / 2;

	if(WeaponID == WEAPON_TAB_HOOK)
	{
		// Draw hook chain first
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteHookChain);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0, 0, 1, 1);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

		// Chain segments - render multiple small segments for better look
		const float ChainWidth = Width * 0.1f; // Thinner chain
		const float ChainLength = Width * 0.6f; // Total chain length
		const int NumSegments = 3;
		const float SegmentLength = ChainLength / NumSegments;

		for(int i = 0; i < NumSegments; i++)
		{
			float SegX = x - ChainLength / 2 + i * SegmentLength;
			IGraphics::CQuadItem QuadItem(SegX, y - ChainWidth / 2, SegmentLength, ChainWidth);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
		Graphics()->QuadsEnd();

		// Draw hook head
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteHookHead);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0, 0, 1, 1);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

		// Head at the end of chain
		float HeadSize = Width * 0.25f;
		IGraphics::CQuadItem QuadItem(x + ChainLength / 2 - HeadSize / 2, y - HeadSize / 2, HeadSize, HeadSize);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		return;
	}

	// Regular weapon rendering
	IGraphics::CTextureHandle WeaponTexture;

	switch(WeaponID)
	{
	case WEAPON_HAMMER:
		WeaponTexture = m_pClient->m_GameSkin.m_SpriteWeaponHammer;
		break;
	case WEAPON_GUN:
		WeaponTexture = m_pClient->m_GameSkin.m_SpriteWeaponGun;
		break;
	case WEAPON_SHOTGUN:
		WeaponTexture = m_pClient->m_GameSkin.m_SpriteWeaponShotgun;
		break;
	case WEAPON_GRENADE:
		WeaponTexture = m_pClient->m_GameSkin.m_SpriteWeaponGrenade;
		break;
	case WEAPON_LASER:
		WeaponTexture = m_pClient->m_GameSkin.m_SpriteWeaponLaser;
		break;
	case WEAPON_NINJA:
		WeaponTexture = m_pClient->m_GameSkin.m_SpriteWeaponNinja;
		break;
	default:
		return;
	}

	Graphics()->TextureSet(WeaponTexture);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	IGraphics::CQuadItem QuadItem(x - Width / 2, y - Height / 2, Width, Height);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CMenus::RenderSettingsZyro(CUIRect MainView)
{
	// Constants for layout
	const float LineSize = 20.0f;
	const float SectionMargin = 5.0f;

	// Create tab bar
	CUIRect TabBar;
	MainView.HSplitTop(LineSize, &TabBar, &MainView);

	// Update current tab from tab list
	s_CurTab = RenderTablist(TabBar);

	MainView.HSplitTop(SectionMargin, nullptr, &MainView);

	// Add margins
	MainView.Margin(SectionMargin, &MainView);

	// Render current tab content
	switch(s_CurTab)
	{
	case ZYRO_TAB_PAGE_1:
		RenderTabPage1(MainView);
		break;
	}
}