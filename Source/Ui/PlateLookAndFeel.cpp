#include "PlateLookAndFeel.h"

namespace PlateUi
{
namespace
{
juce::Path buildArc (float cx, float cy, float radius, float startAngle, float endAngle)
{
    juce::Path arc;
    arc.addCentredArc (cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    return arc;
}

void drawSoftShadow (juce::Graphics& g, juce::Rectangle<float> r, float radius, float yOffset, float alpha, float expand = 0.0f)
{
    g.setColour (juce::Colours::black.withAlpha (alpha));
    g.fillRoundedRectangle (r.expanded (expand).translated (0.0f, yOffset), radius + expand);
}

void drawRaisedSurface (juce::Graphics& g, juce::Rectangle<float> r, float radius,
                        juce::Colour top, juce::Colour bottom, bool pressed = false)
{
    const auto face = pressed ? r.translated (0.0f, 1.0f) : r;

    if (! pressed)
        drawSoftShadow (g, face, radius, 3.0f, 0.38f, 1.0f);

    juce::ColourGradient body (pressed ? bottom : top, face.getX(), face.getY(),
                               pressed ? top : bottom, face.getX(), face.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (face, radius);

    g.setColour (juce::Colours::white.withAlpha (pressed ? 0.04f : 0.14f));
    g.drawHorizontalLine (static_cast<int> (face.getY() + 1.0f), face.getX() + radius * 0.5f, face.getRight() - radius * 0.5f);

    g.setColour (juce::Colours::black.withAlpha (pressed ? 0.22f : 0.35f));
    g.drawRoundedRectangle (face.reduced (0.5f), radius, 1.0f);
}

void drawInsetSurface (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill)
{
    drawSoftShadow (g, r, radius, 1.0f, 0.18f, 0.5f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, radius);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine (static_cast<int> (r.getY() + 1.0f), r.getX() + 2.0f, r.getRight() - 2.0f);
}

void strokeArc (juce::Graphics& g, const juce::Path& path, float thickness, juce::Colour colour, float alpha = 1.0f)
{
    g.setColour (colour.withAlpha (alpha));
    g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawKnob3D (juce::Graphics& g, float cx, float cy, float radius, float angle,
                 float rotaryStart, float rotaryEnd, float sliderPos, juce::Colour accent, bool isLarge)
{
    const float wellR = radius + (isLarge ? 7.0f : 5.0f);
    const float arcR = radius + (isLarge ? 2.0f : 1.0f);
    const float arcThick = isLarge ? 4.0f : 3.0f;
    const float capR = radius * (isLarge ? 0.54f : 0.58f);

    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillEllipse (cx - wellR + 1.0f, cy - wellR + 4.0f, wellR * 2.0f, wellR * 2.0f);

    juce::ColourGradient wellGrad (Theme::backgroundDeep(), cx, cy - wellR,
                                   Theme::surfaceRaised(), cx, cy + wellR, false);
    g.setGradientFill (wellGrad);
    g.fillEllipse (cx - wellR, cy - wellR, wellR * 2.0f, wellR * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (cx - wellR, cy - wellR, wellR * 2.0f, wellR * 2.0f, 1.2f);

    g.setColour (Theme::borderLight().withAlpha (0.10f));
    g.drawEllipse (cx - wellR + 1.5f, cy - wellR + 1.5f, (wellR - 1.5f) * 2.0f, (wellR - 1.5f) * 2.0f, 0.8f);

    juce::Path track = buildArc (cx, cy, arcR, rotaryStart, rotaryEnd);
    strokeArc (g, track, arcThick + 1.5f, juce::Colours::black, 0.35f);
    strokeArc (g, track, arcThick, Theme::border(), 0.85f);

    if (sliderPos > 0.002f)
    {
        const float valueAngle = rotaryStart + sliderPos * (rotaryEnd - rotaryStart);
        juce::Path glowArc = buildArc (cx, cy, arcR, rotaryStart, valueAngle);
        strokeArc (g, glowArc, arcThick + 4.0f, accent, 0.12f);

        juce::Path valueArc = buildArc (cx, cy, arcR, rotaryStart, valueAngle);
        juce::ColourGradient arcGrad (Theme::accentBright(), cx - arcR, cy - arcR,
                                      Theme::accentDeep(), cx + arcR, cy + arcR, true);
        g.setGradientFill (arcGrad);
        g.strokePath (valueArc, juce::PathStrokeType (arcThick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (cx - capR + 1.0f, cy - capR + 2.5f, capR * 2.0f, capR * 2.0f);

    juce::ColourGradient capGrad (Theme::surfaceHighlight(), cx - capR * 0.55f, cy - capR * 0.65f,
                                  Theme::surface(), cx + capR * 0.45f, cy + capR * 0.55f, true);
    g.setGradientFill (capGrad);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    juce::ColourGradient shade (juce::Colours::transparentBlack, cx, cy + capR * 0.15f,
                                juce::Colours::black.withAlpha (0.35f), cx, cy + capR, false);
    g.setGradientFill (shade);
    g.fillEllipse (cx - capR, cy - capR * 0.1f, capR * 2.0f, capR * 1.1f);

    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.fillEllipse (cx - capR * 0.45f, cy - capR * 0.55f, capR * 0.75f, capR * 0.55f);

    g.setColour (Theme::borderLight().withAlpha (0.22f));
    g.drawEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 0.9f);

    const float pointerLen = capR * 0.78f;
    const float px = cx + pointerLen * std::sin (angle);
    const float py = cy - pointerLen * std::cos (angle);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (cx + 1.0f, cy + 1.0f, px + 1.0f, py + 1.0f, 2.4f);

    juce::ColourGradient pointerGrad (Theme::accentBright(), cx, cy, Theme::accentDeep(), px, py, false);
    g.setGradientFill (pointerGrad);
    g.drawLine (cx, cy, px, py, 2.2f);

    g.setColour (Theme::accentBright().withAlpha (0.85f));
    g.fillEllipse (px - 2.6f, py - 2.6f, 5.2f, 5.2f);
}
} // namespace

void drawBackground (juce::Graphics& g, juce::Rectangle<int> bounds, bool active)
{
    juce::ColourGradient base (Theme::background().brighter (0.04f), 0.0f, static_cast<float> (bounds.getY()),
                               Theme::backgroundDeep(), 0.0f, static_cast<float> (bounds.getBottom()), false);
    g.setGradientFill (base);
    g.fillRect (bounds);

    for (const auto corner : { bounds.getTopLeft(), bounds.getTopRight(), bounds.getBottomLeft(), bounds.getBottomRight() })
    {
        juce::ColourGradient vignette (juce::Colours::black.withAlpha (0.42f), static_cast<float> (corner.getX()), static_cast<float> (corner.getY()),
                                        juce::Colours::transparentBlack, static_cast<float> (corner.getX()), static_cast<float> (corner.getY()), true);
        g.setGradientFill (vignette);
        g.fillRect (bounds);
    }

    if (active)
    {
        juce::ColourGradient spotlight (Theme::accent().withAlpha (0.10f),
                                        static_cast<float> (bounds.getCentreX()), static_cast<float> (bounds.getY() + 20.0f),
                                        juce::Colours::transparentBlack,
                                        static_cast<float> (bounds.getCentreX()), static_cast<float> (bounds.getY() + 280.0f), true);
        g.setGradientFill (spotlight);
        g.fillRect (bounds);

        g.setColour (Theme::accentSecondary().withAlpha (0.035f));
        g.fillEllipse (static_cast<float> (bounds.getRight()) - 180.0f, 120.0f, 220.0f, 220.0f);

        g.setColour (Theme::accentTertiary().withAlpha (0.028f));
        g.fillEllipse (40.0f, static_cast<float> (bounds.getBottom()) - 200.0f, 180.0f, 180.0f);
    }

    for (int y = bounds.getY(); y < bounds.getBottom(); y += 3)
    {
        const float alpha = 0.012f + 0.008f * std::sin (static_cast<float> (y) * 0.11f);
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.drawHorizontalLine (y, static_cast<float> (bounds.getX()), static_cast<float> (bounds.getRight()));
    }
}

void drawHeaderBar (juce::Graphics& g, juce::Rectangle<int> bounds, int height, bool active)
{
    const auto header = bounds.withHeight (height).toFloat().reduced (10.0f, 6.0f).withTrimmedTop (0.0f);

    drawSoftShadow (g, header, 10.0f, 5.0f, 0.45f, 2.0f);
    drawRaisedSurface (g, header, 10.0f, Theme::surfaceRaised().brighter (0.08f), Theme::surface(), false);

    juce::ColourGradient accentLine (Theme::accentBright().withAlpha (active ? 0.95f : 0.40f), header.getX() + 20.0f, header.getBottom() - 2.0f,
                                     Theme::accentSecondary().withAlpha (active ? 0.65f : 0.25f), header.getRight() - 20.0f, header.getBottom() - 2.0f, false);
    g.setGradientFill (accentLine);
    g.fillRect (header.getX() + 18.0f, header.getBottom() - 3.0f, header.getWidth() - 36.0f, 2.0f);

    if (active)
    {
        g.setColour (Theme::accent().withAlpha (0.12f));
        g.fillRect (header.getX() + 18.0f, header.getBottom() - 4.0f, header.getWidth() - 36.0f, 6.0f);
    }
}

void drawBrandMark (juce::Graphics& g, juce::Rectangle<int> area, bool active)
{
    const float badgeSize = 13.0f;
    const float badgeX = static_cast<float> (area.getX());
    const float badgeY = static_cast<float> (area.getCentreY()) - badgeSize * 0.5f;
    const auto badge = juce::Rectangle<float> (badgeX, badgeY, badgeSize, badgeSize);

    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillRoundedRectangle (badge.translated (0.0f, 1.0f), 3.0f);

    juce::ColourGradient badgeGrad (Theme::accentBright().withAlpha (active ? 1.0f : 0.65f), badge.getX(), badge.getY(),
                                    Theme::accentDeep().withAlpha (active ? 1.0f : 0.55f), badge.getRight(), badge.getBottom(), false);
    g.setGradientFill (badgeGrad);
    g.fillRoundedRectangle (badge, 3.0f);

    g.setColour (juce::Colours::white.withAlpha (active ? 0.28f : 0.14f));
    g.fillRoundedRectangle (badge.reduced (1.0f, 1.0f).withHeight (badge.getHeight() * 0.45f), 2.0f);

    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (Theme::backgroundDeep().withAlpha (0.92f));
    g.drawText (juce::String::charToString (0x03A9), badge.toNearestInt(), juce::Justification::centred, false);

    const auto textArea = area.withTrimmedLeft (static_cast<int> (badgeSize + 5.0f));

    g.setFont (juce::Font (8.5f, juce::Font::bold));
    g.setColour (Theme::accentDeep().withAlpha (0.35f));
    g.drawText ("OMEGADARREN", textArea.translated (0, 1), juce::Justification::centredLeft, false);

    juce::ColourGradient nameGrad (active ? Theme::accentBright() : Theme::textDim().withAlpha (0.90f),
                                   static_cast<float> (textArea.getX()), static_cast<float> (textArea.getY()),
                                   active ? Theme::accent() : Theme::textDim().withAlpha (0.65f),
                                   static_cast<float> (textArea.getRight()), static_cast<float> (textArea.getBottom()), false);
    g.setGradientFill (nameGrad);
    g.drawText ("OMEGADARREN", textArea, juce::Justification::centredLeft, false);
}

void drawBrandTitle (juce::Graphics& g, juce::Rectangle<int> titleArea, bool active)
{
    const auto logoRow = titleArea.withHeight (14);
    const auto mainTitle = titleArea.withTrimmedTop (16).withHeight (28);

    drawBrandMark (g, logoRow, active);

    g.setFont (juce::Font (21.0f, juce::Font::bold));

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.drawText ("Plate Reverb", mainTitle.translated (0, 2), juce::Justification::centredLeft, false);

    juce::ColourGradient titleGrad (Theme::text(), static_cast<float> (mainTitle.getX()), static_cast<float> (mainTitle.getY()),
                                    active ? Theme::accentBright() : Theme::textDim(),
                                    static_cast<float> (mainTitle.getX() + 80.0f), static_cast<float> (mainTitle.getBottom()), false);
    g.setGradientFill (titleGrad);
    g.drawText ("Plate Reverb", mainTitle, juce::Justification::centredLeft, false);
}

void drawSectionPanel (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title, juce::Colour accent)
{
    const auto r = area.toFloat().reduced (1.0f);
    const float radius = 12.0f;

    drawSoftShadow (g, r, radius, 8.0f, 0.32f, 2.0f);
    drawSoftShadow (g, r, radius, 4.0f, 0.22f, 1.0f);

    juce::ColourGradient panelGrad (Theme::surfaceRaised().brighter (0.06f), r.getX(), r.getY(),
                                    Theme::surface().darker (0.08f), r.getX(), r.getBottom(), false);
    g.setGradientFill (panelGrad);
    g.fillRoundedRectangle (r, radius);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine (static_cast<int> (r.getY() + 1.5f), r.getX() + radius, r.getRight() - radius);

    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawHorizontalLine (static_cast<int> (r.getBottom() - 1.0f), r.getX() + radius * 0.5f, r.getRight() - radius * 0.5f);

    const auto accentBar = juce::Rectangle<float> (r.getX() + 14.0f, r.getY() + 12.0f, 4.0f, 16.0f);
    juce::ColourGradient accentGrad (accent.brighter (0.25f), accentBar.getX(), accentBar.getY(),
                                     accent.darker (0.20f), accentBar.getX(), accentBar.getBottom(), false);
    g.setGradientFill (accentGrad);
    g.fillRoundedRectangle (accentBar, 2.0f);

    g.setColour (accent.withAlpha (0.35f));
    g.fillRoundedRectangle (accentBar.expanded (1.5f, 2.0f), 3.0f);

    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (title, static_cast<int> (r.getX() + 26.0f), static_cast<int> (r.getY() + 10.0f),
                static_cast<int> (r.getWidth() - 30.0f), 18, juce::Justification::centredLeft, false);
    g.setColour (Theme::text().withAlpha (0.94f));
    g.drawText (title, static_cast<int> (r.getX() + 25.0f), static_cast<int> (r.getY() + 9.0f),
                static_cast<int> (r.getWidth() - 30.0f), 18, juce::Justification::centredLeft, false);
}

void drawSectionLabel (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text, juce::Colour colour)
{
    g.setFont (juce::Font (8.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.drawText (text, area.translated (0, 1), juce::Justification::centred, false);
    g.setColour (colour);
    g.drawText (text, area, juce::Justification::centred, false);
}

void drawMeterStrip (juce::Graphics& g, juce::Rectangle<float> area)
{
    drawSoftShadow (g, area, 8.0f, 4.0f, 0.35f, 1.0f);
    drawInsetSurface (g, area, 8.0f, Theme::backgroundDeep().darker (0.05f));

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (area.getX() + 2.0f, area.getY() + 2.0f, area.getWidth() - 4.0f, area.getHeight() * 0.12f);
}

void drawLcdReadout (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text)
{
    const auto r = area.toFloat().reduced (0.5f);

    drawSoftShadow (g, r, 5.0f, 2.0f, 0.30f, 0.5f);
    drawInsetSurface (g, r, 5.0f, Theme::lcdBg());

    g.setColour (Theme::accent().withAlpha (0.06f));
    g.fillRoundedRectangle (r.reduced (2.0f), 4.0f);

    for (int y = static_cast<int> (r.getY()) + 2; y < static_cast<int> (r.getBottom()) - 2; y += 3)
    {
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.drawHorizontalLine (y, r.getX() + 3.0f, r.getRight() - 3.0f);
    }

    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (Theme::accentDeep().withAlpha (0.35f));
    g.drawText (text, area.translated (0, 1), juce::Justification::centred, false);
    g.setColour (Theme::lcdText());
    g.drawText (text, area, juce::Justification::centred, false);
}

void drawVUMeter (juce::Graphics& g, float x, float y, float w, float h, float db, float peakHoldDb)
{
    const auto slot = juce::Rectangle<float> (x - 3.0f, y - 3.0f, w + 6.0f, h + 6.0f);
    drawInsetSurface (g, slot, 4.0f, Theme::backgroundDeep().darker (0.08f));

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (x + 1.0f, y + 1.0f, w - 2.0f, h * 0.10f);

    const float norm = juce::jlimit (0.0f, 1.0f, (juce::jlimit (-60.0f, 6.0f, db) + 60.0f) / 66.0f);

    if (norm > 0.002f)
    {
        const float fillH = norm * h;
        const auto fill = juce::Rectangle<float> (x + 1.0f, y + h - fillH, w - 2.0f, fillH);

        g.setColour (Theme::meterSafe().withAlpha (0.18f));
        g.fillRoundedRectangle (fill.expanded (1.5f, 2.0f), 2.5f);

        juce::ColourGradient meterGrad (Theme::meterSafe(), fill.getBottomLeft().x, fill.getBottomLeft().y,
                                        Theme::meterHot(), fill.getTopLeft().x, fill.getTopLeft().y, false);
        g.setGradientFill (meterGrad);
        g.fillRoundedRectangle (fill, 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (fill.getX(), fill.getY(), fill.getWidth(), juce::jmin (4.0f, fill.getHeight()));
    }

    const float holdNorm = juce::jlimit (0.0f, 1.0f, (juce::jlimit (-60.0f, 6.0f, peakHoldDb) + 60.0f) / 66.0f);

    if (holdNorm > 0.01f)
    {
        const float holdY = y + h - holdNorm * h;
        juce::Colour holdCol = holdNorm > 0.78f ? Theme::meterHot() : (holdNorm > 0.55f ? Theme::meterWarn() : Theme::text());

        g.setColour (holdCol.withAlpha (0.35f));
        g.fillRoundedRectangle (x - 1.0f, holdY - 2.0f, w + 2.0f, 4.0f, 2.0f);
        g.setColour (holdCol);
        g.fillRoundedRectangle (x, holdY - 1.0f, w, 2.0f, 1.0f);
    }
}

void drawPowerIndicator (juce::Graphics& g, float cx, float cy, float radius, bool lit)
{
    if (lit)
    {
        g.setColour (Theme::powerOn().withAlpha (0.18f));
        g.fillEllipse (cx - radius * 2.4f, cy - radius * 2.4f, radius * 4.8f, radius * 4.8f);
        g.setColour (Theme::powerOn().withAlpha (0.10f));
        g.fillEllipse (cx - radius * 3.2f, cy - radius * 3.2f, radius * 6.4f, radius * 6.4f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillEllipse (cx - radius + 0.5f, cy - radius + 1.5f, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient dome (lit ? Theme::powerOn().brighter (0.35f) : Theme::surfaceRaised(),
                               cx - radius * 0.4f, cy - radius * 0.5f,
                               lit ? Theme::powerOn().darker (0.25f) : Theme::surface(),
                               cx + radius * 0.3f, cy + radius * 0.4f, true);
    g.setGradientFill (dome);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (lit ? 0.35f : 0.10f));
    g.fillEllipse (cx - radius * 0.35f, cy - radius * 0.45f, radius * 0.55f, radius * 0.40f);

    g.setColour (Theme::borderLight().withAlpha (0.35f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.9f);
}

void drawFloatingControl (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text, bool pressed)
{
    drawRaisedSurface (g, area.toFloat().reduced (0.5f), 6.0f,
                       Theme::surfaceRaised().brighter (0.05f), Theme::surface(), pressed);

    g.setFont (juce::Font (9.5f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.drawText (text, area.translated (0, 1), juce::Justification::centred, false);
    g.setColour (Theme::textDim());
    g.drawText (text, area, juce::Justification::centred, false);
}

PlateLookAndFeel::PlateLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, Theme::accent());
    setColour (juce::Slider::rotarySliderFillColourId, Theme::accent());
    setColour (juce::Slider::rotarySliderOutlineColourId, Theme::border());
    setColour (juce::Slider::textBoxTextColourId, Theme::text());
    setColour (juce::Slider::textBoxBackgroundColourId, Theme::lcdBg());
    setColour (juce::Slider::textBoxOutlineColourId, Theme::border());
    setColour (juce::Slider::textBoxHighlightColourId, Theme::accent().withAlpha (0.25f));
    setColour (juce::Label::textColourId, Theme::textDim());
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::textColourId, Theme::text());
    setColour (juce::ComboBox::backgroundColourId, Theme::surface());
    setColour (juce::ComboBox::outlineColourId, Theme::border());
    setColour (juce::ComboBox::arrowColourId, Theme::accentBright());
    setColour (juce::PopupMenu::backgroundColourId, Theme::surfaceRaised());
    setColour (juce::PopupMenu::textColourId, Theme::text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accent().withAlpha (0.22f));
    setColour (juce::TextButton::buttonColourId, Theme::surfaceRaised());
    setColour (juce::TextButton::buttonOnColourId, Theme::accent().withAlpha (0.25f));
    setColour (juce::TextButton::textColourOnId, Theme::text());
    setColour (juce::TextButton::textColourOffId, Theme::textDim());
}

void PlateLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    const float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float radius = juce::jmin (static_cast<float> (width), static_cast<float> (height)) * 0.5f - 6.0f;
    const bool isLarge = static_cast<float> (width) >= 52.0f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const juce::Colour accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    drawKnob3D (g, centreX, centreY, radius, angle, rotaryStartAngle, rotaryEndAngle, sliderPosProportional, accent, isLarge);
}

void PlateLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isDown,
                                     int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (1.0f, 1.0f, static_cast<float> (width) - 2.0f, static_cast<float> (height) - 2.0f);

    if (isDown)
        drawInsetSurface (g, bounds, 7.0f, Theme::backgroundDeep());
    else
        drawRaisedSurface (g, bounds, 7.0f, Theme::surfaceRaised(), Theme::surface(), false);

    const juce::Rectangle<float> arrowZone (static_cast<float> (width) - 24.0f, 0.0f, 20.0f, static_cast<float> (height));
    juce::Path path;
    path.startNewSubPath (arrowZone.getX() + 4.0f, arrowZone.getCentreY() - 2.0f);
    path.lineTo (arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
    path.lineTo (arrowZone.getRight() - 4.0f, arrowZone.getCentreY() - 2.0f);
    g.setColour (Theme::accentBright().withAlpha (box.isEnabled() ? 0.95f : 0.30f));
    g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PlateLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (12, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

void PlateLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool highlighted, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    const float radius = 7.0f;

    if (isDown)
    {
        drawInsetSurface (g, bounds, radius, Theme::backgroundDeep());
        return;
    }

    juce::Colour top = on ? Theme::accent().withAlpha (0.35f) : Theme::surfaceRaised().brighter (highlighted ? 0.10f : 0.05f);
    juce::Colour bottom = on ? Theme::accentDeep().withAlpha (0.45f) : Theme::surface();
    drawRaisedSurface (g, bounds, radius, top, bottom, false);

    if (on)
    {
        g.setColour (Theme::accentBright().withAlpha (0.20f));
        g.drawRoundedRectangle (bounds.reduced (1.0f), radius - 1.0f, 1.0f);
    }
}

void PlateLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool isDown)
{
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    auto area = button.getLocalBounds();
    if (isDown)
        area = area.translated (0, 1);

    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.drawText (button.getButtonText(), area.translated (0, 1), juce::Justification::centred, false);
    g.setColour (button.getToggleState() ? Theme::text() : Theme::textDim());
    g.drawText (button.getButtonText(), area, juce::Justification::centred, false);
}

void PlateLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool highlighted, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    const float radius = 8.0f;

    if (isDown)
        drawInsetSurface (g, bounds, radius, Theme::backgroundDeep());
    else
        drawRaisedSurface (g, bounds, radius,
                             on ? Theme::accent().withAlpha (0.32f) : Theme::surfaceRaised().brighter (highlighted ? 0.08f : 0.03f),
                             on ? Theme::accentDeep().withAlpha (0.40f) : Theme::surface(),
                             false);

    const float ledSize = 7.0f;
    const float ledX = bounds.getX() + 10.0f;
    const float ledY = bounds.getCentreY();

    if (on)
    {
        g.setColour (Theme::accent().withAlpha (0.25f));
        g.fillEllipse (ledX - ledSize, ledY - ledSize, ledSize * 2.0f, ledSize * 2.0f);

        juce::ColourGradient led (Theme::accentBright(), ledX - 2.0f, ledY - 3.0f, Theme::accentDeep(), ledX + 2.0f, ledY + 2.0f, true);
        g.setGradientFill (led);
        g.fillEllipse (ledX - ledSize * 0.55f, ledY - ledSize * 0.55f, ledSize * 1.1f, ledSize * 1.1f);

        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillEllipse (ledX - 2.0f, ledY - 3.0f, 3.0f, 2.5f);
    }
    else
    {
        g.setColour (Theme::backgroundDeep());
        g.fillEllipse (ledX - ledSize * 0.55f, ledY - ledSize * 0.55f, ledSize * 1.1f, ledSize * 1.1f);
        g.setColour (Theme::border().withAlpha (0.70f));
        g.drawEllipse (ledX - ledSize * 0.55f, ledY - ledSize * 0.55f, ledSize * 1.1f, ledSize * 1.1f, 0.9f);
    }

    g.setFont (juce::Font (8.5f, juce::Font::bold));
    auto textArea = bounds.withTrimmedLeft (on ? 22.0f : 10.0f).toNearestInt();
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.drawText (button.getButtonText(), textArea.translated (0, 1), juce::Justification::centredLeft, false);
    g.setColour (on ? Theme::text() : Theme::textDim());
    g.drawText (button.getButtonText(), textArea, juce::Justification::centredLeft, false);
}

juce::Font PlateLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (11.0f);
}

juce::Font PlateLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (10.0f);
}

juce::Font PlateLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (9.0f, juce::Font::bold);
}

juce::Font PlateLookAndFeel::getPopupMenuFont()
{
    return juce::Font (12.0f);
}

PlateKnob::PlateKnob (const juce::String& name, juce::LookAndFeel& laf, juce::Colour accentColour)
    : accent (accentColour)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 13);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setLookAndFeel (&laf);
    addAndMakeVisible (slider);

    label.setText (name.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, Theme::textDim().withAlpha (0.90f));
    label.setFont (juce::Font (8.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

void PlateKnob::resized()
{
    auto area = getLocalBounds();
    const int labelH = label.getText().isEmpty() ? 0 : 11;
    slider.setBounds (area.removeFromTop (area.getHeight() - labelH));
    label.setBounds (area);
    label.setVisible (labelH > 0);
}

void PlateKnob::setLabelText (const juce::String& text)
{
    label.setText (text.toUpperCase(), juce::dontSendNotification);
    resized();
}

} // namespace PlateUi
