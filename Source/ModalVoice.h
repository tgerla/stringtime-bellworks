#pragma once

#include <JuceHeader.h>
#include <atomic>

struct ModulationMonitor
{
    std::atomic<float> filterCutoff { 1800.0f };
    std::atomic<float> filterResonance { 0.85f };
    std::atomic<float> sympatheticMix { 0.25f };
    std::atomic<float> sympatheticBrightness { 0.45f };
    std::atomic<float> sympatheticCoupling { 0.40f };
    std::atomic<float> ebowDrive { 0.20f };
    std::atomic<float> ebowFocus { 0.56f };
    std::atomic<float> outputGainDb { -9.0f };
};

class ModalSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class ModalVoice : public juce::SynthesiserVoice
{
public:
    enum class MalletType
    {
        felt = 0,
        rubber,
        wood,
        metal
    };

    enum class MaterialType
    {
        wood = 0,
        gong,
        cymbal,
        glass,
        aerogelCathedral,
        neutronPorcelain,
        plasmaIce,
        timeCrystal
    };

    explicit ModalVoice (juce::AudioProcessorValueTreeState& stateToUse, ModulationMonitor& monitorToUse);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void setCurrentPlaybackSampleRate (double newRate) override;

private:
    static constexpr int maxModes = 10;
    static constexpr int maxSympatheticModes = 8;
    static constexpr int matrixSlotCount = 3;
    static constexpr int lfoCount = 2;

    void updateAdsrFromParameters();
    void updateFilterFromParameters();
    void updateSympatheticParameters();
    void updateLfoMatrixParameters();
    void configureMaterialModel (MaterialType material,
                                 MalletType mallet,
                                 float baseFrequency,
                                 float velocity,
                                 float hardness);
    void configureSympatheticResonator (float baseFrequency, float velocity);
    float renderOneSample (float sympatheticMixValue,
                           float sympatheticBrightnessValue,
                           float ebowDriveValue,
                           float ebowFocusValue,
                           float sympatheticCouplingValue,
                           float sympatheticSpacingValue,
                           float sympatheticDetuneCentsValue);
    float getNextLfoValue (float& lfoPhaseRef, float rateHz, int waveform, float& sampleAndHold);

    juce::AudioProcessorValueTreeState& state;
    ModulationMonitor& modulationMonitor;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParameters;

    juce::dsp::StateVariableTPTFilter<float> bpFilter;

    juce::Random random;

    std::array<float, maxModes> phase {};
    std::array<float, maxModes> phaseIncrement {};
    std::array<float, maxModes> modeAmplitude {};
    std::array<float, maxModes> modeDecay {};
    std::array<float, maxModes> modeCoupling {};

    std::array<float, maxSympatheticModes> sympatheticPhase {};
    std::array<float, maxSympatheticModes> sympatheticPhaseIncrement {};
    std::array<float, maxSympatheticModes> sympatheticAmplitude {};
    std::array<float, maxSympatheticModes> sympatheticDecay {};
    std::array<float, maxSympatheticModes> sympatheticCoupling {};
    std::array<float, maxSympatheticModes> sympatheticDetuneSignature {};

    int activeModes = 0;
    int activeSympatheticModes = 0;
    int excitationSamplesRemaining = 0;
    float excitationState = 0.0f;
    float excitationDecay = 0.997f;
    float baseFrequencyHz = 220.0f;
    float velocityGain = 0.0f;

    float sympatheticMix = 0.0f;
    float sympatheticCouplingAmount = 0.35f;
    float sympatheticBrightness = 0.5f;
    float sympatheticDecayAmount = 0.55f;
    int sympatheticMaterial = 0;
    int sympatheticCourses = 3;
    float sympatheticSpacing = 0.35f;
    int sympatheticTuningMode = 0;
    float sympatheticDetuneCents = 7.0f;
    float ebowDrive = 0.0f;
    float ebowFocus = 0.5f;
    float outputGainDb = -9.0f;
    float filterCutoff = 1800.0f;
    float filterResonance = 0.85f;

    std::array<float, lfoCount> lfoRateHz { 0.35f, 0.60f };
    std::array<float, lfoCount> lfoDepth { 0.0f, 0.0f };
    std::array<int, lfoCount> lfoWaveform { 0, 0 };
    std::array<std::array<int, matrixSlotCount>, lfoCount> matrixDestination {};
    std::array<std::array<float, matrixSlotCount>, lfoCount> matrixAmount {};
    std::array<float, lfoCount> lfoPhase { 0.0f, 0.0f };
    std::array<float, lfoCount> lfoSampleAndHold { 0.0f, 0.0f };

    float sympatheticLowpassState = 0.0f;
    float ebowCarrierPhase = 0.0f;
    float ebowFlutterPhase = 0.0f;
    float ebowCarrierIncrement = 0.0f;
    float ebowFlutterIncrement = 0.0f;

    double sampleRate = 44100.0;
};
