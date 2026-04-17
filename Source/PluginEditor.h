#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ModelingSynthTwoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit ModelingSynthTwoAudioProcessorEditor (ModelingSynthTwoAudioProcessor&);
    ~ModelingSynthTwoAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct LabeledSlider
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupSlider (LabeledSlider& control, const juce::String& labelText, const juce::String& parameterId);
    void setupSectionLabel (juce::Label& label, const juce::String& text);
    void drawCard (juce::Graphics& g, juce::Rectangle<int> area, juce::Colour colour);
    void drawLfoWavePreview (juce::Graphics& g,
                             juce::Rectangle<int> area,
                             int waveform,
                             float rateHz,
                             float depth,
                             float timeSeconds,
                             const juce::String& label);
    void changeKeyboardOctave (int delta);
    void updateOctaveLabel();
    void updatePageVisibility();
    void updateWaveformButtons();
    void configureTickerLabel (juce::Label& label);
    void placeTickerLabel (juce::Label& label, const juce::Slider& slider);
    void randomizePreset();
    void savePresetToDisk();
    void loadPresetFromDisk();
    void timerCallback() override;

    ModelingSynthTwoAudioProcessor& processorRef;

    juce::MidiKeyboardComponent keyboard;

    juce::Label oscillatorSection;
    juce::Label envelopeSection;
    juce::Label filterSection;
    juce::Label sympatheticSection;
    juce::Label fxSection;
    juce::Label matrixSection;

    juce::Label materialLabel;
    juce::ComboBox materialBox;
    std::unique_ptr<ComboAttachment> materialAttachment;
    juce::Label malletMaterialLabel;
    juce::ComboBox malletMaterialBox;
    std::unique_ptr<ComboAttachment> malletMaterialAttachment;
    juce::Label sympatheticMaterialLabel;
    juce::ComboBox sympatheticMaterialBox;
    std::unique_ptr<ComboAttachment> sympatheticMaterialAttachment;
    juce::Label sympatheticTuningModeLabel;
    juce::ComboBox sympatheticTuningModeBox;
    std::unique_ptr<ComboAttachment> sympatheticTuningModeAttachment;

    juce::Label presetLabel;
    juce::ComboBox presetBox;
    juce::TextButton synthTabButton;
    juce::TextButton lfoTabButton;
    juce::TextButton fxTabButton;
    juce::TextButton randomizeButton;
    juce::TextButton savePresetButton;
    juce::TextButton loadPresetButton;
    std::unique_ptr<juce::FileChooser> activeFileChooser;
    juce::Label octaveLabel;
    juce::Label lfoWaveformLabel;
    juce::ComboBox lfoWaveformBox;
    std::unique_ptr<ComboAttachment> lfoWaveformAttachment;
    juce::Label lfo2WaveformLabel;
    juce::ComboBox lfo2WaveformBox;
    std::unique_ptr<ComboAttachment> lfo2WaveformAttachment;
    juce::DrawableButton lfo1WaveSineButton { "LFO1Sine", juce::DrawableButton::ImageOnButtonBackground };
    juce::DrawableButton lfo1WaveTriangleButton { "LFO1Triangle", juce::DrawableButton::ImageOnButtonBackground };
    juce::DrawableButton lfo1WaveSampleHoldButton { "LFO1SampleHold", juce::DrawableButton::ImageOnButtonBackground };
    juce::TextButton lfo1WaveOffButton;
    juce::DrawableButton lfo2WaveSineButton { "LFO2Sine", juce::DrawableButton::ImageOnButtonBackground };
    juce::DrawableButton lfo2WaveTriangleButton { "LFO2Triangle", juce::DrawableButton::ImageOnButtonBackground };
    juce::DrawableButton lfo2WaveSampleHoldButton { "LFO2SampleHold", juce::DrawableButton::ImageOnButtonBackground };
    juce::TextButton lfo2WaveOffButton;

    juce::Label matrixDest1Label;
    juce::ComboBox matrixDest1Box;
    std::unique_ptr<ComboAttachment> matrixDest1Attachment;

    juce::Label matrixDest2Label;
    juce::ComboBox matrixDest2Box;
    std::unique_ptr<ComboAttachment> matrixDest2Attachment;

    juce::Label matrixDest3Label;
    juce::ComboBox matrixDest3Box;
    std::unique_ptr<ComboAttachment> matrixDest3Attachment;

    juce::Label matrixDest4Label;
    juce::ComboBox matrixDest4Box;
    std::unique_ptr<ComboAttachment> matrixDest4Attachment;

    juce::Label matrixDest5Label;
    juce::ComboBox matrixDest5Box;
    std::unique_ptr<ComboAttachment> matrixDest5Attachment;

    juce::Label matrixDest6Label;
    juce::ComboBox matrixDest6Box;
    std::unique_ptr<ComboAttachment> matrixDest6Attachment;

    int currentKeyboardOctave = 4;

    LabeledSlider malletHardness;

    LabeledSlider attack;
    LabeledSlider decay;
    LabeledSlider sustain;
    LabeledSlider release;

    LabeledSlider filterCutoff;
    LabeledSlider filterResonance;

    LabeledSlider reverbMix;
    LabeledSlider reverbRoomSize;
    LabeledSlider reverbDamping;
    LabeledSlider delayTimeMs;
    LabeledSlider delayFeedback;
    LabeledSlider delayMix;
    LabeledSlider sympatheticMix;
    LabeledSlider sympatheticDecay;
    LabeledSlider sympatheticBrightness;
    LabeledSlider sympatheticCoupling;
    LabeledSlider sympatheticCourses;
    LabeledSlider sympatheticSpacing;
    LabeledSlider sympatheticDetuneSpread;
    LabeledSlider ebowDrive;
    LabeledSlider ebowFocus;
    LabeledSlider lfoRate;
    LabeledSlider lfoDepth;
    LabeledSlider lfo2Rate;
    LabeledSlider lfo2Depth;
    LabeledSlider matrixAmount1;
    LabeledSlider matrixAmount2;
    LabeledSlider matrixAmount3;
    LabeledSlider matrixAmount4;
    LabeledSlider matrixAmount5;
    LabeledSlider matrixAmount6;
    LabeledSlider compThreshold;
    LabeledSlider compRatio;
    LabeledSlider compAttack;
    LabeledSlider compRelease;
    LabeledSlider compMakeup;
    LabeledSlider overdriveDrive;
    LabeledSlider overdriveTone;
    LabeledSlider overdriveMix;
    LabeledSlider overdriveOutput;

    LabeledSlider outputGain;

    juce::Label filterCutoffTicker;
    juce::Label filterResonanceTicker;
    juce::Label sympatheticMixTicker;
    juce::Label sympatheticBrightnessTicker;
    juce::Label sympatheticCouplingTicker;
    juce::Label ebowDriveTicker;
    juce::Label ebowFocusTicker;
    juce::Label outputGainTicker;

    juce::Rectangle<int> lfoWavePreview1Bounds;
    juce::Rectangle<int> lfoWavePreview2Bounds;

    enum class EditorPage
    {
        synth,
        lfo,
        fx
    };

    EditorPage currentPage = EditorPage::synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModelingSynthTwoAudioProcessorEditor)
};
