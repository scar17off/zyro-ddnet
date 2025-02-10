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
    CUIRect Button, Right, Row, Label;
    const float LineSize = 20.0f;
    const float Spacing = 2.0f;
    const float CheckboxWidth = 80.0f;
    const float SliderWidth = 150.0f;

    // Limit the width and height of the menu
    MainView.VSplitLeft(300.0f, &MainView, nullptr);
    MainView.HSplitTop(140.0f, &MainView, nullptr);

    // Aimbot settings (Aim, Silent, FOV on same row)
    MainView.HSplitTop(LineSize, &Row, &MainView);
    Row.VSplitLeft(CheckboxWidth, &Button, &Right);
    if(DoButton_CheckBox(&g_Config.m_ZrAimbot, Localize("aim"), g_Config.m_ZrAimbot, &Button))
        g_Config.m_ZrAimbot ^= 1;
    
    Right.VSplitLeft(CheckboxWidth, &Button, &Label);
    if(DoButton_CheckBox(&g_Config.m_ZrAimbotMode, Localize("silent"), g_Config.m_ZrAimbotMode, &Button))
        g_Config.m_ZrAimbotMode ^= 1;

    Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotFoV, &g_Config.m_ZrAimbotFoV, &Label, Localize("fov"), 1, 360, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "°");

    // Hook and Laser accuracy on same row
    MainView.HSplitTop(Spacing, nullptr, &MainView);
    MainView.HSplitTop(LineSize, &Row, &MainView);
    Row.VSplitLeft(SliderWidth, &Button, &Right);
    Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotHookAccuracy, &g_Config.m_ZrAimbotHookAccuracy, &Button, Localize("hook acc"), 1, 200, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");
    Right.VSplitLeft(5.0f, nullptr, &Right);
    Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotLaserAccuracy, &g_Config.m_ZrAimbotLaserAccuracy, &Right, Localize("laser acc"), 1, 200, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "%");

    // Laser bounce settings
    MainView.HSplitTop(Spacing, nullptr, &MainView);
    MainView.HSplitTop(LineSize, &Row, &MainView);
    Row.VSplitLeft(CheckboxWidth, &Button, &Right);
    if(DoButton_CheckBox(&g_Config.m_ZrAimbotLaserUseBounce, Localize("bounce"), g_Config.m_ZrAimbotLaserUseBounce, &Button))
        g_Config.m_ZrAimbotLaserUseBounce ^= 1;

    Right.VSplitLeft(CheckboxWidth, &Button, &Label);
    if(DoButton_CheckBox(&g_Config.m_ZrAimbotLaserBounceOnly, Localize("only"), g_Config.m_ZrAimbotLaserBounceOnly, &Button))
        g_Config.m_ZrAimbotLaserBounceOnly ^= 1;

    // Bounce count and path selection on same row
    MainView.HSplitTop(Spacing, nullptr, &MainView);
    MainView.HSplitTop(LineSize, &Row, &MainView);
    Row.VSplitLeft(SliderWidth, &Button, &Right);
    Ui()->DoScrollbarOption(&g_Config.m_ZrAimbotLaserBounceCount, &g_Config.m_ZrAimbotLaserBounceCount, &Button, Localize("count"), 1, 10, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);
    Right.VSplitLeft(5.0f, nullptr, &Right);
    const char *apBouncePaths[] = {"closest", "furthest", "random"};
    static CUi::SDropDownState s_BouncePathDropDownState;
    static CScrollRegion s_BouncePathDropDownScrollRegion;
    s_BouncePathDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_BouncePathDropDownScrollRegion;
    g_Config.m_ZrAimbotLaserBouncePath = Ui()->DoDropDown(&Right, g_Config.m_ZrAimbotLaserBouncePath, apBouncePaths, std::size(apBouncePaths), s_BouncePathDropDownState);

    // Discord settings (RPC checkbox and dropdown on same row)
    MainView.HSplitTop(Spacing * 2, nullptr, &MainView);
    MainView.HSplitTop(LineSize, &Row, &MainView);
    Row.VSplitLeft(CheckboxWidth, &Button, &Right);
    if(DoButton_CheckBox(&g_Config.m_ZrDiscordRPC, Localize("rpc"), g_Config.m_ZrDiscordRPC, &Button))
        g_Config.m_ZrDiscordRPC ^= 1;

    const char *apDiscordApps[] = {"ddnet", "tater", "cff", "krx", "zyro"};
    static CUi::SDropDownState s_DiscordDropDownState;
    static CScrollRegion s_DiscordDropDownScrollRegion;
    s_DiscordDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_DiscordDropDownScrollRegion;
    g_Config.m_ZrDiscord = Ui()->DoDropDown(&Right, g_Config.m_ZrDiscord, apDiscordApps, std::size(apDiscordApps), s_DiscordDropDownState);
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