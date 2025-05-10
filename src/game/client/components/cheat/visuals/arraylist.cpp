#include "arraylist.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <algorithm>
#include <cmath>

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
    m_AnimTime = 0.0f;
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

ColorRGBA CArrayList::GetGradientColor(const ColorRGBA& BaseColor, float Y) const
{
    // Handle different gradient types
    // THIS SHIT IS COMPLETELY STOLEN FROM EVERYWHERE (IT REQUIRES A BIG BRAIN TO UNDERSTAND)
    switch(g_Config.m_ZrArrayListGradientType)
    {
    case GRADIENT_RAINBOW:
    {
        // Rainbow gradient based on time and position
        float Hue = fmodf(m_AnimTime * 0.5f + Y * 0.01f, 1.0f);
        float Sat = 1.0f;
        float Val = 1.0f;
        
        // HSV to RGB conversion
        float C = Val * Sat;
        float X = C * (1.0f - fabsf(fmodf(Hue * 6.0f, 2.0f) - 1.0f));
        float M = Val - C;
        
        float R, G, B;
        if(Hue < 1.0f/6.0f)      { R = C; G = X; B = 0; }
        else if(Hue < 2.0f/6.0f) { R = X; G = C; B = 0; }
        else if(Hue < 3.0f/6.0f) { R = 0; G = C; B = X; }
        else if(Hue < 4.0f/6.0f) { R = 0; G = X; B = C; }
        else if(Hue < 5.0f/6.0f) { R = X; G = 0; B = C; }
        else                     { R = C; G = 0; B = X; }
        
        return ColorRGBA(R + M, G + M, B + M, 1.0f);
    }
    
    case GRADIENT_PULSE:
    {
        // Create a moving darkness effect from top to bottom
        float AnimSpeed = 1.0f; 
        float WaveHeight = 100.0f;
        float DarkenFactor = 0.3f; // How dark it gets at maximum (30% of original brightness)
        
        // Calculate the wave position based on time
        float WavePos = fmodf(m_AnimTime * AnimSpeed * 200.0f, WaveHeight * 2.0f);
        
        // Calculate distance from the wave center to current Y position
        float DistFromWave = fabsf(Y - WavePos);
        
        // Create a smooth transition using sine
        float Intensity = 1.0f - (DarkenFactor * (1.0f + cosf(DistFromWave * (pi / WaveHeight))) * 0.5f);
        Intensity = clamp(Intensity, 1.0f - DarkenFactor, 1.0f);
        
        // Apply the darkness effect while maintaining color ratios
        return ColorRGBA(
            BaseColor.r * Intensity,
            BaseColor.g * Intensity,
            BaseColor.b * Intensity,
            1.0f
        );
    }
    
    case GRADIENT_STATIC:
    default:
        // Default static gradient with base color
        return BaseColor;
    }
}

void CArrayList::RenderFeature(float Y, const char* Name, const ColorRGBA& Color) const
{
    float TextWidth = Ui()->TextRender()->TextWidth(m_TextSize, Name, -1);
    float BoxWidth = TextWidth + m_Padding * 2;
    float BoxHeight = m_TextSize + m_Padding * 2;
    float X = Graphics()->ScreenWidth() - BoxWidth - 2.0f;

    // Get gradient color if enabled
    ColorRGBA GradientColor = g_Config.m_ZrArrayListGradient ? GetGradientColor(Color, Y) : Color;

    // Main background with rounded corners (more transparent)
    CUIRect MainBg = {X - 2.0f, Y, BoxWidth + 4.0f, BoxHeight};
    MainBg.Draw(ColorRGBA(0.1f, 0.1f, 0.1f, 0.6f), IGraphics::CORNER_ALL, 3.0f);

    // Colored border using gradient color
    if(g_Config.m_ZrArrayListBorder)
    {
        // Top border
        Graphics()->TextureClear();
        Graphics()->QuadsBegin();
        Graphics()->SetColor(GradientColor.r, GradientColor.g, GradientColor.b, 1.0f);
        IGraphics::CQuadItem QuadTop(X - 2.0f, Y, BoxWidth + 4.0f, 2.0f);
        Graphics()->QuadsDrawTL(&QuadTop, 1);
        Graphics()->QuadsEnd();

        // Bottom border
        Graphics()->TextureClear();
        Graphics()->QuadsBegin();
        Graphics()->SetColor(GradientColor.r, GradientColor.g, GradientColor.b, 1.0f);
        IGraphics::CQuadItem QuadBottom(X - 2.0f, Y + BoxHeight - 2.0f, BoxWidth + 4.0f, 2.0f);
        Graphics()->QuadsDrawTL(&QuadBottom, 1);
        Graphics()->QuadsEnd();

        // Left border
        Graphics()->TextureClear();
        Graphics()->QuadsBegin();
        Graphics()->SetColor(GradientColor.r, GradientColor.g, GradientColor.b, 1.0f);
        IGraphics::CQuadItem QuadLeft(X - 2.0f, Y, 2.0f, BoxHeight);
        Graphics()->QuadsDrawTL(&QuadLeft, 1);
        Graphics()->QuadsEnd();

        // Right border
        Graphics()->TextureClear();
        Graphics()->QuadsBegin();
        Graphics()->SetColor(GradientColor.r, GradientColor.g, GradientColor.b, 1.0f);
        IGraphics::CQuadItem QuadRight(X + BoxWidth, Y, 2.0f, BoxHeight);
        Graphics()->QuadsDrawTL(&QuadRight, 1);
        Graphics()->QuadsEnd();
    }

    // Text with gradient color
    Ui()->TextRender()->TextColor(0.0f, 0.0f, 0.0f, 0.5f);
    Ui()->TextRender()->Text(X + m_Padding + 1, Y + m_Padding + 1, m_TextSize, Name, -1.0f);
    Ui()->TextRender()->TextColor(GradientColor.r, GradientColor.g, GradientColor.b, 1.0f);
    Ui()->TextRender()->Text(X + m_Padding, Y + m_Padding, m_TextSize, Name, -1.0f);
    
    // This is necessary to reset the text color to default white so it doesn't affect other text
    Ui()->TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CArrayList::OnRender()
{
    // Only render if arraylist is enabled
    if(!g_Config.m_ZrArrayList || !m_pClient->m_Cheat.m_Active)
        return;

    auto Features = GetEnabledFeatures();
    if(Features.empty())
        return;

    // Update animation time
    m_AnimTime += Client()->RenderFrameTime();

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

    for(const ArrayListItem& Feature : Features)
    {
        RenderFeature(Y, Feature.name, Feature.color);
        Y += BoxHeight + 2.0f;
    }

    // Restore original screen mapping
    Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}
