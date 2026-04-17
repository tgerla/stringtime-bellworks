#include "PluginEditor.h"

namespace
{
const auto sectionTitleColour = juce::Colour::fromRGB (228, 226, 212);
const auto bodyTextColour = juce::Colour::fromRGB (213, 220, 225);
const auto accentA = juce::Colour::fromRGB (226, 145, 88);
const auto accentB = juce::Colour::fromRGB (112, 159, 197);
const auto cardBase = juce::Colour::fromRGB (22, 31, 40);
const auto backgroundTop = juce::Colour::fromRGB (41, 52, 60);
const auto backgroundBottom = juce::Colour::fromRGB (16, 21, 28);

std::unique_ptr<juce::Drawable> createWaveIconDrawable (const juce::String& pathData, juce::Colour strokeColour)
{
    const auto svg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 32'>"
                     "<path d='" + pathData + "' fill='none' stroke='#ffffff' stroke-width='3' "
                     "stroke-linecap='round' stroke-linejoin='round'/></svg>";

    auto xml = juce::XmlDocument::parse (svg);
    if (xml == nullptr)
        return {};

    auto drawable = juce::Drawable::createFromSVG (*xml);
    if (drawable != nullptr)
        drawable->replaceColour (juce::Colours::white, strokeColour);

    return drawable;
}
}

ModelingSynthTwoAudioProcessorEditor::ModelingSynthTwoAudioProcessorEditor (ModelingSynthTwoAudioProcessor& p)
    : AudioProcessorEditor (&p),
            processorRef (p),
            keyboard (processorRef.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize (980, 900);
    setWantsKeyboardFocus (true);

    setupSectionLabel (oscillatorSection, "Material");
    setupSectionLabel (envelopeSection, "ADSR");
    setupSectionLabel (filterSection, "Band-Pass");
    setupSectionLabel (sympatheticSection, "Sympathetic / Ebow");
    setupSectionLabel (fxSection, "FX");
    setupSectionLabel (matrixSection, "LFO / Matrix");

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetLabel.setJustificationType (juce::Justification::centredLeft);
    presetLabel.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (presetLabel);

    presetBox.addItemList (processorRef.getPresetNames(), 1);
    presetBox.onChange = [this]
    {
        const auto selected = presetBox.getSelectedItemIndex();
        if (selected >= 0)
            processorRef.applyPreset (selected);
    };
    presetBox.setSelectedItemIndex (0, juce::dontSendNotification);
    addAndMakeVisible (presetBox);

    synthTabButton.setButtonText ("Synth");
    synthTabButton.onClick = [this]
    {
        currentPage = EditorPage::synth;
        updatePageVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible (synthTabButton);

    lfoTabButton.setButtonText ("LFO");
    lfoTabButton.onClick = [this]
    {
        currentPage = EditorPage::lfo;
        updatePageVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible (lfoTabButton);

    fxTabButton.setButtonText ("FX");
    fxTabButton.onClick = [this]
    {
        currentPage = EditorPage::fx;
        updatePageVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible (fxTabButton);

    randomizeButton.setButtonText ("Dice");
    randomizeButton.onClick = [this] { randomizePreset(); };
    addAndMakeVisible (randomizeButton);

    savePresetButton.setButtonText ("Save...");
    savePresetButton.onClick = [this] { savePresetToDisk(); };
    addAndMakeVisible (savePresetButton);

    loadPresetButton.setButtonText ("Load...");
    loadPresetButton.onClick = [this] { loadPresetFromDisk(); };
    addAndMakeVisible (loadPresetButton);

    octaveLabel.setJustificationType (juce::Justification::centredRight);
    octaveLabel.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (octaveLabel);

    lfoWaveformBox.addItemList (juce::StringArray { "Sine", "Triangle", "Sample & Hold", "Off" }, 1);
    lfoWaveformAttachment = std::make_unique<ComboAttachment> (processorRef.parameters, "lfoWaveform", lfoWaveformBox);
    lfoWaveformBox.onChange = [this] { updateWaveformButtons(); };

    lfo2WaveformBox.addItemList (juce::StringArray { "Sine", "Triangle", "Sample & Hold", "Off" }, 1);
    lfo2WaveformAttachment = std::make_unique<ComboAttachment> (processorRef.parameters, "lfo2Waveform", lfo2WaveformBox);
    lfo2WaveformBox.onChange = [this] { updateWaveformButtons(); };

    auto configureOffButton = [this] (juce::TextButton& button, std::function<void()> onClick)
    {
        button.setButtonText ("Off");
        button.setClickingTogglesState (false);
        button.setColour (juce::TextButton::buttonColourId, juce::Colours::black.withAlpha (0.24f));
        button.setColour (juce::TextButton::buttonOnColourId, accentB.withAlpha (0.55f));
        button.setColour (juce::TextButton::textColourOffId, bodyTextColour);
        button.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        button.getProperties().set ("offButton", true);
        button.setTooltip ("Disable this LFO waveform");
        button.onClick = std::move (onClick);
        addAndMakeVisible (button);
    };

    auto configureWaveIconButton = [this] (juce::DrawableButton& button,
                                           const juce::String& pathData,
                                           std::function<void()> onClick)
    {
        button.setClickingTogglesState (false);
        button.setColour (juce::DrawableButton::backgroundColourId, juce::Colours::black.withAlpha (0.24f));
        button.setColour (juce::DrawableButton::backgroundOnColourId, accentB.withAlpha (0.55f));

        auto normal = createWaveIconDrawable (pathData, bodyTextColour.withAlpha (0.9f));
        auto over = createWaveIconDrawable (pathData, juce::Colours::white.withAlpha (0.95f));
        auto on = createWaveIconDrawable (pathData, juce::Colours::white);
        button.setImages (normal.get(), over.get(), nullptr, nullptr, on.get(), on.get(), nullptr, nullptr);

        button.onClick = std::move (onClick);
        addAndMakeVisible (button);
    };

    constexpr auto sinePath = "M2 16 C10 3 22 3 30 16 C38 29 50 29 62 16";
    constexpr auto trianglePath = "M2 24 L18 8 L34 24 L50 8 L62 24";
    constexpr auto sampleHoldPath = "M2 24 L2 10 L18 10 L18 24 L34 24 L34 10 L50 10 L50 24 L62 24";

    configureOffButton (lfo1WaveOffButton, [this] { lfoWaveformBox.setSelectedItemIndex (3, juce::sendNotificationSync); });
    configureWaveIconButton (lfo1WaveSineButton, sinePath, [this] { lfoWaveformBox.setSelectedItemIndex (0, juce::sendNotificationSync); });
    configureWaveIconButton (lfo1WaveTriangleButton, trianglePath, [this] { lfoWaveformBox.setSelectedItemIndex (1, juce::sendNotificationSync); });
    configureWaveIconButton (lfo1WaveSampleHoldButton, sampleHoldPath, [this] { lfoWaveformBox.setSelectedItemIndex (2, juce::sendNotificationSync); });

    configureOffButton (lfo2WaveOffButton, [this] { lfo2WaveformBox.setSelectedItemIndex (3, juce::sendNotificationSync); });
    configureWaveIconButton (lfo2WaveSineButton, sinePath, [this] { lfo2WaveformBox.setSelectedItemIndex (0, juce::sendNotificationSync); });
    configureWaveIconButton (lfo2WaveTriangleButton, trianglePath, [this] { lfo2WaveformBox.setSelectedItemIndex (1, juce::sendNotificationSync); });
    configureWaveIconButton (lfo2WaveSampleHoldButton, sampleHoldPath, [this] { lfo2WaveformBox.setSelectedItemIndex (2, juce::sendNotificationSync); });

    matrixDest1Label.setText ("Slot 1", juce::dontSendNotification);
    matrixDest1Label.setJustificationType (juce::Justification::centred);
    matrixDest1Label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (matrixDest1Label);

    matrixDest2Label.setText ("Slot 2", juce::dontSendNotification);
    matrixDest2Label.setJustificationType (juce::Justification::centred);
    matrixDest2Label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (matrixDest2Label);

    matrixDest3Label.setText ("Slot 3", juce::dontSendNotification);
    matrixDest3Label.setJustificationType (juce::Justification::centred);
    matrixDest3Label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (matrixDest3Label);

    matrixDest4Label.setText ("Slot 4", juce::dontSendNotification);
    matrixDest4Label.setJustificationType (juce::Justification::centred);
    matrixDest4Label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (matrixDest4Label);

    matrixDest5Label.setText ("Slot 5", juce::dontSendNotification);
    matrixDest5Label.setJustificationType (juce::Justification::centred);
    matrixDest5Label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (matrixDest5Label);

    matrixDest6Label.setText ("Slot 6", juce::dontSendNotification);
    matrixDest6Label.setJustificationType (juce::Justification::centred);
    matrixDest6Label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (matrixDest6Label);

    const juce::StringArray matrixDestinations {
        "None",
        "Filter Cutoff",
        "Filter Resonance",
        "Sympathetic Mix",
        "Sympathetic Tone",
        "Sympathetic Link",
        "Ebow Drive",
        "Output Gain",
        "Ebow Focus",
        "Sympathetic Spacing",
        "Sympathetic Detune"
    };

    matrixDest1Box.addItemList (matrixDestinations, 1);
    matrixDest2Box.addItemList (matrixDestinations, 1);
    matrixDest3Box.addItemList (matrixDestinations, 1);
    matrixDest4Box.addItemList (matrixDestinations, 1);
    matrixDest5Box.addItemList (matrixDestinations, 1);
    matrixDest6Box.addItemList (matrixDestinations, 1);
    matrixDest1Attachment = std::make_unique<ComboAttachment> (processorRef.parameters, "matrixDest1", matrixDest1Box);
    matrixDest2Attachment = std::make_unique<ComboAttachment> (processorRef.parameters, "matrixDest2", matrixDest2Box);
    matrixDest3Attachment = std::make_unique<ComboAttachment> (processorRef.parameters, "matrixDest3", matrixDest3Box);
    matrixDest4Attachment = std::make_unique<ComboAttachment> (processorRef.parameters, "matrixDest4", matrixDest4Box);
    matrixDest5Attachment = std::make_unique<ComboAttachment> (processorRef.parameters, "matrixDest5", matrixDest5Box);
    matrixDest6Attachment = std::make_unique<ComboAttachment> (processorRef.parameters, "matrixDest6", matrixDest6Box);
    addAndMakeVisible (matrixDest1Box);
    addAndMakeVisible (matrixDest2Box);
    addAndMakeVisible (matrixDest3Box);
    addAndMakeVisible (matrixDest4Box);
    addAndMakeVisible (matrixDest5Box);
    addAndMakeVisible (matrixDest6Box);

    materialLabel.setText ("Body", juce::dontSendNotification);
    materialLabel.setJustificationType (juce::Justification::centredLeft);
    materialLabel.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (materialLabel);

    materialBox.addItemList (juce::StringArray {
        "Wood Block",
        "Gong",
        "Cymbal",
        "Glass",
        "Aerogel Cathedral",
        "Neutron Porcelain",
        "Plasma Ice",
        "Time Crystal"
    }, 1);
    materialAttachment = std::make_unique<ComboAttachment> (processorRef.parameters, "material", materialBox);
    addAndMakeVisible (materialBox);

    malletMaterialLabel.setText ("Mallet", juce::dontSendNotification);
    malletMaterialLabel.setJustificationType (juce::Justification::centredLeft);
    malletMaterialLabel.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (malletMaterialLabel);

    malletMaterialBox.addItemList (juce::StringArray { "Felt", "Rubber", "Wood", "Metal" }, 1);
    malletMaterialAttachment = std::make_unique<ComboAttachment> (processorRef.parameters, "malletMaterial", malletMaterialBox);
    addAndMakeVisible (malletMaterialBox);

    sympatheticMaterialLabel.setText ("String Kind", juce::dontSendNotification);
    sympatheticMaterialLabel.setJustificationType (juce::Justification::centredLeft);
    sympatheticMaterialLabel.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (sympatheticMaterialLabel);

    sympatheticMaterialBox.addItemList (juce::StringArray { "Gut", "Steel", "Nylon", "Aether" }, 1);
    sympatheticMaterialAttachment = std::make_unique<ComboAttachment> (processorRef.parameters, "sympatheticMaterial", sympatheticMaterialBox);
    addAndMakeVisible (sympatheticMaterialBox);

    sympatheticTuningModeLabel.setText ("Tuning Mode", juce::dontSendNotification);
    sympatheticTuningModeLabel.setJustificationType (juce::Justification::centredLeft);
    sympatheticTuningModeLabel.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (sympatheticTuningModeLabel);

    sympatheticTuningModeBox.addItemList (juce::StringArray {
        "Unison Stack",
        "Perfect Fifths",
        "Drone Cluster",
        "Raga Resonance",
        "Inharmonic Cloud"
    }, 1);
    sympatheticTuningModeAttachment = std::make_unique<ComboAttachment> (processorRef.parameters, "sympatheticTuningMode", sympatheticTuningModeBox);
    addAndMakeVisible (sympatheticTuningModeBox);

    setupSlider (malletHardness, "Mallet", "malletHardness");

    setupSlider (attack, "Attack", "attack");
    setupSlider (decay, "Decay", "decay");
    setupSlider (sustain, "Sustain", "sustain");
    setupSlider (release, "Release", "release");

    setupSlider (filterCutoff, "Filter Cutoff", "filterCutoff");
    setupSlider (filterResonance, "Filter Resonance", "filterResonance");

    setupSlider (reverbMix, "Reverb Mix", "reverbMix");
    setupSlider (reverbRoomSize, "Reverb Room Size", "reverbRoomSize");
    setupSlider (reverbDamping, "Reverb Damping", "reverbDamping");
    setupSlider (delayTimeMs, "Delay Time", "delayTimeMs");
    setupSlider (delayFeedback, "Delay Feedback", "delayFeedback");
    setupSlider (delayMix, "Delay Mix", "delayMix");
    setupSlider (sympatheticMix, "Sympathetic Mix", "sympatheticMix");
    setupSlider (sympatheticDecay, "Sympathetic Decay", "sympatheticDecay");
    setupSlider (sympatheticBrightness, "Sympathetic Brightness", "sympatheticBrightness");
    setupSlider (sympatheticCoupling, "Sympathetic Coupling", "sympatheticCoupling");
    setupSlider (sympatheticCourses, "Courses", "sympatheticCourses");
    setupSlider (sympatheticSpacing, "String Spacing", "sympatheticSpacing");
    setupSlider (sympatheticDetuneSpread, "Detune Spread (cents)", "sympatheticDetuneCents");
    setupSlider (ebowDrive, "Ebow Drive", "ebowDrive");
    setupSlider (ebowFocus, "Ebow Focus", "ebowFocus");
    setupSlider (lfoRate, "LFO 1 Rate", "lfoRateHz");
    setupSlider (lfoDepth, "LFO 1 Depth", "lfoDepth");
    setupSlider (lfo2Rate, "LFO 2 Rate", "lfo2RateHz");
    setupSlider (lfo2Depth, "LFO 2 Depth", "lfo2Depth");
    setupSlider (matrixAmount1, "Amount", "matrixAmount1");
    setupSlider (matrixAmount2, "Amount", "matrixAmount2");
    setupSlider (matrixAmount3, "Amount", "matrixAmount3");
    setupSlider (matrixAmount4, "Amount", "matrixAmount4");
    setupSlider (matrixAmount5, "Amount", "matrixAmount5");
    setupSlider (matrixAmount6, "Amount", "matrixAmount6");
    setupSlider (compThreshold, "Compressor Threshold", "compThresholdDb");
    setupSlider (compRatio, "Compressor Ratio", "compRatio");
    setupSlider (compAttack, "Compressor Attack", "compAttackMs");
    setupSlider (compRelease, "Compressor Release", "compReleaseMs");
    setupSlider (compMakeup, "Compressor Makeup", "compMakeupDb");
    setupSlider (overdriveDrive, "Overdrive Drive", "overdriveDriveDb");
    setupSlider (overdriveTone, "Overdrive Tone", "overdriveTone");
    setupSlider (overdriveMix, "Overdrive Mix", "overdriveMix");
    setupSlider (overdriveOutput, "Overdrive Output", "overdriveOutputDb");

    auto configureMatrixAmountSlider = [] (LabeledSlider& control)
    {
        control.slider.setSliderStyle (juce::Slider::LinearHorizontal);
        control.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 18);
    };

    auto configureHorizontalControlSlider = [] (LabeledSlider& control)
    {
        control.slider.setSliderStyle (juce::Slider::LinearHorizontal);
        control.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 18);
        control.label.setJustificationType (juce::Justification::centredLeft);
    };

    configureMatrixAmountSlider (matrixAmount1);
    configureMatrixAmountSlider (matrixAmount2);
    configureMatrixAmountSlider (matrixAmount3);
    configureMatrixAmountSlider (matrixAmount4);
    configureMatrixAmountSlider (matrixAmount5);
    configureMatrixAmountSlider (matrixAmount6);

    configureHorizontalControlSlider (sympatheticMix);
    configureHorizontalControlSlider (sympatheticDecay);
    configureHorizontalControlSlider (sympatheticBrightness);
    configureHorizontalControlSlider (sympatheticCoupling);
    configureHorizontalControlSlider (sympatheticCourses);
    configureHorizontalControlSlider (sympatheticSpacing);
    configureHorizontalControlSlider (sympatheticDetuneSpread);
    configureHorizontalControlSlider (ebowDrive);
    configureHorizontalControlSlider (ebowFocus);

    configureTickerLabel (filterCutoffTicker);
    configureTickerLabel (filterResonanceTicker);
    configureTickerLabel (sympatheticMixTicker);
    configureTickerLabel (sympatheticBrightnessTicker);
    configureTickerLabel (sympatheticCouplingTicker);
    configureTickerLabel (ebowDriveTicker);
    configureTickerLabel (ebowFocusTicker);
    configureTickerLabel (outputGainTicker);

    setupSlider (outputGain, "Output", "outputGainDb");

    keyboard.setAvailableRange (24, 96);
    keyboard.setVelocity (1.0f, true);
    keyboard.setKeyPressBaseOctave (currentKeyboardOctave);
    addAndMakeVisible (keyboard);

    updateOctaveLabel();
    updatePageVisibility();
    updateWaveformButtons();
    startTimerHz (30);
}

void ModelingSynthTwoAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient gradient (backgroundTop, 0.0f, 0.0f,
                                   backgroundBottom, 0.0f, (float) getHeight(), false);
    g.setGradientFill (gradient);
    g.fillAll();

    auto bounds = getLocalBounds().reduced (16);
    bounds.removeFromTop (34);
    bounds.removeFromTop (8);

    const auto synthContentHeight = juce::jmin (560, bounds.getHeight() - 150);
    const auto contentHeight = currentPage == EditorPage::synth
                                   ? synthContentHeight
                                   : bounds.getHeight() - 130;
    auto content = bounds.removeFromTop (contentHeight);

    if (currentPage == EditorPage::synth)
    {
        auto materialAndVoiceRow = content.removeFromTop (220);
        drawCard (g, materialAndVoiceRow.removeFromLeft (300).reduced (6), accentA.withAlpha (0.16f));
        drawCard (g, materialAndVoiceRow.reduced (6), accentB.withAlpha (0.15f));
        drawCard (g, content.reduced (6), juce::Colours::white.withAlpha (0.05f));
    }
    else if (currentPage == EditorPage::lfo)
    {
        auto rowTop = content.removeFromTop (220);
        auto rowMid = content.removeFromTop (320);
        auto rowBottom = content;
        drawCard (g, (rowTop.withHeight (rowTop.getHeight() + rowMid.getHeight() + rowBottom.getHeight())).reduced (6),
                  accentB.withAlpha (0.14f));
    }
    else
    {
        auto rowTop = content.removeFromTop (220);
        auto rowMid = content.removeFromTop (320);
        auto rowBottom = content;
        drawCard (g, (rowTop.withHeight (rowTop.getHeight() + rowMid.getHeight() + rowBottom.getHeight())).reduced (6),
                  juce::Colours::white.withAlpha (0.05f));
    }

    if (currentPage == EditorPage::lfo)
    {
        const auto nowSeconds = (float) (juce::Time::getMillisecondCounterHiRes() * 0.001);

        drawLfoWavePreview (g,
                            lfoWavePreview1Bounds,
                            lfoWaveformBox.getSelectedItemIndex(),
                            (float) lfoRate.slider.getValue(),
                            (float) lfoDepth.slider.getValue(),
                            nowSeconds,
                            "LFO 1");

        drawLfoWavePreview (g,
                            lfoWavePreview2Bounds,
                            lfo2WaveformBox.getSelectedItemIndex(),
                            (float) lfo2Rate.slider.getValue(),
                            (float) lfo2Depth.slider.getValue(),
                            nowSeconds,
                            "LFO 2");
    }
}

void ModelingSynthTwoAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    auto topBar = bounds.removeFromTop (34);

    presetLabel.setBounds (topBar.removeFromLeft (70));
    presetBox.setBounds (topBar.removeFromLeft (210).reduced (2, 0));

    synthTabButton.setBounds (topBar.removeFromLeft (58).reduced (2, 0));
    lfoTabButton.setBounds (topBar.removeFromLeft (48).reduced (2, 0));
    fxTabButton.setBounds (topBar.removeFromLeft (44).reduced (2, 0));

    randomizeButton.setBounds (topBar.removeFromLeft (80).reduced (2, 0));
    savePresetButton.setBounds (topBar.removeFromLeft (86).reduced (2, 0));
    loadPresetButton.setBounds (topBar.removeFromLeft (86).reduced (2, 0));
    octaveLabel.setBounds (topBar.reduced (2, 0));

    bounds.removeFromTop (8);

    const auto synthContentHeight = juce::jmin (560, bounds.getHeight() - 150);
    const auto contentHeight = currentPage == EditorPage::synth
                                   ? synthContentHeight
                                   : bounds.getHeight() - 130;
    auto content = bounds.removeFromTop (contentHeight);

    auto synthContent = content;
    auto firstRow = synthContent.removeFromTop (220);
    auto synthLowerArea = synthContent;

    auto materialArea = firstRow.removeFromLeft (300).reduced (12);
    auto envAndFilterArea = firstRow.reduced (12);

    oscillatorSection.setBounds (materialArea.removeFromTop (26));

    auto materialLine = materialArea.removeFromTop (30);
    materialLabel.setBounds (materialLine.removeFromLeft (70));
    materialBox.setBounds (materialLine.reduced (2));

    auto malletMaterialLine = materialArea.removeFromTop (30);
    malletMaterialLabel.setBounds (malletMaterialLine.removeFromLeft (70));
    malletMaterialBox.setBounds (malletMaterialLine.reduced (2));

    auto hardnessArea = materialArea.removeFromTop (120);
    malletHardness.label.setBounds (hardnessArea.removeFromTop (20));
    malletHardness.slider.setBounds (hardnessArea.reduced (12, 2));

    outputGain.label.setBounds (materialArea.removeFromTop (20));
    outputGain.slider.setBounds (materialArea.removeFromTop (56).reduced (12, 2));

    auto envelopeArea = envAndFilterArea.removeFromLeft (envAndFilterArea.getWidth() / 2).reduced (10);
    auto filterArea = envAndFilterArea.reduced (10);

    envelopeSection.setBounds (envelopeArea.removeFromTop (26));
    filterSection.setBounds (filterArea.removeFromTop (26));

    auto adsrSliderWidth = envelopeArea.getWidth() / 4;

    auto placeAdsr = [&] (LabeledSlider& control)
    {
        auto area = envelopeArea.removeFromLeft (adsrSliderWidth);
        control.label.setBounds (area.removeFromTop (18));
        control.slider.setBounds (area.reduced (4, 4));
    };

    placeAdsr (attack);
    placeAdsr (decay);
    placeAdsr (sustain);
    placeAdsr (release);

    auto filterSliderWidth = filterArea.getWidth() / 2;

    auto placeFilter = [&] (LabeledSlider& control)
    {
        auto area = filterArea.removeFromLeft (filterSliderWidth);
        control.label.setBounds (area.removeFromTop (18));
        control.slider.setBounds (area.reduced (8, 4));
    };

    placeFilter (filterCutoff);
    placeFilter (filterResonance);

    placeTickerLabel (filterCutoffTicker, filterCutoff.slider);
    placeTickerLabel (filterResonanceTicker, filterResonance.slider);

    auto layoutFxRow = [] (juce::Rectangle<int> row, std::initializer_list<LabeledSlider*> controls)
    {
        const auto sliderWidth = row.getWidth() / (int) controls.size();
        for (auto* control : controls)
        {
            auto area = row.removeFromLeft (sliderWidth);
            control->label.setBounds (area.removeFromTop (18));
            control->slider.setBounds (area.reduced (6, 4));
        }
    };

    auto synthModArea = synthLowerArea.reduced (12);
    sympatheticSection.setBounds (synthModArea.removeFromTop (24));
    auto controlsArea = synthModArea;

    auto layoutHorizontalControl = [] (juce::Rectangle<int> area, LabeledSlider& control)
    {
        area = area.reduced (8, 3);
        auto labelArea = area.removeFromLeft (152);
        control.label.setBounds (labelArea);
        control.slider.setBounds (area);
    };

    auto layoutHorizontalPair = [&] (juce::Rectangle<int> row, LabeledSlider& leftControl, LabeledSlider& rightControl)
    {
        auto left = row.removeFromLeft (row.getWidth() / 2);
        auto right = row;
        layoutHorizontalControl (left, leftControl);
        layoutHorizontalControl (right, rightControl);
    };

    auto modeRow = controlsArea.removeFromTop (34);
    auto materialCell = modeRow.removeFromLeft (modeRow.getWidth() / 2).reduced (8, 3);
    sympatheticMaterialLabel.setBounds (materialCell.removeFromLeft (96));
    sympatheticMaterialBox.setBounds (materialCell);

    auto tuningCell = modeRow.reduced (8, 3);
    sympatheticTuningModeLabel.setBounds (tuningCell.removeFromLeft (106));
    sympatheticTuningModeBox.setBounds (tuningCell);

    layoutHorizontalPair (controlsArea.removeFromTop (40), sympatheticMix, sympatheticDecay);
    layoutHorizontalPair (controlsArea.removeFromTop (40), sympatheticBrightness, sympatheticCoupling);
    layoutHorizontalPair (controlsArea.removeFromTop (40), sympatheticCourses, sympatheticSpacing);
    layoutHorizontalPair (controlsArea.removeFromTop (40), sympatheticDetuneSpread, ebowDrive);

    auto ebowRow = controlsArea.removeFromTop (40);
    layoutHorizontalControl (ebowRow.removeFromLeft (ebowRow.getWidth() / 2), ebowFocus);

    placeTickerLabel (sympatheticMixTicker, sympatheticMix.slider);
    placeTickerLabel (sympatheticBrightnessTicker, sympatheticBrightness.slider);
    placeTickerLabel (sympatheticCouplingTicker, sympatheticCoupling.slider);
    placeTickerLabel (ebowDriveTicker, ebowDrive.slider);
    placeTickerLabel (ebowFocusTicker, ebowFocus.slider);

    auto fxArea = (currentPage == EditorPage::fx ? content : synthLowerArea).reduced (12);
    fxSection.setBounds (fxArea.removeFromTop (26));
    auto overdriveRow = fxArea.removeFromTop (fxArea.getHeight() / 3);
    auto fxRowTop = fxArea.removeFromTop (fxArea.getHeight() / 2);
    auto fxRowBottom = fxArea;
    layoutFxRow (overdriveRow, { &overdriveDrive, &overdriveTone, &overdriveMix, &overdriveOutput });
    layoutFxRow (fxRowTop, { &reverbMix, &reverbRoomSize, &reverbDamping, &delayTimeMs, &delayFeedback, &delayMix });
    layoutFxRow (fxRowBottom, { &compThreshold, &compRatio, &compAttack, &compRelease, &compMakeup });

    auto matrixArea = (currentPage == EditorPage::lfo ? content : synthLowerArea).reduced (12);
    matrixSection.setBounds (matrixArea.removeFromTop (24));

    auto lfoArea = matrixArea.removeFromLeft (330);
    auto slotArea = matrixArea;

    auto lfoBlock1 = lfoArea.removeFromTop (lfoArea.getHeight() / 2).reduced (2);
    auto lfoBlock2 = lfoArea.reduced (2);

    auto layoutLfoBlock = [] (juce::Rectangle<int> block,
                              LabeledSlider& rate,
                              LabeledSlider& depth,
                              juce::TextButton& offButton)
    {
        auto offColumn = block.removeFromLeft (44);
        auto offButtonArea = offColumn.removeFromTop (40).reduced (4, 2);
        offButton.setBounds (offButtonArea);

        auto knobsArea = block.reduced (8, 2);

        auto rateArea = knobsArea.removeFromLeft (knobsArea.getWidth() / 2);
        rate.label.setBounds (rateArea.removeFromTop (18));
        rate.slider.setBounds (rateArea.reduced (6, 4));

        depth.label.setBounds (knobsArea.removeFromTop (18));
        depth.slider.setBounds (knobsArea.reduced (6, 4));
    };

    layoutLfoBlock (lfoBlock1, lfoRate, lfoDepth, lfo1WaveOffButton);
    layoutLfoBlock (lfoBlock2, lfo2Rate, lfo2Depth, lfo2WaveOffButton);

    const auto rowHeight = slotArea.getHeight() / 2;
    auto slotBand1 = slotArea.removeFromTop (rowHeight).reduced (0, 2);
    auto slotBand2 = slotArea.reduced (0, 2);
    auto waveRow1 = slotBand1.removeFromBottom (36);
    auto waveRow2 = slotBand2.removeFromBottom (36);
    auto previewRow1 = slotBand1.removeFromBottom (200).reduced (10, 6);
    auto previewRow2 = slotBand2.removeFromBottom (200).reduced (10, 6);
    auto slotRow1 = slotBand1;
    auto slotRow2 = slotBand2;

    lfoWavePreview1Bounds = previewRow1.withSizeKeepingCentre (previewRow1.getWidth() / 2, previewRow1.getHeight());
    lfoWavePreview2Bounds = previewRow2.withSizeKeepingCentre (previewRow2.getWidth() / 2, previewRow2.getHeight());
    const auto slotWidth = slotRow1.getWidth() / 3;
    std::array<juce::Label*, 6> slotLabels {
        &matrixDest1Label, &matrixDest2Label, &matrixDest3Label,
        &matrixDest4Label, &matrixDest5Label, &matrixDest6Label
    };
    std::array<juce::ComboBox*, 6> slotBoxes {
        &matrixDest1Box, &matrixDest2Box, &matrixDest3Box,
        &matrixDest4Box, &matrixDest5Box, &matrixDest6Box
    };
    std::array<LabeledSlider*, 6> slotAmounts {
        &matrixAmount1, &matrixAmount2, &matrixAmount3,
        &matrixAmount4, &matrixAmount5, &matrixAmount6
    };

    for (int i = 0; i < 6; ++i)
    {
        auto& row = i < 3 ? slotRow1 : slotRow2;
        auto cell = row.removeFromLeft (slotWidth).reduced (6, 4);
        slotLabels[(size_t) i]->setBounds (cell.removeFromTop (18));
        slotBoxes[(size_t) i]->setBounds (cell.removeFromTop (28));
        cell.removeFromTop (2);
        slotAmounts[(size_t) i]->label.setBounds (cell.removeFromTop (16));
        slotAmounts[(size_t) i]->slider.setBounds (cell.removeFromTop (24));
    }

    auto layoutWaveRow = [] (juce::Rectangle<int> row,
                             juce::DrawableButton& sine,
                             juce::DrawableButton& triangle,
                             juce::DrawableButton& sampleHold)
    {
        row = row.reduced (8, 6);
        const auto buttonWidth = juce::jmin (62, row.getWidth() / 3);
        const auto totalWidth = buttonWidth * 3;
        auto buttons = row.withSizeKeepingCentre (totalWidth, row.getHeight());

        sine.setBounds (buttons.removeFromLeft (buttonWidth).reduced (4, 2));
        triangle.setBounds (buttons.removeFromLeft (buttonWidth).reduced (4, 2));
        sampleHold.setBounds (buttons.reduced (4, 2));
    };

    layoutWaveRow (waveRow1, lfo1WaveSineButton, lfo1WaveTriangleButton, lfo1WaveSampleHoldButton);
    layoutWaveRow (waveRow2, lfo2WaveSineButton, lfo2WaveTriangleButton, lfo2WaveSampleHoldButton);

    auto keyboardArea = bounds.reduced (6, 0);
    const auto keyboardWidth = juce::jmin (780, keyboardArea.getWidth());
    keyboard.setBounds (keyboardArea.withSizeKeepingCentre (keyboardWidth, keyboardArea.getHeight()));

    placeTickerLabel (outputGainTicker, outputGain.slider);

    updatePageVisibility();
}

bool ModelingSynthTwoAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    const auto character = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());

    if (character == 'z')
    {
        changeKeyboardOctave (-1);
        return true;
    }

    if (character == 'x')
    {
        changeKeyboardOctave (1);
        return true;
    }

    return false;
}

void ModelingSynthTwoAudioProcessorEditor::changeKeyboardOctave (int delta)
{
    const auto newOctave = juce::jlimit (0, 10, currentKeyboardOctave + delta);

    if (newOctave == currentKeyboardOctave)
        return;

    currentKeyboardOctave = newOctave;
    keyboard.setKeyPressBaseOctave (currentKeyboardOctave);
    updateOctaveLabel();
}

void ModelingSynthTwoAudioProcessorEditor::updateOctaveLabel()
{
    octaveLabel.setText ("Keyboard Octave: C" + juce::String (currentKeyboardOctave) + " (Z/X)",
                         juce::dontSendNotification);
}

void ModelingSynthTwoAudioProcessorEditor::updatePageVisibility()
{
    synthTabButton.setToggleState (currentPage == EditorPage::synth, juce::dontSendNotification);
    lfoTabButton.setToggleState (currentPage == EditorPage::lfo, juce::dontSendNotification);
    fxTabButton.setToggleState (currentPage == EditorPage::fx, juce::dontSendNotification);

    auto setControlVisible = [] (LabeledSlider& control, bool shouldBeVisible)
    {
        control.label.setVisible (shouldBeVisible);
        control.slider.setVisible (shouldBeVisible);
    };

    const auto showSynth = currentPage == EditorPage::synth;
    const auto showLfo = currentPage == EditorPage::lfo;
    const auto showFx = currentPage == EditorPage::fx;

    oscillatorSection.setVisible (showSynth);
    envelopeSection.setVisible (showSynth);
    filterSection.setVisible (showSynth);
    sympatheticSection.setVisible (showSynth);
    matrixSection.setVisible (showLfo);
    fxSection.setVisible (showFx);

    materialLabel.setVisible (showSynth);
    materialBox.setVisible (showSynth);
    malletMaterialLabel.setVisible (showSynth);
    malletMaterialBox.setVisible (showSynth);
    sympatheticMaterialLabel.setVisible (showSynth);
    sympatheticMaterialBox.setVisible (showSynth);
    sympatheticTuningModeLabel.setVisible (showSynth);
    sympatheticTuningModeBox.setVisible (showSynth);
    lfoWaveformLabel.setVisible (false);
    lfoWaveformBox.setVisible (false);
    lfo2WaveformLabel.setVisible (false);
    lfo2WaveformBox.setVisible (false);
    lfo1WaveSineButton.setVisible (showLfo);
    lfo1WaveTriangleButton.setVisible (showLfo);
    lfo1WaveSampleHoldButton.setVisible (showLfo);
    lfo1WaveOffButton.setVisible (showLfo);
    lfo2WaveSineButton.setVisible (showLfo);
    lfo2WaveTriangleButton.setVisible (showLfo);
    lfo2WaveSampleHoldButton.setVisible (showLfo);
    lfo2WaveOffButton.setVisible (showLfo);

    matrixDest1Label.setVisible (showLfo);
    matrixDest1Box.setVisible (showLfo);
    matrixDest2Label.setVisible (showLfo);
    matrixDest2Box.setVisible (showLfo);
    matrixDest3Label.setVisible (showLfo);
    matrixDest3Box.setVisible (showLfo);
    matrixDest4Label.setVisible (showLfo);
    matrixDest4Box.setVisible (showLfo);
    matrixDest5Label.setVisible (showLfo);
    matrixDest5Box.setVisible (showLfo);
    matrixDest6Label.setVisible (showLfo);
    matrixDest6Box.setVisible (showLfo);

    setControlVisible (malletHardness, showSynth);
    setControlVisible (attack, showSynth);
    setControlVisible (decay, showSynth);
    setControlVisible (sustain, showSynth);
    setControlVisible (release, showSynth);
    setControlVisible (filterCutoff, showSynth);
    setControlVisible (filterResonance, showSynth);
    setControlVisible (lfoRate, showLfo);
    setControlVisible (lfoDepth, showLfo);
    setControlVisible (lfo2Rate, showLfo);
    setControlVisible (lfo2Depth, showLfo);
    setControlVisible (matrixAmount1, showLfo);
    setControlVisible (matrixAmount2, showLfo);
    setControlVisible (matrixAmount3, showLfo);
    setControlVisible (matrixAmount4, showLfo);
    setControlVisible (matrixAmount5, showLfo);
    setControlVisible (matrixAmount6, showLfo);
    setControlVisible (outputGain, showSynth);

    setControlVisible (reverbMix, showFx);
    setControlVisible (reverbRoomSize, showFx);
    setControlVisible (reverbDamping, showFx);
    setControlVisible (delayTimeMs, showFx);
    setControlVisible (delayFeedback, showFx);
    setControlVisible (delayMix, showFx);
    setControlVisible (sympatheticMix, showSynth);
    setControlVisible (sympatheticDecay, showSynth);
    setControlVisible (sympatheticBrightness, showSynth);
    setControlVisible (sympatheticCoupling, showSynth);
    setControlVisible (sympatheticCourses, showSynth);
    setControlVisible (sympatheticSpacing, showSynth);
    setControlVisible (sympatheticDetuneSpread, showSynth);
    setControlVisible (ebowDrive, showSynth);
    setControlVisible (ebowFocus, showSynth);
    setControlVisible (compThreshold, showFx);
    setControlVisible (compRatio, showFx);
    setControlVisible (compAttack, showFx);
    setControlVisible (compRelease, showFx);
    setControlVisible (compMakeup, showFx);
    setControlVisible (overdriveDrive, showFx);
    setControlVisible (overdriveTone, showFx);
    setControlVisible (overdriveMix, showFx);
    setControlVisible (overdriveOutput, showFx);

    filterCutoffTicker.setVisible (showSynth);
    filterResonanceTicker.setVisible (showSynth);
    outputGainTicker.setVisible (showSynth);
    sympatheticMixTicker.setVisible (false);
    sympatheticBrightnessTicker.setVisible (false);
    sympatheticCouplingTicker.setVisible (false);
    ebowDriveTicker.setVisible (false);
    ebowFocusTicker.setVisible (false);

    repaint();
}

void ModelingSynthTwoAudioProcessorEditor::updateWaveformButtons()
{
    auto updateWaveGroup = [] (int selectedIndex,
                               juce::Button& offButton,
                               juce::Button& sineButton,
                               juce::Button& triangleButton,
                               juce::Button& sampleHoldButton)
    {
        offButton.setToggleState (selectedIndex == 3, juce::dontSendNotification);
        sineButton.setToggleState (selectedIndex == 0, juce::dontSendNotification);
        triangleButton.setToggleState (selectedIndex == 1, juce::dontSendNotification);
        sampleHoldButton.setToggleState (selectedIndex == 2, juce::dontSendNotification);
    };

    auto applyLfoSectionState = [] (bool isEnabled,
                                    LabeledSlider& rate,
                                    LabeledSlider& depth,
                                    juce::Label& dest1Label,
                                    juce::ComboBox& dest1Box,
                                    LabeledSlider& amt1,
                                    juce::Label& dest2Label,
                                    juce::ComboBox& dest2Box,
                                    LabeledSlider& amt2,
                                    juce::Label& dest3Label,
                                    juce::ComboBox& dest3Box,
                                    LabeledSlider& amt3,
                                    juce::Button& offButton,
                                    juce::Button& sineButton,
                                    juce::Button& triangleButton,
                                    juce::Button& sampleHoldButton)
    {
        const auto activeAlpha = 1.0f;
        const auto dimmedAlpha = 0.42f;
        const auto selectorDimAlpha = 0.68f;

        auto setControlState = [isEnabled, activeAlpha, dimmedAlpha] (LabeledSlider& control)
        {
            control.label.setEnabled (isEnabled);
            control.slider.setEnabled (isEnabled);
            control.label.setAlpha (isEnabled ? activeAlpha : dimmedAlpha);
            control.slider.setAlpha (isEnabled ? activeAlpha : dimmedAlpha);
        };

        auto setDestState = [isEnabled, activeAlpha, dimmedAlpha] (juce::Label& label, juce::ComboBox& box)
        {
            label.setEnabled (isEnabled);
            box.setEnabled (isEnabled);
            label.setAlpha (isEnabled ? activeAlpha : dimmedAlpha);
            box.setAlpha (isEnabled ? activeAlpha : dimmedAlpha);
        };

        setControlState (rate);
        setControlState (depth);
        setDestState (dest1Label, dest1Box);
        setControlState (amt1);
        setDestState (dest2Label, dest2Box);
        setControlState (amt2);
        setDestState (dest3Label, dest3Box);
        setControlState (amt3);

        // Keep waveform selectors active so users can re-enable the lane.
        const auto selectorAlpha = isEnabled ? activeAlpha : selectorDimAlpha;
        offButton.setEnabled (true);
        sineButton.setEnabled (true);
        triangleButton.setEnabled (true);
        sampleHoldButton.setEnabled (true);
        offButton.setAlpha (selectorAlpha);
        sineButton.setAlpha (selectorAlpha);
        triangleButton.setAlpha (selectorAlpha);
        sampleHoldButton.setAlpha (selectorAlpha);
    };

    updateWaveGroup (lfoWaveformBox.getSelectedItemIndex(),
                     lfo1WaveOffButton, lfo1WaveSineButton, lfo1WaveTriangleButton, lfo1WaveSampleHoldButton);
    updateWaveGroup (lfo2WaveformBox.getSelectedItemIndex(),
                     lfo2WaveOffButton, lfo2WaveSineButton, lfo2WaveTriangleButton, lfo2WaveSampleHoldButton);

    const auto lfo1Enabled = lfoWaveformBox.getSelectedItemIndex() != 3;
    const auto lfo2Enabled = lfo2WaveformBox.getSelectedItemIndex() != 3;

    applyLfoSectionState (lfo1Enabled,
                          lfoRate, lfoDepth,
                          matrixDest1Label, matrixDest1Box, matrixAmount1,
                          matrixDest2Label, matrixDest2Box, matrixAmount2,
                          matrixDest3Label, matrixDest3Box, matrixAmount3,
                          lfo1WaveOffButton, lfo1WaveSineButton, lfo1WaveTriangleButton, lfo1WaveSampleHoldButton);

    applyLfoSectionState (lfo2Enabled,
                          lfo2Rate, lfo2Depth,
                          matrixDest4Label, matrixDest4Box, matrixAmount4,
                          matrixDest5Label, matrixDest5Box, matrixAmount5,
                          matrixDest6Label, matrixDest6Box, matrixAmount6,
                          lfo2WaveOffButton, lfo2WaveSineButton, lfo2WaveTriangleButton, lfo2WaveSampleHoldButton);
}

void ModelingSynthTwoAudioProcessorEditor::configureTickerLabel (juce::Label& label)
{
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, bodyTextColour.brighter (0.1f));
    label.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (0.38f));
    label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

void ModelingSynthTwoAudioProcessorEditor::placeTickerLabel (juce::Label& label, const juce::Slider& slider)
{
    auto area = slider.getBounds();
    const auto w = juce::jmin (66, area.getWidth() - 10);
    const auto h = 16;
    label.setBounds (area.getCentreX() - (w / 2), area.getCentreY() - 2, w, h);
}

void ModelingSynthTwoAudioProcessorEditor::timerCallback()
{
    const auto& monitor = processorRef.getModulationMonitor();

    filterCutoffTicker.setText (juce::String (monitor.filterCutoff.load (std::memory_order_relaxed), 0), juce::dontSendNotification);
    filterResonanceTicker.setText (juce::String (monitor.filterResonance.load (std::memory_order_relaxed), 2), juce::dontSendNotification);
    sympatheticMixTicker.setText (juce::String (monitor.sympatheticMix.load (std::memory_order_relaxed), 2), juce::dontSendNotification);
    sympatheticBrightnessTicker.setText (juce::String (monitor.sympatheticBrightness.load (std::memory_order_relaxed), 2), juce::dontSendNotification);
    sympatheticCouplingTicker.setText (juce::String (monitor.sympatheticCoupling.load (std::memory_order_relaxed), 2), juce::dontSendNotification);
    ebowDriveTicker.setText (juce::String (monitor.ebowDrive.load (std::memory_order_relaxed), 2), juce::dontSendNotification);
    ebowFocusTicker.setText (juce::String (monitor.ebowFocus.load (std::memory_order_relaxed), 2), juce::dontSendNotification);
    outputGainTicker.setText (juce::String (monitor.outputGainDb.load (std::memory_order_relaxed), 1) + " dB", juce::dontSendNotification);

    if (currentPage == EditorPage::lfo)
    {
        if (! lfoWavePreview1Bounds.isEmpty())
            repaint (lfoWavePreview1Bounds.expanded (4));
        if (! lfoWavePreview2Bounds.isEmpty())
            repaint (lfoWavePreview2Bounds.expanded (4));
    }
}

void ModelingSynthTwoAudioProcessorEditor::drawLfoWavePreview (juce::Graphics& g,
                                                               juce::Rectangle<int> area,
                                                               int waveform,
                                                               float rateHz,
                                                               float depth,
                                                               float timeSeconds,
                                                               const juce::String& label)
{
    if (area.isEmpty())
        return;

    auto panel = area.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (panel, 8.0f, 1.0f);

    auto bounds = area.reduced (8);
    auto title = bounds.removeFromTop (14);
    auto plot = bounds.reduced (2, 1);
    const auto midY = (float) plot.getCentreY();
    const auto leftX = (float) plot.getX();
    const auto rightX = (float) plot.getRight();
    const auto width = juce::jmax (1.0f, rightX - leftX);
    const auto isOff = waveform == 3;

    const auto strokeAlpha = isOff ? 0.42f : 0.88f;
    const auto gridAlpha = isOff ? 0.05f : 0.08f;
    const auto titleAlpha = isOff ? 0.52f : 0.82f;

    g.setColour (bodyTextColour.withAlpha (titleAlpha));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (label, title, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (gridAlpha));
    g.drawRect (plot);
    g.drawLine (leftX, midY, rightX, midY, 1.0f);

    juce::Path wavePath;
    constexpr int points = 140;

    for (int i = 0; i < points; ++i)
    {
        const auto xNorm = (float) i / (float) (points - 1);
        const auto x = leftX + xNorm * width;

        const auto phase = xNorm + (isOff ? 0.0f : timeSeconds * juce::jmax (0.05f, rateHz));
        const auto wrapped = phase - std::floor (phase);

        float value = 0.0f;
        switch (waveform)
        {
            case 3: // Off
                value = 0.0f;
                break;
            case 1: // Triangle
                value = 1.0f - 4.0f * std::abs (wrapped - 0.5f);
                break;
            case 2: // Sample and hold
            {
                const auto step = (int) std::floor (phase * 8.0f);
                value = std::sin ((float) step * 1.7f + 0.9f);
                break;
            }
            case 0:
            default:
                value = std::sin (juce::MathConstants<float>::twoPi * wrapped);
                break;
        }

        value *= juce::jlimit (0.0f, 1.0f, depth);
        const auto y = midY - value * ((float) plot.getHeight() * 0.42f);

        if (i == 0)
            wavePath.startNewSubPath (x, y);
        else
            wavePath.lineTo (x, y);
    }

    g.setColour (accentB.withAlpha (strokeAlpha));
    g.strokePath (wavePath, juce::PathStrokeType (1.5f));

    if (! isOff)
    {
        const auto playhead = leftX + std::fmod (timeSeconds * juce::jmax (0.05f, rateHz), 1.0f) * width;
        g.setColour (accentA.withAlpha (0.65f));
        g.drawLine (playhead, (float) plot.getY() + 1.0f, playhead, (float) plot.getBottom() - 1.0f, 1.1f);
    }

    if (isOff)
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRoundedRectangle (panel, 8.0f);
        g.setColour (bodyTextColour.withAlpha (0.58f));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("LFO OFF", panel.toNearestInt(), juce::Justification::centred, false);
    }
}

void ModelingSynthTwoAudioProcessorEditor::randomizePreset()
{
    processorRef.randomizeParameters();
}

void ModelingSynthTwoAudioProcessorEditor::savePresetToDisk()
{
    const auto initialDirectory = processorRef.getLastPresetDirectory();
    const auto initialFile = initialDirectory
                                 .getChildFile ("StringtimeBellworks_Preset.ms2preset");

    activeFileChooser = std::make_unique<juce::FileChooser> ("Save preset", initialFile, "*.ms2preset");

    constexpr auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;

    activeFileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        auto presetFile = chooser.getResult();
        activeFileChooser.reset();

        if (presetFile == juce::File{})
            return;

        if (presetFile.getFileExtension() != ".ms2preset")
            presetFile = presetFile.withFileExtension (".ms2preset");

        processorRef.setLastPresetDirectory (presetFile.getParentDirectory());

        if (! processorRef.savePresetToFile (presetFile))
        {
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                    "Preset Save Failed",
                                                    "Could not write preset file:\n" + presetFile.getFullPathName());
        }
    });
}

void ModelingSynthTwoAudioProcessorEditor::loadPresetFromDisk()
{
    const auto initialDirectory = processorRef.getLastPresetDirectory();
    activeFileChooser = std::make_unique<juce::FileChooser> ("Load preset", initialDirectory, "*.ms2preset");

    constexpr auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

    activeFileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto presetFile = chooser.getResult();
        activeFileChooser.reset();

        if (presetFile == juce::File{})
            return;

        processorRef.setLastPresetDirectory (presetFile.getParentDirectory());

        if (! processorRef.loadPresetFromFile (presetFile))
        {
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                    "Preset Load Failed",
                                                    "Could not read preset file:\n" + presetFile.getFullPathName());
        }
    });
}

void ModelingSynthTwoAudioProcessorEditor::setupSlider (LabeledSlider& control, const juce::String& labelText, const juce::String& parameterId)
{
    control.label.setText (labelText, juce::dontSendNotification);
    control.label.setJustificationType (juce::Justification::centred);
    control.label.setFont (juce::FontOptions (12.0f));
    control.label.setColour (juce::Label::textColourId, bodyTextColour);
    addAndMakeVisible (control.label);

    control.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    control.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 18);
    control.slider.setColour (juce::Slider::thumbColourId, accentA);
    control.slider.setColour (juce::Slider::rotarySliderFillColourId, accentB);
    control.slider.setColour (juce::Slider::textBoxTextColourId, bodyTextColour);
    control.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    control.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha (0.22f));
    control.attachment = std::make_unique<SliderAttachment> (processorRef.parameters, parameterId, control.slider);
    addAndMakeVisible (control.slider);
}

void ModelingSynthTwoAudioProcessorEditor::setupSectionLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (16.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, sectionTitleColour);
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void ModelingSynthTwoAudioProcessorEditor::drawCard (juce::Graphics& g, juce::Rectangle<int> area, juce::Colour colour)
{
    g.setColour (cardBase);
    g.fillRoundedRectangle (area.toFloat(), 14.0f);

    g.setColour (colour);
    g.fillRoundedRectangle (area.toFloat(), 14.0f);

    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (area.toFloat(), 14.0f, 1.0f);
}
