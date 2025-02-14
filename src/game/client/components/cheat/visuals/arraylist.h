#ifndef GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_ARRAYLIST_H
#define GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_ARRAYLIST_H

#include <game/client/component.h>
#include <base/color.h>
#include <vector>

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
            return str_comp(name, other.name) < 0;
        }
    };

private:
    std::vector<ArrayListItem> GetEnabledFeatures() const;
    void RenderFeature(float Y, const char* Name, const ColorRGBA& Color) const;

    float m_X = 0.0f;
    float m_Padding = 6.0f;
    float m_EntryHeight = 22.0f;
    float m_TextSize = 15.0f;
};

#endif
