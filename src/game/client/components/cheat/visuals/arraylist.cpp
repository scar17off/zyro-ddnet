#include "arraylist.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <algorithm>

namespace
{
    struct Feature {
        const char* name;
        ColorRGBA color;
    };

    static const Feature FEATURES[] = {
        {"Aimbot", ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f)},
        {"Balance Bot", ColorRGBA(0.8f, 0.0f, 0.8f, 1.0f)},
        {"Fast Fire", ColorRGBA(1.0f, 0.5f, 0.0f, 1.0f)},
        {"Player Box", ColorRGBA(1.0f, 1.0f, 0.0f, 1.0f)},
        {"Player Tracer", ColorRGBA(0.0f, 1.0f, 0.0f, 1.0f)},
        {"Spectator List", ColorRGBA(0.0f, 1.0f, 1.0f, 1.0f)}
    };
}

void CArrayList::OnReset()
{
    m_X = Graphics()->ScreenWidth();
    m_Padding = 4.0f;
    m_EntryHeight = 20.0f;
    m_TextSize = 16.0f;
}

std::vector<CArrayList::ArrayListItem> CArrayList::GetEnabledFeatures() const
{
    std::vector<ArrayListItem> Features;

    // Aimbot
    if(g_Config.m_ZrAimbot)
        Features.push_back({FEATURES[0].name, FEATURES[0].color});

    // Balance Bot
    if(m_pClient->m_BalanceBot.m_Active)
        Features.push_back({FEATURES[1].name, FEATURES[1].color});

    // Fast Fire
    if(g_Config.m_ZrFastFire)
        Features.push_back({FEATURES[2].name, FEATURES[2].color});

    // Player Box
    if(g_Config.m_ZrPlrBox)
        Features.push_back({FEATURES[3].name, FEATURES[3].color});

    // Player Tracer
    if(g_Config.m_ZrPlrTracer)
        Features.push_back({FEATURES[4].name, FEATURES[4].color});

    // Spectator List
    if(g_Config.m_ZrSpecList)
        Features.push_back({FEATURES[5].name, FEATURES[5].color});

    // Sort alphabetically
    std::sort(Features.begin(), Features.end());
    return Features;
}

void CArrayList::RenderFeature(float Y, const char* Name, const ColorRGBA& Color) const
{
    float TextWidth = Ui()->TextRender()->TextWidth(m_TextSize, Name, -1);
    float BoxWidth = TextWidth + m_Padding * 2;
    float BoxHeight = m_TextSize + m_Padding * 2;
    float X = Graphics()->ScreenWidth() - BoxWidth - 2.0f; // Small gap from screen edge

    // Main background
    CUIRect MainBg = {X - 2.0f, Y, BoxWidth + 4.0f, BoxHeight};
    MainBg.Draw(ColorRGBA(0.1f, 0.1f, 0.1f, 0.85f), IGraphics::CORNER_L, 3.0f);

    // Accent line on the left
    if(g_Config.m_ZrArrayListLine)
    {
        Graphics()->TextureClear();
        Graphics()->QuadsBegin();
        Graphics()->SetColor(Color.r, Color.g, Color.b, 1.0f);
        IGraphics::CQuadItem QuadLine(X - 2.0f, Y, 2.0f, BoxHeight);
        Graphics()->QuadsDrawTL(&QuadLine, 1);
        Graphics()->QuadsEnd();
    }

    // Feature background
    if(g_Config.m_ZrArrayListGradient)
    {
        // Gradient overlay
        Graphics()->TextureClear();
        Graphics()->QuadsBegin();
        Graphics()->SetColor4(
            ColorRGBA(Color.r, Color.g, Color.b, 0.1f),  // Top left
            ColorRGBA(Color.r, Color.g, Color.b, 0.1f),  // Top right
            ColorRGBA(Color.r, Color.g, Color.b, 0.0f),  // Bottom right
            ColorRGBA(Color.r, Color.g, Color.b, 0.0f)   // Bottom left
        );
        IGraphics::CFreeformItem Freeform(
            X, Y,                          // Top left
            X + BoxWidth, Y,              // Top right
            X, Y + BoxHeight,             // Bottom left
            X + BoxWidth, Y + BoxHeight   // Bottom right
        );
        Graphics()->QuadsDrawFreeform(&Freeform, 1);
        Graphics()->QuadsEnd();
    }

    // Text with shadow
    Ui()->TextRender()->TextColor(0.0f, 0.0f, 0.0f, 0.5f);
    Ui()->TextRender()->Text(X + m_Padding + 1, Y + m_Padding + 1, m_TextSize, Name, -1.0f);
    Ui()->TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
    Ui()->TextRender()->Text(X + m_Padding, Y + m_Padding, m_TextSize, Name, -1.0f);
}

void CArrayList::OnRender()
{
    // Only render if arraylist is enabled
    if(!g_Config.m_ZrArrayList || !m_pClient->m_Cheat.m_Active)
        return;

    auto Features = GetEnabledFeatures();
    if(Features.empty())
        return;

    // Save current screen mapping
    float Points[4];
    Graphics()->GetScreen(Points, Points+1, Points+2, Points+3);

    // Map screen coordinates
    float ScreenWidth = Graphics()->ScreenWidth();
    float ScreenHeight = Graphics()->ScreenHeight();
    Graphics()->MapScreen(0.0f, 0.0f, ScreenWidth, ScreenHeight);

    // Calculate total height and vertical position
    float BoxHeight = m_TextSize + m_Padding * 2;
    float TotalHeight = Features.size() * BoxHeight;
    float Y = (ScreenHeight - TotalHeight) / 2; // Vertically centered

    // Render each feature
    for(const auto& Feature : Features)
    {
        RenderFeature(Y, Feature.name, Feature.color);
        Y += BoxHeight;
    }

    // Restore original screen mapping
    Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}
