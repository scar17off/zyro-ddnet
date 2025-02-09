#include <game/localization.h>

#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>

#include "menus.h"

static int s_CurTab = 0;
enum
{
    ZYRO_TAB_PAGE_1 = 0,
    NUMBER_OF_ZYRO_TABS
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
        if(i == 0) Corners |= IGraphics::CORNER_L;
        if(i == NUMBER_OF_ZYRO_TABS-1) Corners |= IGraphics::CORNER_R;

        // Get tab text
        const char *pTabText = "";
        switch(i) {
            case ZYRO_TAB_PAGE_1: pTabText = "Page 1"; break;
        }

        if(DoButton_MenuTab(&s_aPageTabs[i], Localize(pTabText), s_CurZyroTab == i, &Tab, Corners))
            s_CurZyroTab = i;
    }

    return s_CurZyroTab;
}

void CMenus::RenderTabPage1(CUIRect MainView)
{
    CUIRect Button, Label, Row;
    const float ButtonHeight = 20.0f;
    const float FontSize = 14.0f;
    float RowWidth = 60.0f;

    MainView.HSplitTop(ButtonHeight, &Row, &MainView);
    
    // Row: Aimbot settings
    // First item (Aim checkbox)
    Row.VSplitLeft(RowWidth, &Button, &Row);
    if(DoButton_CheckBox(&g_Config.m_ZrAimbot, Localize("aim"), g_Config.m_ZrAimbot, &Button))
        g_Config.m_ZrAimbot ^= 1;

    // Second item (Silent checkbox) 
    Row.VSplitLeft(RowWidth, &Button, &Row);
    if(DoButton_CheckBox(&g_Config.m_ZrAimbotMode, Localize("silent"), g_Config.m_ZrAimbotMode, &Button))
        g_Config.m_ZrAimbotMode ^= 1;

    // Third item (FOV slider)
    Row.VSplitLeft(RowWidth + 100.0f, &Button, &Row);
    Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotFoV, &g_Config.m_ZrAimbotFoV, &Button, Localize("fov"), 1, 360, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "°");

    // Single item (Hook accuracy)
    MainView.HSplitTop(ButtonHeight, &Row, &MainView);

    Row.VSplitLeft(RowWidth + 100.0f, &Button, &Row);
    Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotHookAccuracy, &g_Config.m_ZrAimbotHookAccuracy, &Button, Localize("hook accuracy"), 1, 200, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");

    // Row: Discord presence
    MainView.HSplitTop(ButtonHeight, &Row, &MainView);
    
    // First item (RPC checkbox)
    Row.VSplitLeft(RowWidth, &Button, &Row);
    if(DoButton_CheckBox(&g_Config.m_ZrDiscordRPC, Localize("rpc"), g_Config.m_ZrDiscordRPC, &Button))
        g_Config.m_ZrDiscordRPC ^= 1;

    // Second item (Discord app dropdown)
    Row.VSplitLeft(RowWidth + 100.0f, &Button, &Row);
    const char *apDiscordApps[] = {"DDNet", "Tater", "CFF", "KRX", "Zyro"};
    static CUi::SDropDownState s_DiscordDropDownState;
    static CScrollRegion s_DiscordDropDownScrollRegion;
    s_DiscordDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_DiscordDropDownScrollRegion;
    g_Config.m_ZrDiscord = Ui()->DoDropDown(&Button, g_Config.m_ZrDiscord, apDiscordApps, std::size(apDiscordApps), s_DiscordDropDownState);
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