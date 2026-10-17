#include "ChromeLookAndFeel.h"
#include <BinaryData.h>

namespace chrome
{
namespace
{
std::unique_ptr<juce::Drawable> drawableFromData (const void* data, int size)
{
    return juce::Drawable::createFromImageData (data, static_cast<size_t> (size));
}
}

ChromeLookAndFeel::ChromeLookAndFeel()
{
    inter = juce::Typeface::createSystemTypefaceFor (BinaryData::InterVariable_ttf, BinaryData::InterVariable_ttfSize);
    setDefaultSansSerifTypeface (inter);

    smallFace = drawableFromData (BinaryData::knob_small_face_png, BinaryData::knob_small_face_pngSize);
    smallRim = drawableFromData (BinaryData::knob_small_rim_svg, BinaryData::knob_small_rim_svgSize);
    smallShadow = drawableFromData (BinaryData::knob_small_shadow_svg, BinaryData::knob_small_shadow_svgSize);
    smallPatina = drawableFromData (BinaryData::knob_small_patina_svg, BinaryData::knob_small_patina_svgSize);
    smallSpecular = drawableFromData (BinaryData::knob_small_specular_svg, BinaryData::knob_small_specular_svgSize);
    bigFace = drawableFromData (BinaryData::knob_big_face_png, BinaryData::knob_big_face_pngSize);
    bigRim = drawableFromData (BinaryData::knob_big_rim_svg, BinaryData::knob_big_rim_svgSize);
    bigShadow = drawableFromData (BinaryData::knob_big_shadow_svg, BinaryData::knob_big_shadow_svgSize);
    bigPatina = drawableFromData (BinaryData::knob_big_patina_svg, BinaryData::knob_big_patina_svgSize);
    bigSpecular = drawableFromData (BinaryData::knob_big_specular_svg, BinaryData::knob_big_specular_svgSize);
    bigHalo = drawableFromData (BinaryData::knob_big_halo_svg, BinaryData::knob_big_halo_svgSize);

    const auto ink = juce::Colour (0xff292b2e);
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xffd8d7d7));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xffe3e2e2));
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xff3d4044));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xffeff5ff));
}

juce::Typeface::Ptr ChromeLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    juce::ignoreUnused (font);
    return inter;
}

juce::Font ChromeLookAndFeel::getLabelFont (juce::Label& label)
{
    const auto height = juce::jmax (9.0f, label.getFont().getHeight());
    return juce::Font (juce::FontOptions (inter).withHeight (height));
}

juce::Font ChromeLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    const auto characterMode = box.getProperties().contains ("characterMode");
    const auto height = characterMode ? static_cast<float> (box.getHeight()) * 0.45f
                                      : juce::jmin (16.0f, static_cast<float> (box.getHeight()) * 0.62f);
    return juce::Font (juce::FontOptions (inter).withHeight (height));
}

void ChromeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    if (box.getProperties().contains ("characterMode"))
    {
        const auto left = juce::jmax (5, juce::roundToInt (static_cast<float> (box.getWidth()) * 0.085f));
        const auto arrowSpace = juce::jmax (17, juce::roundToInt (static_cast<float> (box.getWidth()) * 0.21f));
        label.setBounds (left, 1, box.getWidth() - left - arrowSpace, box.getHeight() - 2);
    }
    else
    {
        juce::LookAndFeel_V4::positionComboBoxText (box, label);
    }
    label.setFont (getComboBoxFont (box));
}

void ChromeLookAndFeel::drawAsset (juce::Graphics& g, const std::unique_ptr<juce::Drawable>& asset, juce::Rectangle<float> bounds)
{
    if (asset != nullptr)
        asset->drawWithin (g, bounds, juce::RectanglePlacement::stretchToFit, 1.0f);
}

void ChromeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float position,
                                           float startAngle, float endAngle, juce::Slider& slider)
{
    const auto isBig = static_cast<bool> (slider.getProperties().getWithDefault ("bigKnob", false));
    const auto glows = static_cast<bool> (slider.getProperties().getWithDefault ("glowKnob", false));
    const auto diameter = static_cast<float> (juce::jmin (width, height)) - (isBig ? 2.0f : 1.0f);
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y), static_cast<float> (width), static_cast<float> (height))
                            .withSizeKeepingCentre (diameter, diameter);
    const auto& face = isBig ? bigFace : smallFace;
    const auto& rim = isBig ? bigRim : smallRim;
    const auto& shadow = isBig ? bigShadow : smallShadow;
    const auto& patina = isBig ? bigPatina : smallPatina;
    const auto& specular = isBig ? bigSpecular : smallSpecular;
    auto relative = [&bounds] (float left, float top, float widthRatio, float heightRatio)
    {
        return juce::Rectangle<float> (bounds.getX() + bounds.getWidth() * left,
                                       bounds.getY() + bounds.getHeight() * top,
                                       bounds.getWidth() * widthRatio,
                                       bounds.getHeight() * heightRatio);
    };
    if (isBig)
    {
        if (glows)
        {
            const auto glowBounds = bounds.reduced (diameter * 0.025f);
            g.setColour (juce::Colour (0x245c8fe9));
            g.drawEllipse (glowBounds, juce::jmax (1.0f, diameter * 0.045f));
            g.setColour (juce::Colour (0xb59fc2ff));
            g.drawEllipse (glowBounds, juce::jmax (0.8f, diameter * 0.010f));
        }
        drawAsset (g, shadow, relative (0.1195f, 0.1547f, 0.7729f, 0.7729f));
        drawAsset (g, rim, relative (0.1329f, 0.1329f, 0.7342f, 0.7342f));
        drawAsset (g, face, relative (0.1564f, 0.1564f, 0.6873f, 0.6873f));
        drawAsset (g, patina, relative (0.1564f, 0.1564f, 0.6873f, 0.6873f));
        drawAsset (g, specular, relative (0.3482f, 0.5253f, 0.3920f, 0.2150f));
    }
    else
    {
        drawAsset (g, shadow, relative (0.0900f, 0.1383f, 0.8200f, 0.8200f));
        drawAsset (g, rim, relative (0.0948f, 0.0948f, 0.8103f, 0.8103f));
        drawAsset (g, face, relative (0.1700f, 0.1700f, 0.6600f, 0.6600f));
        drawAsset (g, patina, relative (0.1700f, 0.1700f, 0.6600f, 0.6600f));
        drawAsset (g, specular, relative (0.3416f, 0.5264f, 0.4092f, 0.2244f));
    }

    const auto centre = bounds.getCentre();
    const auto tickCount = isBig ? 25 : 13;
    for (int tick = 0; tick < tickCount; ++tick)
    {
        const auto tickAngle = startAngle + static_cast<float> (tick) / static_cast<float> (tickCount - 1) * (endAngle - startAngle);
        const auto point = centre.getPointOnCircumference (diameter * (isBig ? 0.465f : 0.395f), tickAngle);
        const auto dot = isBig ? diameter * 0.017f : diameter * 0.018f;
        g.setColour (glows && isBig ? juce::Colour (0x989fc2ff) : juce::Colour (0xa06f7276));
        g.fillEllipse (point.x - dot * 0.5f, point.y - dot * 0.5f, dot, dot);
    }

    const auto angle = startAngle + position * (endAngle - startAngle);
    juce::Path marker;
    const auto markerWidth = juce::jmax (1.2f, diameter * 0.016f);
    marker.addRoundedRectangle (-markerWidth * 0.5f, -diameter * (isBig ? 0.335f : 0.315f),
                                markerWidth, diameter * (isBig ? 0.18f : 0.17f), markerWidth * 0.5f);
    g.setColour (slider.isEnabled() ? juce::Colour (0xff5f91e8) : juce::Colour (0xff8b8e91));
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
    if (button.getProperties().contains ("footerTab"))
    {
        const auto top = active ? juce::Colour (0xff526f9f) : juce::Colour (0xff4c4d50);
        const auto bottom = active ? juce::Colour (0xff273b5d) : juce::Colour (0xff27292c);
        g.setColour (juce::Colour (0x66000000));
        g.fillRoundedRectangle (b.translated (0.0f, 1.5f), 7.0f);
        g.setGradientFill (juce::ColourGradient (top, b.getX(), b.getY(), bottom, b.getX(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 7.0f);
        g.setColour (active ? juce::Colour (0xff9fc2ff) : juce::Colour (0xff73767a));
        g.drawRoundedRectangle (b, 7.0f, active ? 1.25f : 0.8f);
        return;
    }
    auto top = active ? juce::Colour (0xff5b76a2) : juce::Colour (0xffe2e1e1);
    if (highlighted) top = top.brighter (0.08f);
    g.setColour (juce::Colour (0x26000000)); g.fillRoundedRectangle (b.translated (0.0f, 1.5f), 5.0f);
    g.setGradientFill (juce::ColourGradient (top, b.getX(), b.getY(), active ? juce::Colour (0xff33435e) : juce::Colour (0xffb8b7b7), b.getX(), b.getBottom(), false)); g.fillRoundedRectangle (b, 5.0f);
    g.setColour (active ? juce::Colour (0xff9fc2ff) : juce::Colour (0xff929292)); g.drawRoundedRectangle (b, 5.0f, active ? 1.15f : 0.75f);
    if (button.getProperties().contains ("saveIcon"))
    {
        const auto icon = b.withSizeKeepingCentre (juce::jmin (b.getWidth(), b.getHeight()) * 0.42f,
                                                    juce::jmin (b.getWidth(), b.getHeight()) * 0.42f);
        g.setColour (juce::Colour (0xff3d4044));
        g.drawRoundedRectangle (icon, 1.2f, 1.2f);
        g.fillRect (icon.getX() + icon.getWidth() * 0.22f, icon.getY() + icon.getHeight() * 0.10f,
                    icon.getWidth() * 0.48f, icon.getHeight() * 0.25f);
        g.drawRect (icon.getX() + icon.getWidth() * 0.24f, icon.getY() + icon.getHeight() * 0.56f,
                    icon.getWidth() * 0.52f, icon.getHeight() * 0.30f, 1.0f);
    }
}

void ChromeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto b = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width - 1), static_cast<float> (height - 1));
    const auto dark = box.getProperties().contains ("darkCombo");
    const auto characterMode = box.getProperties().contains ("characterMode");
    if (characterMode)
    {
        juce::ColourGradient fill (juce::Colour (0xffd6d4d1), b.getX(), b.getCentreY(),
                                   juce::Colour (0xffb0b0ad), b.getRight(), b.getCentreY(), false);
        fill.addColour (0.48, juce::Colour (0xffc7c4c2));
        g.setGradientFill (fill);
        g.fillRoundedRectangle (b, 11.0f);
        g.setColour (juce::Colour (0xff5c5e61));
        g.drawRoundedRectangle (b, 11.0f, 0.95f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (dark ? juce::Colour (0xff57595c) : juce::Colour (0xffe7e6e6), 0, 0,
                                                 dark ? juce::Colour (0xff26282b) : juce::Colour (0xffb9b8b8), 0, static_cast<float> (height), false));
        g.fillRoundedRectangle (b, 7.0f);
        g.setColour (juce::Colour (0xff8e8e8f));
        g.drawRoundedRectangle (b, 7.0f, 0.8f);
    }
    juce::Path arrow; arrow.addTriangle (static_cast<float> (width - 20), height * 0.42f, static_cast<float> (width - 10), height * 0.42f, static_cast<float> (width - 15), height * 0.62f);
    g.setColour (dark ? juce::Colour (0xffeceef1) : box.findColour (juce::ComboBox::textColourId)); g.fillPath (arrow);
}
}
