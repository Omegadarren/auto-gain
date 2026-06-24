#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace PlateUi
{
struct Theme
{
    static juce::Colour background()        { return juce::Colour (0xff141210); }
    static juce::Colour backgroundDeep()    { return juce::Colour (0xff0a0908); }
    static juce::Colour surface()           { return juce::Colour (0xff221f1c); }
    static juce::Colour surfaceRaised()   { return juce::Colour (0xff2c2824); }
    static juce::Colour surfaceHighlight() { return juce::Colour (0xff38332e); }
    static juce::Colour border()            { return juce::Colour (0xff3a3530); }
    static juce::Colour borderLight()       { return juce::Colour (0xff5a524a); }
    static juce::Colour text()              { return juce::Colour (0xfff8f4ef); }
    static juce::Colour textDim()           { return juce::Colour (0xff9a928a); }
    static juce::Colour accent()            { return juce::Colour (0xffe09840); }
    static juce::Colour accentBright()      { return juce::Colour (0xfff0b868); }
    static juce::Colour accentDeep()        { return juce::Colour (0xffa86828); }
    static juce::Colour accentSecondary()   { return juce::Colour (0xffd88090); }
    static juce::Colour accentTertiary()    { return juce::Colour (0xff68b890); }
    static juce::Colour accentShimmer()     { return juce::Colour (0xffc090e0); }
    static juce::Colour meterSafe()         { return juce::Colour (0xff58a880); }
    static juce::Colour meterWarn()         { return juce::Colour (0xfff0b050); }
    static juce::Colour meterHot()          { return juce::Colour (0xffe85858); }
    static juce::Colour powerOn()           { return juce::Colour (0xff58d888); }
    static juce::Colour lcdBg()             { return juce::Colour (0xff0c0a08); }
    static juce::Colour lcdText()           { return juce::Colour (0xfff0b868); }

    static juce::Colour panel()             { return surface(); }
    static juce::Colour divider()           { return border(); }
    static juce::Colour gold()              { return accentBright(); }
    static juce::Colour orange()            { return accent(); }
    static juce::Colour green()             { return accentTertiary(); }
    static juce::Colour red()               { return meterHot(); }
};

void drawBackground (juce::Graphics& g, juce::Rectangle<int> bounds, bool active);
void drawHeaderBar (juce::Graphics& g, juce::Rectangle<int> bounds, int height, bool active);
void drawBrandMark (juce::Graphics& g, juce::Rectangle<int> area, bool active);
void drawBrandTitle (juce::Graphics& g, juce::Rectangle<int> titleArea, bool active);
void drawSectionPanel (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title, juce::Colour accent);
void drawSectionLabel (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text, juce::Colour colour = Theme::textDim());
void drawMeterStrip (juce::Graphics& g, juce::Rectangle<float> area);
void drawLcdReadout (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text);
void drawVUMeter (juce::Graphics& g, float x, float y, float w, float h, float db, float peakHoldDb = -60.0f);
void drawPowerIndicator (juce::Graphics& g, float cx, float cy, float radius, bool lit);
void drawFloatingControl (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text, bool pressed);

class PlateLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PlateLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
};

class PlateKnob : public juce::Component
{
public:
    PlateKnob (const juce::String& name, juce::LookAndFeel& laf, juce::Colour accentColour);

    juce::Slider& getSlider() { return slider; }
    void setLabelText (const juce::String& text);
    void resized() override;

private:
    juce::Slider slider;
    juce::Label label;
    juce::Colour accent;
};

} // namespace PlateUi
