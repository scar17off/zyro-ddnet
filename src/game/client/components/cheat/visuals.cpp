#include "visuals.h"

#include <game/client/gameclient.h>

void CVisuals::RenderBox(float x, float y, float w, float h, bool fill, bool stroke, ColorRGBA fillColor, ColorRGBA strokeColor)
{
    Graphics()->TextureClear();

    // Draw filled box
    if(fill)
    {
        Graphics()->QuadsBegin();
        Graphics()->SetColor(fillColor);
        IGraphics::CQuadItem QuadItem(x, y, w, h);
        Graphics()->QuadsDrawTL(&QuadItem, 1);
        Graphics()->QuadsEnd();
    }

    // Draw stroke/outline
    if(stroke)
    {
        Graphics()->LinesBegin();
        Graphics()->SetColor(strokeColor);
        
        // Draw 4 lines to make the box outline
        IGraphics::CLineItem Lines[] = {
            IGraphics::CLineItem(x, y, x + w, y),         // Top
            IGraphics::CLineItem(x + w, y, x + w, y + h), // Right
            IGraphics::CLineItem(x + w, y + h, x, y + h), // Bottom
            IGraphics::CLineItem(x, y + h, x, y)          // Left
        };
        
        Graphics()->LinesDraw(Lines, 4);
        Graphics()->LinesEnd();
    }
}

void CVisuals::DrawLine(float x1, float y1, float x2, float y2, float w, ColorRGBA color)
{
    Graphics()->TextureClear();
    Graphics()->QuadsBegin();
    Graphics()->SetColor(color);
    
    // Create a line with width by making a quad
    vec2 LineDir = normalize(vec2(x2 - x1, y2 - y1));
    vec2 Perpendicular = vec2(LineDir.y, -LineDir.x) * w;

    IGraphics::CFreeformItem Freeform(
        x1 - Perpendicular.x, y1 - Perpendicular.y, // Top left
        x1 + Perpendicular.x, y1 + Perpendicular.y, // Top right
        x2 - Perpendicular.x, y2 - Perpendicular.y, // Bottom left
        x2 + Perpendicular.x, y2 + Perpendicular.y  // Bottom right
    );
    
    Graphics()->QuadsDrawFreeform(&Freeform, 1);
    Graphics()->QuadsEnd();
}