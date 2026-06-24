#include "PluginEditor.h"
#include "Ui/PlateLookAndFeel.h"

// Out-of-class definitions (C++14/17 ODR safety)
constexpr float       AutoGainAudioProcessorEditor::kZoomFactors[];
constexpr const char* AutoGainAudioProcessorEditor::kZoomLabels[];

//==============================================================================
//  Colour palette
//==============================================================================
using T = PlateUi::Theme;
static const juce::Colour kBg       = T::background();
static const juce::Colour kPanel    = T::surface();
static const juce::Colour kHeader   = T::surfaceRaised();
static const juce::Colour kAccent   = T::accent();
static const juce::Colour kTextMain = T::text();
static const juce::Colour kTextDim  = T::textDim();
static const juce::Colour kDivider  = T::border();
static const juce::Colour kBoost    = T::accentBright(); // gain boost
static const juce::Colour kCut      = T::meterHot();    // gain cut (warm red)

// Use the shared warm-dark PlateLookAndFeel
using AutoGainLAF = PlateUi::PlateLookAndFeel;

//==============================================================================
//  Gain Display — history graph + dual VU meters
//==============================================================================
class GainDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit GainDisplay (AutoGainAudioProcessor& p) : proc (p) { startTimerHz (30); }

private:
    void timerCallback() override { repaint(); }

    //--------------------------------------------------------------------------
    // Draw one vertical VU meter bar
    //--------------------------------------------------------------------------
    static void drawVU (juce::Graphics& g,
                        float x, float y, float w, float h,
                        float db, const juce::String& labelStr)
    {
        static constexpr float kDbMin = -60.f, kDbMax = 6.f, kDbRange = kDbMax - kDbMin;

        g.setColour (juce::Colour (4, 5, 12));
        g.fillRoundedRectangle (x, y, w, h, 3.f);
        g.setColour (kDivider.withAlpha (0.28f));
        g.drawRoundedRectangle (x, y, w, h, 3.f, 0.7f);

        // Fill bar (colour gradient: green bottom -> yellow -> red top)
        const float norm  = juce::jlimit (0.f, 1.f, (juce::jlimit (kDbMin, kDbMax, db) - kDbMin) / kDbRange);
        const float fillH = norm * h;
        const float fillY = y + h - fillH;

        if (fillH > 0.5f)
        {
            juce::ColourGradient grad (
                juce::Colour (220,  55,  55), x, y,
                juce::Colour ( 65, 200,  80), x, y + h, false);
            grad.addColour (0.09f, juce::Colour ( 65, 200,  80));
            grad.addColour (0.54f, juce::Colour ( 65, 200,  80));
            grad.addColour (0.68f, juce::Colour (225, 200,  50));
            grad.addColour (0.82f, juce::Colour (220, 120,  50));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (x, fillY, w, fillH, 2.5f);
        }

        // Tick marks at multiples of 6 dB
        for (float mark : { 0.f, -6.f, -12.f, -18.f, -24.f, -36.f, -48.f })
        {
            bool isZero = (std::abs (mark) < 0.01f);
            float ym    = y + h * (1.f - (mark - kDbMin) / kDbRange);
            g.setColour (isZero ? kTextDim.withAlpha (0.5f)
                                : kDivider.withAlpha (0.22f));
            g.fillRect (x + (isZero ? 0.f : w * 0.15f), ym,
                        isZero ? w : w * 0.70f, isZero ? 0.9f : 0.5f);
        }

        // Label (IN / OUT)
        g.setFont (juce::Font (8.5f, juce::Font::bold));
        g.setColour (kTextDim.withAlpha (0.55f));
        g.drawText (labelStr, (int)(x - 2), (int)(y + h + 3), (int)(w + 4), 12,
                    juce::Justification::centred, false);
    }

    //--------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        const float bw = (float)getWidth();
        const float bh = (float)getHeight();

        // Overall component background
        g.setColour (juce::Colour (3, 4, 10));
        g.fillRoundedRectangle (0.f, 0.f, bw, bh, 5.f);

        static constexpr int   N           = AutoGainAudioProcessor::kHistorySize;
        static constexpr float kMaxDispDb  = 24.f;

        // Layout inside the component
        const float vuW  = 22.f;
        const float vuX1 = 5.f;
        const float vuX2 = bw - 5.f - vuW;
        const float vuY  = 6.f;
        const float vuH  = bh - 22.f;   // leave 22px below for label

        // Gain history area (between the two VU meters)
        const float histX = vuX1 + vuW + 6.f;
        const float histW = vuX2 - 6.f - histX;
        const float histY = vuY;
        const float histH = vuH;

        // Internal margins of the history plot (for dB labels)
        const float mx   = 26.f;    // left (dB scale)
        const float mr   = 4.f;     // right
        const float mt   = 6.f;     // top
        const float mb   = 14.f;    // bottom
        const float plotX = histX + mx;
        const float plotW = histW - mx - mr;
        const float plotY = histY + mt;
        const float plotH = histH - mt - mb;

        auto dbToY = [&](float db) -> float {
            return plotY + plotH * 0.5f * (1.f - db / kMaxDispDb);
        };
        const float zeroY = dbToY (0.f);

        // History background
        g.setColour (juce::Colour (3, 5, 10));
        g.fillRoundedRectangle (histX, histY, histW, histH, 4.f);

        // dB grid lines + labels
        for (float db : { 24.f, 18.f, 12.f, 6.f, -6.f, -12.f, -18.f, -24.f })
        {
            float gy = dbToY (db);
            g.setColour (kDivider.withAlpha (0.18f));
            g.fillRect (plotX, gy, plotW, 0.6f);
            g.setFont  (juce::Font (7.5f));
            g.setColour (kTextDim.withAlpha (0.38f));
            juce::String lbl = (db > 0 ? "+" : "") + juce::String ((int)db);
            g.drawText (lbl, (int)histX, (int)(gy - 6), (int)mx - 2, 12,
                        juce::Justification::centredRight, false);
        }
        // 0 dB centre line (brighter)
        g.setColour (kDivider.withAlpha (0.55f));
        g.fillRect (plotX, zeroY, plotW, 1.f);
        g.setFont  (juce::Font (7.5f));
        g.setColour (kTextDim.withAlpha (0.55f));
        g.drawText ("0", (int)histX, (int)(zeroY - 6), (int)mx - 2, 12,
                    juce::Justification::centredRight, false);

        // Footer label
        g.setFont (juce::Font (7.5f));
        g.setColour (kTextDim.withAlpha (0.30f));
        g.drawText ("GAIN HISTORY", (int)plotX, (int)(plotY + plotH + 2), (int)plotW, 12,
                    juce::Justification::centred, false);

        // Read ring buffer
        const int wp = proc.historyWritePos.load();

        auto hx = [&](int i) -> float {
            return plotX + (float)i / (float)(N - 1) * plotW;
        };
        auto hy = [&](float db) -> float {
            return juce::jlimit (plotY, plotY + plotH, dbToY (db));
        };

        // Boost fill (gain > 0 = above 0dB line = blue)
        {
            juce::Path p;
            p.startNewSubPath (hx (0), zeroY);
            for (int i = 0; i < N; ++i)
            {
                int   idx = (wp + i) % N;
                float db  = juce::jmax (0.f, proc.gainHistory[idx]);
                p.lineTo (hx (i), hy (db));
            }
            p.lineTo (hx (N - 1), zeroY);
            p.closeSubPath();
            g.setColour (kBoost.withAlpha (0.22f));
            g.fillPath (p);
        }

        // Cut fill (gain < 0 = below 0dB line = orange)
        {
            juce::Path p;
            p.startNewSubPath (hx (0), zeroY);
            for (int i = 0; i < N; ++i)
            {
                int   idx = (wp + i) % N;
                float db  = juce::jmin (0.f, proc.gainHistory[idx]);
                p.lineTo (hx (i), hy (db));
            }
            p.lineTo (hx (N - 1), zeroY);
            p.closeSubPath();
            g.setColour (kCut.withAlpha (0.22f));
            g.fillPath (p);
        }

        // White gain history line
        {
            juce::Path line;
            for (int i = 0; i < N; ++i)
            {
                int   idx = (wp + i) % N;
                float db  = proc.gainHistory[idx];
                float lx  = hx (i);
                float ly  = hy (db);
                if (i == 0) line.startNewSubPath (lx, ly);
                else        line.lineTo (lx, ly);
            }
            // Glow
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.strokePath (line, juce::PathStrokeType (4.f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            // Main
            g.setColour (juce::Colours::white.withAlpha (0.78f));
            g.strokePath (line, juce::PathStrokeType (1.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // History border
        g.setColour (kDivider.withAlpha (0.35f));
        g.drawRoundedRectangle (histX, histY, histW, histH, 4.f, 0.8f);

        // VU meters
        drawVU (g, vuX1, vuY, vuW, vuH, proc.inputLevelDb.load(),  "IN");
        drawVU (g, vuX2, vuY, vuW, vuH, proc.outputLevelDb.load(), "OUT");

        // Component border
        g.setColour (kDivider.withAlpha (0.28f));
        g.drawRoundedRectangle (0.5f, 0.5f, bw - 1.f, bh - 1.f, 4.5f, 0.8f);
    }

    AutoGainAudioProcessor& proc;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainDisplay)
};

//==============================================================================
//  Logo — 5-bar level-meter icon
//==============================================================================
static void drawLogoIcon (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (kAccent.withAlpha (0.15f));
    g.fillEllipse (r);
    g.setColour (kDivider.withAlpha (0.5f));
    g.drawEllipse (r.reduced (0.5f), 0.8f);

    const float cx     = r.getCentreX(), cy = r.getCentreY();
    const float totalW = r.getWidth()  * 0.62f;
    const float maxH   = r.getHeight() * 0.52f;
    static constexpr int   nBars = 5;
    static constexpr float kBarH[nBars] = { 0.40f, 0.70f, 1.0f, 0.70f, 0.40f };
    const float barW   = totalW / (nBars * 2 - 1);
    const float baseY  = cy + maxH * 0.5f;

    for (int i = 0; i < nBars; ++i)
    {
        float bx = cx - totalW * 0.5f + i * 2.f * barW;
        float fh = maxH * kBarH[i];
        g.setColour (kAccent.withAlpha (0.85f));
        g.fillRect (bx, baseY - fh, barW, fh);
    }
}

//==============================================================================
//  Editor constructor
//==============================================================================
AutoGainAudioProcessorEditor::AutoGainAudioProcessorEditor (AutoGainAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    laf = std::make_unique<AutoGainLAF>();
    setLookAndFeel (laf.get());

    gainDisplay = std::make_unique<GainDisplay> (p);
    addAndMakeVisible (*gainDisplay);

    // Attack knob
    attackSlider.setTextValueSuffix (" ms");
    attackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
    attackSlider.setTooltip ("Attack time: how quickly the gain responds to a louder signal (1-500ms). Shorter = tighter leveling.");
    addAndMakeVisible (attackSlider);
    attackLabel.setText ("ATTACK", juce::dontSendNotification);
    attackLabel.setFont  (juce::Font (9.0f));
    attackLabel.setColour (juce::Label::textColourId, kTextDim);
    attackLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (attackLabel);
    attackAtt = std::make_unique<SliderAtt> (p.apvts, "attack", attackSlider);

    // Release knob
    releaseSlider.setTextValueSuffix (" ms");
    releaseSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
    releaseSlider.setTooltip ("Release time: how slowly the gain recovers after a loud signal fades (50-2000ms). Longer = smoother, less pumping.");
    addAndMakeVisible (releaseSlider);
    releaseLabel.setText ("RELEASE", juce::dontSendNotification);
    releaseLabel.setFont  (juce::Font (9.0f));
    releaseLabel.setColour (juce::Label::textColourId, kTextDim);
    releaseLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (releaseLabel);
    releaseAtt = std::make_unique<SliderAtt> (p.apvts, "release", releaseSlider);

    // Output gain knob
    outputGainSlider.setTextValueSuffix (" dB");
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
    outputGainSlider.setTooltip ("Output level trim applied to the normalized (wet) signal. "
                                 "Mix=0% passes the dry signal through unchanged; "
                                 "Mix=100% gives fully auto-gained audio at this output level.");
    addAndMakeVisible (outputGainSlider);
    outputGainLabel.setText ("TARGET LEVEL", juce::dontSendNotification);
    outputGainLabel.setFont  (juce::Font (9.0f));
    outputGainLabel.setColour (juce::Label::textColourId, kTextDim);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputGainLabel);
    outputGainAtt = std::make_unique<SliderAtt> (p.apvts, "outputGain", outputGainSlider);

    // Mix knob
    mixSlider.setTextValueSuffix (" %");
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
    mixSlider.setTooltip ("Mix: 0% = dry (no leveling), 100% = fully auto-leveled");
    addAndMakeVisible (mixSlider);
    mixLabel.setText ("MIX", juce::dontSendNotification);
    mixLabel.setFont  (juce::Font (9.0f));
    mixLabel.setColour (juce::Label::textColourId, kTextDim);
    mixLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mixLabel);
    mixAtt = std::make_unique<SliderAtt> (p.apvts, "mix", mixSlider);

    zoomIndex = p.editorZoomIndex;
    setSize (kBaseW, kBaseH);
    applyZoom();
    startTimerHz (30);
}

AutoGainAudioProcessorEditor::~AutoGainAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void AutoGainAudioProcessorEditor::visibilityChanged()
{
    if (isVisible())
    {
        applyZoom();
        if (! centred)
        {
            centred = true;
            if (auto* tlw = getTopLevelComponent(); tlw != this)
                if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
                    tlw->setCentrePosition (d->userArea.getCentre());
        }
    }
}
void AutoGainAudioProcessorEditor::parentHierarchyChanged(){ applyZoom(); }
void AutoGainAudioProcessorEditor::applyZoom()             { if (getPeer()) setScaleFactor (kZoomFactors[zoomIndex]); }
void AutoGainAudioProcessorEditor::timerCallback()         { repaint(); }

//==============================================================================
void AutoGainAudioProcessorEditor::resized()
{
    zoomButtonBounds = { 8, 12, 38, 26 };

    // Display: y=52, height=216
    gainDisplay->setBounds (8, 52, kBaseW - 16, 216);

    // Knobs: 4 evenly across 600px  cx = 120, 240, 360, 480
    attackSlider      .setBounds ( 90, 328, 60, 74);
    attackLabel       .setBounds ( 90, 402, 60, 14);
    releaseSlider     .setBounds (210, 328, 60, 74);
    releaseLabel      .setBounds (210, 402, 60, 14);
    outputGainSlider  .setBounds (330, 328, 60, 74);
    outputGainLabel   .setBounds (330, 402, 60, 14);
    mixSlider         .setBounds (450, 328, 60, 74);
    mixLabel          .setBounds (450, 402, 60, 14);
}

//==============================================================================
void AutoGainAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (zoomButtonBounds.contains (e.position.toInt()))
    {
        zoomIndex = (zoomIndex + 1) % 3;
        processorRef.editorZoomIndex = zoomIndex;
        applyZoom();
        repaint();
    }
}

//==============================================================================
void AutoGainAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int W = kBaseW, H = kBaseH;

    // ── Background ───────────────────────────────────────────────────────────
    PlateUi::drawBackground (g, getLocalBounds(), true);

    // ── Header bar ───────────────────────────────────────────────────────────
    PlateUi::drawHeaderBar (g, getLocalBounds(), 50, true);

    // ── Zoom button ──────────────────────────────────────────────────────────
    {
        auto& zb = zoomButtonBounds;
        PlateUi::drawFloatingControl (g, zb, kZoomLabels[zoomIndex], false);
    }

    // ── OMEGADARREN brand ────────────────────────────────────────────────────
    PlateUi::drawBrandMark (g, { 14, 10, 140, 18 }, true);

    // ── Title "AUTO GAIN" ────────────────────────────────────────────────────
    {
        juce::Font titleFont (20.f, juce::Font::bold);
        g.setFont (titleFont);
        const juce::String p1 = "AUTO ";
        const juce::String p2 = "GAIN";
        float w1  = titleFont.getStringWidthFloat (p1);
        float w2  = titleFont.getStringWidthFloat (p2);
        float sx  = (W - w1 - w2) * 0.5f;
        g.setColour (PlateUi::Theme::text().withAlpha (0.88f));
        g.drawText (p1, (int)sx,        18, (int)w1 + 4, 18,
                    juce::Justification::centredLeft, false);
        juce::ColourGradient tGrad (PlateUi::Theme::accentBright(), sx + w1, 18.f,
                                    PlateUi::Theme::accentDeep(),   sx + w1 + w2, 36.f, false);
        g.setGradientFill (tGrad);
        g.drawText (p2, (int)(sx + w1), 18, (int)w2 + 4, 18,
                    juce::Justification::centredLeft, false);
    }

    // ── Version ───────────────────────────────────────────────────────────────
    g.setFont (juce::Font (8.5f));
    g.setColour (PlateUi::Theme::textDim().withAlpha (0.50f));
    g.drawText ("v1.0", W - 52, 38, 40, 10, juce::Justification::centredRight, false);

    // ── Gain applied info strip ───────────────────────────────────────────────
    {
        const float gain = processorRef.gainAppliedDb.load();
        const float mX   = 8.f, mY = 274.f, mW = W - 16.f, mH = 24.f;

        // Panel background
        g.setColour (kPanel.withAlpha (0.55f));
        g.fillRoundedRectangle (mX, mY, mW, mH, 5.f);
        g.setColour (kDivider.withAlpha (0.28f));
        g.drawRoundedRectangle (mX + 0.5f, mY + 0.5f, mW - 1.f, mH - 1.f, 4.5f, 0.8f);

        // Directional fill
        const float cX   = mX + mW * 0.5f;
        const float fillH = mH - 4.f;
        const float fillY = mY + 2.f;
        static constexpr float kMaxBarDb = 24.f;
        float fillW = std::abs (gain) / kMaxBarDb * (mW * 0.5f - 4.f);
        fillW = juce::jlimit (0.f, mW * 0.5f - 4.f, fillW);

        if (gain > 0.1f)
        {
            g.setColour (kBoost.withAlpha (0.38f));
            g.fillRoundedRectangle (cX + 1.f, fillY, fillW, fillH, 3.f);
        }
        else if (gain < -0.1f)
        {
            g.setColour (kCut.withAlpha (0.38f));
            g.fillRoundedRectangle (cX - fillW - 1.f, fillY, fillW, fillH, 3.f);
        }

        // Centre 0dB marker
        g.setColour (kTextDim.withAlpha (0.30f));
        g.fillRect (cX - 0.5f, fillY, 1.f, fillH);

        // Gain value text
        juce::String txt;
        if (gain > 0.05f)       txt = "GAIN APPLIED:  +"  + juce::String (gain, 1) + " dB";
        else if (gain < -0.05f) txt = "GAIN APPLIED:  "   + juce::String (gain, 1) + " dB";
        else                    txt = "GAIN APPLIED:  0.0 dB";

        g.setFont (juce::Font (9.5f, juce::Font::bold));
        g.setColour (gain >  0.1f ? kAccent
                   : gain < -0.1f ? kCut
                   : kTextDim);
        g.drawText (txt, (int)mX, (int)mY, (int)mW, (int)mH,
                    juce::Justification::centred, false);
    }

    // ── Control strip panel ───────────────────────────────────────────────────
    {
        juce::Rectangle<float> panel (6.f, 304.f, W - 12.f, H - 310.f);
        g.setColour (kPanel.withAlpha (0.55f));
        g.fillRoundedRectangle (panel, 6.f);
        g.setColour (kDivider.withAlpha (0.28f));
        g.drawRoundedRectangle (panel.reduced (0.5f), 5.5f, 0.8f);

        // Section header labels for 4 knobs
        g.setFont (juce::Font (9.5f, juce::Font::bold));
        g.setColour (kTextDim.withAlpha (0.60f));
        g.drawText ("ATTACK",  90, 310, 60, 14, juce::Justification::centred, false);
        g.drawText ("RELEASE", 210, 310, 60, 14, juce::Justification::centred, false);
        g.drawText ("TARGET",  330, 310, 60, 14, juce::Justification::centred, false);
        g.drawText ("MIX",     450, 310, 60, 14, juce::Justification::centred, false);

        // Vertical dividers between columns
        g.setColour (kDivider.withAlpha (0.18f));
        g.fillRect (180.f, 310.f, 0.6f, (float)(H - 318));
        g.fillRect (300.f, 310.f, 0.6f, (float)(H - 318));
        g.fillRect (420.f, 310.f, 0.6f, (float)(H - 318));

        // Gain applied readout at bottom of panel
        const float gain   = processorRef.gainAppliedDb.load();
        const float target = processorRef.apvts.getRawParameterValue ("outputGain")->load();

        g.setFont (juce::Font (8.5f));
        g.setColour (kTextDim.withAlpha (0.50f));
        g.drawText ("TARGET:  " + juce::String (target, 1) + " dB",
                    220, 421, 160, 13, juce::Justification::centred, false);

        g.setFont (juce::Font (9.0f, juce::Font::bold));
        g.setColour (gain >  0.1f ? kAccent
                   : gain < -0.1f ? kCut
                   : kTextDim.withAlpha (0.55f));
        juce::String appliedTxt = juce::String ("APPLIED:  ")
            + (gain > 0.05f ? juce::String ("+") : juce::String (""))
            + juce::String (gain, 1) + " dB";
        g.drawText (appliedTxt, 220, 434, 160, 13, juce::Justification::centred, false);
    }
}
