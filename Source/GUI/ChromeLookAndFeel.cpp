#include "ChromeLookAndFeel.h"
#include <BinaryData.h>

namespace chrome
{
ChromeLookAndFeel::ChromeLookAndFeel()
{
    rajdhaniMedium = juce::Typeface::createSystemTypefaceFor (BinaryData::RajdhaniMedium_ttf, BinaryData::RajdhaniMedium_ttfSize);
    rajdhaniSemiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::RajdhaniSemiBold_ttf, BinaryData::RajdhaniSemiBold_ttfSize);
    rajdhaniBold = juce::Typeface::createSystemTypefaceFor (BinaryData::RajdhaniBold_ttf, BinaryData::RajdhaniBold_ttfSize);
    dmMonoRegular = juce::Typeface::createSystemTypefaceFor (BinaryData::DMMonoRegular_ttf, BinaryData::DMMonoRegular_ttfSize);
    dmMonoMedium = juce::Typeface::createSystemTypefaceFor (BinaryData::DMMonoMedium_ttf, BinaryData::DMMonoMedium_ttfSize);
    setDefaultSansSerifTypeface (rajdhaniMedium);
    const auto ink = juce::Colour (0xff323437);
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x15ffffff));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xfff3f3f5));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xffececee));
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xff51555a));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff273747));
}

juce::Typeface::Ptr ChromeLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    if (font.getTypefaceName().containsIgnoreCase ("DM Mono")) return font.isBold() ? dmMonoMedium : dmMonoRegular;
    return font.isBold() ? rajdhaniBold : rajdhaniMedium;
}

juce::Font ChromeLookAndFeel::getLabelFont (juce::Label& label)
{
    const auto height = juce::jmax (9.0f, label.getFont().getHeight());
    const auto numeric = dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr;
    return juce::Font (juce::FontOptions (numeric ? dmMonoMedium : rajdhaniSemiBold).withHeight (height));
}

void ChromeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float position,
                                           float startAngle, float endAngle, juce::Slider& slider)
{
    const auto diameter = static_cast<float> (juce::jmin (width, height)) - 10.0f;
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y), static_cast<float> (width), static_cast<float> (height))
                            .withSizeKeepingCentre (diameter, diameter);
    g.setColour (juce::Colour (0x33000000)); g.fillEllipse (bounds.translated (0.0f, 3.0f).expanded (2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff9fafb), bounds.getX(), bounds.getY(), juce::Colour (0xff73777b), bounds.getRight(), bounds.getBottom(), false));
    g.fillEllipse (bounds.expanded (1.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff777b7f), bounds.getX(), bounds.getY(), juce::Colour (0xfffafafa), bounds.getCentreX(), bounds.getCentreY(), true));
    g.fillEllipse (bounds.reduced (3.0f));
    const auto centre = bounds.getCentre();
    for (int segment = 0; segment < 10; ++segment)
    {
        const auto a1 = juce::MathConstants<float>::twoPi * static_cast<float> (segment) / 10.0f;
        const auto a2 = juce::MathConstants<float>::twoPi * static_cast<float> (segment + 1) / 10.0f;
        juce::Path reflection; reflection.startNewSubPath (centre); reflection.lineTo (centre.getPointOnCircumference (diameter * 0.43f, a1)); reflection.lineTo (centre.getPointOnCircumference (diameter * 0.43f, a2)); reflection.closeSubPath();
        g.setColour ((segment % 2 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha (0.022f)); g.fillPath (reflection);
    }
    g.setColour (juce::Colour (0xff4a4d50)); g.drawEllipse (bounds, 1.0f);
    g.setColour (juce::Colour (0x449bd4ff)); g.drawEllipse (bounds.expanded (4.0f), slider.isMouseOverOrDragging() ? 2.0f : 0.8f);
    for (int tick = 0; tick <= 10; ++tick)
    {
        const auto tickAngle = startAngle + static_cast<float> (tick) / 10.0f * (endAngle - startAngle);
        const auto outer = centre.getPointOnCircumference (diameter * 0.58f, tickAngle);
        const auto inner = centre.getPointOnCircumference (diameter * 0.53f, tickAngle);
        g.setColour (juce::Colour (0xff85898d)); g.drawLine ({ inner, outer }, 0.8f);
    }
    const auto angle = startAngle + position * (endAngle - startAngle);
    juce::Path marker;
    marker.addRoundedRectangle (-1.1f, -diameter * 0.39f, 2.2f, diameter * 0.25f, 1.0f);
    g.setColour (slider.isEnabled() ? juce::Colour (0xff548ae4) : juce::Colour (0xff8b8e91));
    g.fillPath (marker, juce::AffineTransform::rotation (angle).translated (bounds.getCentreX(), bounds.getCentreY()));
}

void ChromeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted, bool down)
{
    if (button.getProperties().contains ("logoHitArea"))
        return;
    const auto b = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto active = down || button.getToggleState();
    if (button.getProperties().contains ("chromePower"))
    {
        const auto size = juce::jmin (b.getWidth(), b.getHeight());
        const auto circle = b.withSizeKeepingCentre (size, size).reduced (2.0f);
        g.setColour (juce::Colour (0x33000000)); g.fillEllipse (circle.translated (0.0f, 3.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffcfcfd), circle.getX(), circle.getY(), juce::Colour (0xff777b80), circle.getRight(), circle.getBottom(), false)); g.fillEllipse (circle);
        g.setColour ((active ? juce::Colour (0xff85b3ff) : juce::Colour (0xff72767b)).withAlpha (highlighted ? 1.0f : 0.75f)); g.drawEllipse (circle.expanded (2.0f), active ? 2.0f : 1.0f);
        const auto c = circle.getCentre(); g.setColour (juce::Colour (0xff303438)); g.drawLine (c.x, c.y - 10.0f, c.x, c.y + 1.0f, 2.3f);
        juce::Path powerArc; powerArc.addCentredArc (c.x, c.y, circle.getWidth() * 0.22f, circle.getHeight() * 0.22f, 0.0f, 0.72f, 5.56f, true);
        g.strokePath (powerArc, juce::PathStrokeType (2.3f));
        return;
    }
    auto top = active ? juce::Colour (0xffc4d9ff) : juce::Colour (0xfff3f3f5);
    if (highlighted) top = top.brighter (0.08f);
    g.setColour (juce::Colour (0x28000000)); g.fillRoundedRectangle (b.translated (0.0f, 2.0f), 7.0f);
    g.setGradientFill (juce::ColourGradient (top, b.getX(), b.getY(), juce::Colour (0xffb9bbbe), b.getX(), b.getBottom(), false)); g.fillRoundedRectangle (b, 7.0f);
    g.setColour (active ? juce::Colour (0xff85b3ff) : juce::Colour (0xff8d9093)); g.drawRoundedRectangle (b, 7.0f, active ? 1.5f : 0.8f);
}

void ChromeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto b = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width - 1), static_cast<float> (height - 1));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3f3f5), 0, 0, juce::Colour (0xffc0c1c4), 0, static_cast<float> (height), false));
    g.fillRoundedRectangle (b, 8.0f); g.setColour (juce::Colour (0xff999ca0)); g.drawRoundedRectangle (b, 8.0f, 1.0f);
    juce::Path arrow; arrow.addTriangle (static_cast<float> (width - 20), height * 0.42f, static_cast<float> (width - 10), height * 0.42f, static_cast<float> (width - 15), height * 0.62f);
    g.setColour (box.findColour (juce::ComboBox::textColourId)); g.fillPath (arrow);
}
}
