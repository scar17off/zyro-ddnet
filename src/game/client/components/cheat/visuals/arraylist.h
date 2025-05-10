#ifndef GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_ARRAYLIST_H
#define GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_ARRAYLIST_H

#include <game/client/component.h>
#include <game/client/ui.h>
#include <base/color.h>
#include <vector>
#include <utility>

// Gradient types for the ArrayList
enum EArrayListGradientType
{
    GRADIENT_STATIC = 0,  // Default static gradient using feature color
    GRADIENT_RAINBOW,     // Animated rainbow gradient
    GRADIENT_PULSE,       // Pulsing gradient that fades in and out
    NUM_GRADIENT_TYPES
};

class CArrayList : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnReset() override;
    virtual void OnRender() override;

    struct ArrayListItem {
        const char* name;
        ColorRGBA color;
        
        bool operator<(const ArrayListItem& other) const {
            return strcmp(name, other.name) < 0;
        }
    };

private:
    float m_X = 0.0f;
    float m_Padding = 6.0f;
    float m_EntryHeight = 22.0f;
    float m_TextSize = 15.0f;
    
    // Time tracking for animations
    float m_AnimTime = 0.0f;

    std::vector<ArrayListItem> GetEnabledFeatures() const;
    void RenderFeature(float Y, const char* Name, const ColorRGBA& Color) const;
    ColorRGBA GetGradientColor(const ColorRGBA& BaseColor, float Y) const;
};

#endif
