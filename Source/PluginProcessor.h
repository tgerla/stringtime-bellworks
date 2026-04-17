#pragma once

#include <JuceHeader.h>
#include "ModalVoice.h"

class ModelingSynthTwoAudioProcessor : public juce::AudioProcessor
{
public:
    ModelingSynthTwoAudioProcessor();
    ~ModelingSynthTwoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    const juce::StringArray& getPresetNames() const;
    void applyPreset (int presetIndex);
    void randomizeParameters();
    bool savePresetToFile (const juce::File& file);
    bool loadPresetFromFile (const juce::File& file);
    juce::File getLastPresetDirectory() const;
    void setLastPresetDirectory (const juce::File& directory);
    const ModulationMonitor& getModulationMonitor() const;

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;

private:
    static constexpr const char* lastPresetDirectoryKey = "lastPresetDirectory";

    struct PresetDefinition
    {
        juce::String name;
        int material = 0;
        float malletHardness = 0.55f;
        float attack = 0.01f;
        float decay = 0.7f;
        float sustain = 0.2f;
        float release = 1.2f;
        float filterCutoff = 1800.0f;
        float filterResonance = 0.85f;
        float reverbMix = 0.2f;
        float reverbRoomSize = 0.5f;
        float reverbDamping = 0.4f;
        float delayTimeMs = 260.0f;
        float delayFeedback = 0.35f;
        float delayMix = 0.18f;
        float sympatheticMix = 0.25f;
        float sympatheticDecay = 0.55f;
        float sympatheticBrightness = 0.45f;
        float sympatheticCoupling = 0.40f;
        float ebowDrive = 0.20f;
        float ebowFocus = 0.56f;
        float outputGainDb = -9.0f;
        float lfoRateHz = 0.35f;
        float lfoDepth = 0.0f;
        bool lfoEnabled = true;
        int lfoWaveform = 0;
        float lfo2RateHz = 0.60f;
        float lfo2Depth = 0.0f;
        bool lfo2Enabled = true;
        int lfo2Waveform = 0;
        int matrixDest1 = 0;
        float matrixAmount1 = 0.0f;
        int matrixDest2 = 0;
        float matrixAmount2 = 0.0f;
        int matrixDest3 = 0;
        float matrixAmount3 = 0.0f;
        int matrixDest4 = 0;
        float matrixAmount4 = 0.0f;
        int matrixDest5 = 0;
        float matrixAmount5 = 0.0f;
        int matrixDest6 = 0;
        float matrixAmount6 = 0.0f;
        int malletMaterial = 0;
        int sympatheticMaterial = 0;
        float sympatheticCourses = 3.0f;
        float sympatheticSpacing = 0.35f;
        int sympatheticTuningMode = 0;
        float sympatheticDetuneCents = 7.0f;
        float overdriveDriveDb = 8.0f;
        float overdriveTone = 0.55f;
        float overdriveMix = 0.0f;
        float overdriveOutputDb = 0.0f;
    };

    juce::Synthesiser synth;
    ModulationMonitor modulationMonitor;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 };
    juce::dsp::Reverb reverb;
    juce::dsp::Compressor<float> compressor;

    std::vector<float> overdrivePreEmphasisState;
    std::vector<float> overdriveToneState;

    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int delayWritePosition = 0;

    juce::ApplicationProperties appProperties;
    juce::File lastPresetDirectory;

    std::vector<PresetDefinition> presetBank;
    juce::StringArray presetNames;

    void prepareFx (double sampleRate, int samplesPerBlock, int numChannels);
    void processOverdrive (juce::AudioBuffer<float>& buffer);
    void processDelay (juce::AudioBuffer<float>& buffer);
    void processReverb (juce::AudioBuffer<float>& buffer);
    void processCompressor (juce::AudioBuffer<float>& buffer);
    void initialiseUserPreferences();
    void loadLastPresetDirectoryFromPreferences();
    void storeLastPresetDirectoryInPreferences();
    void initialisePresetBank();
    void setParameterById (const juce::String& parameterId, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModelingSynthTwoAudioProcessor)
};
