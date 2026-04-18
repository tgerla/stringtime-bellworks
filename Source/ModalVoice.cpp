#include "ModalVoice.h"

namespace
{
float dbToLinear (float db)
{
    return juce::Decibels::decibelsToGain (db);
}

ModalVoice::MaterialType materialFromChoice (int value)
{
    switch (value)
    {
        case 1: return ModalVoice::MaterialType::gong;
        case 2: return ModalVoice::MaterialType::cymbal;
        case 3: return ModalVoice::MaterialType::glass;
        case 4: return ModalVoice::MaterialType::aerogelCathedral;
        case 5: return ModalVoice::MaterialType::neutronPorcelain;
        case 6: return ModalVoice::MaterialType::plasmaIce;
        case 7: return ModalVoice::MaterialType::timeCrystal;
        default: return ModalVoice::MaterialType::wood;
    }
}

ModalVoice::MalletType malletFromChoice (int value)
{
    switch (value)
    {
        case 1: return ModalVoice::MalletType::rubber;
        case 2: return ModalVoice::MalletType::wood;
        case 3: return ModalVoice::MalletType::metal;
        default: return ModalVoice::MalletType::felt;
    }
}
}

ModalVoice::ModalVoice (juce::AudioProcessorValueTreeState& stateToUse, ModulationMonitor& monitorToUse)
    : state (stateToUse),
      modulationMonitor (monitorToUse)
{
    bpFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    bpFilter.prepare (spec);

    adsr.setSampleRate (sampleRate);
}

bool ModalVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<ModalSound*> (sound) != nullptr;
}

void ModalVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    baseFrequencyHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    velocityGain = juce::jlimit (0.0f, 1.0f, velocity);

    const auto hardness = state.getRawParameterValue ("malletHardness")->load();
    const auto mallet = malletFromChoice ((int) state.getRawParameterValue ("malletMaterial")->load());
    const auto material = materialFromChoice ((int) state.getRawParameterValue ("material")->load());

    updateSympatheticParameters();

    configureMaterialModel (material, mallet, baseFrequencyHz, velocityGain, hardness);
    configureSympatheticResonator (baseFrequencyHz, velocityGain);
    lfoSampleAndHold[0] = random.nextFloat() * 2.0f - 1.0f;
    lfoSampleAndHold[1] = random.nextFloat() * 2.0f - 1.0f;

    float impactTimeScale = 1.0f;
    float impactDecayOffset = 0.0f;
    float excitationLowpassCutoffHz = 1800.0f;
    float excitationHighpassCutoffHz = 600.0f;

    excitationLowpassMix = 0.70f;
    excitationHighpassMix = 0.35f;
    excitationClickAmount = 0.04f;

    switch (mallet)
    {
        case MalletType::felt:
            impactTimeScale = 1.7f;
            impactDecayOffset = 0.012f;
            excitationLowpassCutoffHz = 900.0f;
            excitationHighpassCutoffHz = 220.0f;
            excitationLowpassMix = 1.00f;
            excitationHighpassMix = 0.08f;
            excitationClickAmount = 0.0f;
            break;
        case MalletType::rubber:
            impactTimeScale = 1.2f;
            impactDecayOffset = 0.006f;
            excitationLowpassCutoffHz = 1700.0f;
            excitationHighpassCutoffHz = 550.0f;
            excitationLowpassMix = 0.82f;
            excitationHighpassMix = 0.28f;
            excitationClickAmount = 0.03f;
            break;
        case MalletType::wood:
            impactTimeScale = 0.92f;
            impactDecayOffset = 0.001f;
            excitationLowpassCutoffHz = 3000.0f;
            excitationHighpassCutoffHz = 1200.0f;
            excitationLowpassMix = 0.58f;
            excitationHighpassMix = 0.56f;
            excitationClickAmount = 0.08f;
            break;
        case MalletType::metal:
            impactTimeScale = 0.66f;
            impactDecayOffset = -0.005f;
            excitationLowpassCutoffHz = 7600.0f;
            excitationHighpassCutoffHz = 3200.0f;
            excitationLowpassMix = 0.28f;
            excitationHighpassMix = 0.95f;
            excitationClickAmount = 0.18f;
            break;
        default:
            break;
    }

    const auto dt = 1.0f / (float) juce::jmax (1.0, sampleRate);
    const auto lowOmega = juce::MathConstants<float>::twoPi * juce::jmax (20.0f, excitationLowpassCutoffHz);
    excitationLowpassAlpha = 1.0f - std::exp (-lowOmega * dt);

    const auto hpCutoff = juce::jmax (20.0f, excitationHighpassCutoffHz);
    const auto rc = 1.0f / (juce::MathConstants<float>::twoPi * hpCutoff);
    excitationHighpassAlpha = rc / (rc + dt);

    excitationLowpassState = 0.0f;
    excitationHighpassState = 0.0f;
    excitationPrevInput = 0.0f;

    excitationState = 1.0f;
    excitationSamplesRemaining = juce::jmax (8,
                                             (int) (sampleRate * juce::jmap (hardness, 0.010f, 0.003f) * impactTimeScale));
    excitationTotalSamples = excitationSamplesRemaining;
    excitationDecay = juce::jlimit (0.94f, 0.9995f, juce::jmap (hardness, 0.992f, 0.965f) + impactDecayOffset);

    updateAdsrFromParameters();
    adsr.noteOn();
}

void ModalVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void ModalVoice::pitchWheelMoved (int)
{
}

void ModalVoice::controllerMoved (int, int)
{
}

void ModalVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    updateAdsrFromParameters();
    updateFilterFromParameters();
    updateSympatheticParameters();
    updateLfoMatrixParameters();

    auto* left = outputBuffer.getWritePointer (0);
    auto* right = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto env = adsr.getNextSample();

        if (! adsr.isActive() && env <= 0.00001f)
        {
            clearCurrentNote();
            break;
        }

        const auto lfo1 = getNextLfoValue (lfoPhase[0], lfoRateHz[0], lfoWaveform[0], lfoSampleAndHold[0])
                  * (lfoEnabled[0] ? lfoDepth[0] : 0.0f);
        const auto lfo2 = getNextLfoValue (lfoPhase[1], lfoRateHz[1], lfoWaveform[1], lfoSampleAndHold[1])
                  * (lfoEnabled[1] ? lfoDepth[1] : 0.0f);

        auto modCutoff = filterCutoff;
        auto modResonance = filterResonance;
        auto modSymMix = sympatheticMix;
        auto modSymBrightness = sympatheticBrightness;
        auto modSymCoupling = sympatheticCouplingAmount;
        auto modSymSpacing = sympatheticSpacing;
        auto modSymDetune = sympatheticDetuneCents;
        auto modEbowDrive = ebowDrive;
        auto modEbowFocus = ebowFocus;
        auto modOutputGainDb = outputGainDb;

        auto applyMatrix = [&] (int lfoIndex, float lfoValue)
        {
            for (int slot = 0; slot < matrixSlotCount; ++slot)
            {
                const auto amount = matrixAmount[(size_t) lfoIndex][(size_t) slot];
                const auto modulation = lfoValue * amount;

                switch (matrixDestination[(size_t) lfoIndex][(size_t) slot])
                {
                    case 1: // Filter cutoff
                        modCutoff *= std::pow (2.0f, modulation * 2.0f);
                        break;
                    case 2: // Filter resonance
                        modResonance += modulation * 2.0f;
                        break;
                    case 3: // Sympathetic mix
                        modSymMix += modulation * 0.6f;
                        break;
                    case 4: // Sympathetic tone
                        modSymBrightness += modulation * 0.6f;
                        break;
                    case 5: // Sympathetic link
                        modSymCoupling += modulation * 0.7f;
                        break;
                    case 6: // Ebow drive
                        modEbowDrive += modulation * 0.8f;
                        break;
                    case 7: // Output gain
                        modOutputGainDb += modulation * 9.0f;
                        break;
                    case 8: // Ebow focus
                        modEbowFocus += modulation * 0.8f;
                        break;
                    case 9: // Sympathetic spacing
                        modSymSpacing += modulation * 0.75f;
                        break;
                    case 10: // Sympathetic detune
                        modSymDetune += modulation * 20.0f;
                        break;
                    case 0:
                    default:
                        break;
                }
            }
        };

        applyMatrix (0, lfo1);
        applyMatrix (1, lfo2);

        modCutoff = juce::jlimit (40.0f, 18000.0f, modCutoff);
        modResonance = juce::jlimit (0.2f, 10.0f, modResonance);
        modSymMix = juce::jlimit (0.0f, 1.0f, modSymMix);
        modSymBrightness = juce::jlimit (0.0f, 1.0f, modSymBrightness);
        modSymCoupling = juce::jlimit (0.0f, 1.0f, modSymCoupling);
        modSymSpacing = juce::jlimit (0.0f, 1.0f, modSymSpacing);
        modSymDetune = juce::jlimit (0.0f, 48.0f, modSymDetune);
        modEbowDrive = juce::jlimit (0.0f, 1.0f, modEbowDrive);
        modEbowFocus = juce::jlimit (0.0f, 1.0f, modEbowFocus);
        modOutputGainDb = juce::jlimit (-24.0f, 6.0f, modOutputGainDb);

        modulationMonitor.filterCutoff.store (modCutoff, std::memory_order_relaxed);
        modulationMonitor.filterResonance.store (modResonance, std::memory_order_relaxed);
        modulationMonitor.sympatheticMix.store (modSymMix, std::memory_order_relaxed);
        modulationMonitor.sympatheticBrightness.store (modSymBrightness, std::memory_order_relaxed);
        modulationMonitor.sympatheticCoupling.store (modSymCoupling, std::memory_order_relaxed);
        modulationMonitor.ebowDrive.store (modEbowDrive, std::memory_order_relaxed);
        modulationMonitor.ebowFocus.store (modEbowFocus, std::memory_order_relaxed);
        modulationMonitor.outputGainDb.store (modOutputGainDb, std::memory_order_relaxed);

        bpFilter.setCutoffFrequency (modCutoff);
        bpFilter.setResonance (modResonance);

        auto sample = renderOneSample (modSymMix,
                           modSymBrightness,
                           modEbowDrive,
                           modEbowFocus,
                           modSymCoupling,
                           modSymSpacing,
                           modSymDetune) * env;
        sample *= dbToLinear (modOutputGainDb);
        sample = bpFilter.processSample (0, sample);

        left[startSample + i] += sample;
        if (right != nullptr)
            right[startSample + i] += sample;
    }
}

void ModalVoice::setCurrentPlaybackSampleRate (double newRate)
{
    sampleRate = newRate;
    adsr.setSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    bpFilter.prepare (spec);
}

void ModalVoice::updateAdsrFromParameters()
{
    adsrParameters.attack = state.getRawParameterValue ("attack")->load();
    adsrParameters.decay = state.getRawParameterValue ("decay")->load();
    adsrParameters.sustain = state.getRawParameterValue ("sustain")->load();
    adsrParameters.release = state.getRawParameterValue ("release")->load();
    adsr.setParameters (adsrParameters);
}

void ModalVoice::updateFilterFromParameters()
{
    filterCutoff = state.getRawParameterValue ("filterCutoff")->load();
    filterResonance = state.getRawParameterValue ("filterResonance")->load();
}

void ModalVoice::updateSympatheticParameters()
{
    sympatheticMix = state.getRawParameterValue ("sympatheticMix")->load();
    sympatheticCouplingAmount = state.getRawParameterValue ("sympatheticCoupling")->load();
    sympatheticBrightness = state.getRawParameterValue ("sympatheticBrightness")->load();
    sympatheticDecayAmount = state.getRawParameterValue ("sympatheticDecay")->load();
    sympatheticMaterial = (int) state.getRawParameterValue ("sympatheticMaterial")->load();
    sympatheticCourses = juce::jlimit (1, 6, (int) std::round (state.getRawParameterValue ("sympatheticCourses")->load()));
    sympatheticSpacing = state.getRawParameterValue ("sympatheticSpacing")->load();
    sympatheticTuningMode = (int) state.getRawParameterValue ("sympatheticTuningMode")->load();
    sympatheticDetuneCents = state.getRawParameterValue ("sympatheticDetuneCents")->load();
    ebowDrive = state.getRawParameterValue ("ebowDrive")->load();
    ebowFocus = state.getRawParameterValue ("ebowFocus")->load();
    outputGainDb = state.getRawParameterValue ("outputGainDb")->load();
}

void ModalVoice::updateLfoMatrixParameters()
{
    lfoRateHz[0] = state.getRawParameterValue ("lfoRateHz")->load();
    lfoDepth[0] = state.getRawParameterValue ("lfoDepth")->load();
    lfoEnabled[0] = state.getRawParameterValue ("lfoEnabled")->load() >= 0.5f;
    lfoWaveform[0] = (int) state.getRawParameterValue ("lfoWaveform")->load();
    lfoRateHz[1] = state.getRawParameterValue ("lfo2RateHz")->load();
    lfoDepth[1] = state.getRawParameterValue ("lfo2Depth")->load();
    lfoEnabled[1] = state.getRawParameterValue ("lfo2Enabled")->load() >= 0.5f;
    lfoWaveform[1] = (int) state.getRawParameterValue ("lfo2Waveform")->load();

    matrixDestination[0][0] = (int) state.getRawParameterValue ("matrixDest1")->load();
    matrixDestination[0][1] = (int) state.getRawParameterValue ("matrixDest2")->load();
    matrixDestination[0][2] = (int) state.getRawParameterValue ("matrixDest3")->load();
    matrixAmount[0][0] = state.getRawParameterValue ("matrixAmount1")->load();
    matrixAmount[0][1] = state.getRawParameterValue ("matrixAmount2")->load();
    matrixAmount[0][2] = state.getRawParameterValue ("matrixAmount3")->load();

    matrixDestination[1][0] = (int) state.getRawParameterValue ("matrixDest4")->load();
    matrixDestination[1][1] = (int) state.getRawParameterValue ("matrixDest5")->load();
    matrixDestination[1][2] = (int) state.getRawParameterValue ("matrixDest6")->load();
    matrixAmount[1][0] = state.getRawParameterValue ("matrixAmount4")->load();
    matrixAmount[1][1] = state.getRawParameterValue ("matrixAmount5")->load();
    matrixAmount[1][2] = state.getRawParameterValue ("matrixAmount6")->load();
}

void ModalVoice::configureMaterialModel (MaterialType material,
                                         MalletType mallet,
                                         float baseFrequency,
                                         float velocity,
                                         float hardness)
{
    static constexpr std::array<float, maxModes> woodRatios   { 1.00f, 2.80f, 5.30f, 8.90f, 13.20f, 18.10f, 0.0f, 0.0f, 0.0f, 0.0f };
    static constexpr std::array<float, maxModes> woodDecays   { 0.9993f, 0.9989f, 0.9982f, 0.9974f, 0.9963f, 0.9954f, 0.0f, 0.0f, 0.0f, 0.0f };

    static constexpr std::array<float, maxModes> gongRatios   { 1.00f, 1.42f, 1.91f, 2.63f, 3.66f, 5.08f, 6.90f, 0.0f, 0.0f, 0.0f };
    static constexpr std::array<float, maxModes> gongDecays   { 0.99985f, 0.99980f, 0.99972f, 0.99962f, 0.99952f, 0.99945f, 0.99935f, 0.0f, 0.0f, 0.0f };

    static constexpr std::array<float, maxModes> cymbalRatios { 1.00f, 1.31f, 1.57f, 2.11f, 2.96f, 3.77f, 4.81f, 6.20f, 7.88f, 9.31f };
    static constexpr std::array<float, maxModes> cymbalDecays { 0.99970f, 0.99966f, 0.99960f, 0.99955f, 0.99948f, 0.99942f, 0.99937f, 0.99932f, 0.99928f, 0.99922f };

    static constexpr std::array<float, maxModes> glassRatios  { 1.00f, 2.32f, 4.25f, 6.87f, 9.92f, 13.40f, 0.0f, 0.0f, 0.0f, 0.0f };
    static constexpr std::array<float, maxModes> glassDecays  { 0.99978f, 0.99965f, 0.99948f, 0.99930f, 0.99916f, 0.99894f, 0.0f, 0.0f, 0.0f, 0.0f };

    static constexpr std::array<float, maxModes> aerogelRatios { 1.00f, 1.13f, 1.41f, 1.93f, 2.75f, 3.94f, 5.66f, 7.95f, 10.80f, 14.40f };
    static constexpr std::array<float, maxModes> aerogelDecays { 0.99992f, 0.99990f, 0.99988f, 0.99984f, 0.99980f, 0.99974f, 0.99968f, 0.99960f, 0.99952f, 0.99946f };

    static constexpr std::array<float, maxModes> neutronRatios { 1.00f, 3.40f, 7.80f, 12.50f, 18.30f, 25.00f, 33.00f, 0.0f, 0.0f, 0.0f };
    static constexpr std::array<float, maxModes> neutronDecays { 0.9989f, 0.9980f, 0.9972f, 0.9965f, 0.9957f, 0.9950f, 0.9944f, 0.0f, 0.0f, 0.0f };

    static constexpr std::array<float, maxModes> plasmaIceRatios { 1.00f, 1.67f, 2.01f, 2.03f, 3.55f, 4.90f, 5.02f, 7.70f, 8.11f, 11.80f };
    static constexpr std::array<float, maxModes> plasmaIceDecays { 0.99970f, 0.99982f, 0.99938f, 0.99972f, 0.99920f, 0.99955f, 0.99910f, 0.99936f, 0.99902f, 0.99922f };

    static constexpr std::array<float, maxModes> timeCrystalRatios { 1.00f, 1.50f, 2.25f, 3.38f, 5.06f, 7.59f, 11.39f, 17.09f, 0.0f, 0.0f };
    static constexpr std::array<float, maxModes> timeCrystalDecays { 0.99990f, 0.99910f, 0.99986f, 0.99905f, 0.99980f, 0.99900f, 0.99972f, 0.99895f, 0.0f, 0.0f };

    const std::array<float, maxModes>* ratios = &woodRatios;
    const std::array<float, maxModes>* decays = &woodDecays;
    auto maxMaterialModes = maxModes;
    auto localMaterialDecayBias = -0.00002f;
    auto localHighModeDamping = 0.00010f;
    auto localModeCouplingScale = 1.0f;
    float malletBrightness = 1.0f;
    float couplingScale = 1.0f;

    switch (mallet)
    {
        case MalletType::felt:
            malletBrightness = 0.62f;
            couplingScale = 0.70f;
            break;
        case MalletType::rubber:
            malletBrightness = 0.83f;
            couplingScale = 0.86f;
            break;
        case MalletType::wood:
            malletBrightness = 1.00f;
            couplingScale = 1.00f;
            break;
        case MalletType::metal:
            malletBrightness = 1.38f;
            couplingScale = 1.25f;
            break;
        default:
            break;
    }

    switch (material)
    {
        case MaterialType::gong:
            ratios = &gongRatios;
            decays = &gongDecays;
            maxMaterialModes = 7;
            localMaterialDecayBias = 0.00008f;
            localHighModeDamping = 0.00006f;
            localModeCouplingScale = 1.05f;
            break;
        case MaterialType::cymbal:
            ratios = &cymbalRatios;
            decays = &cymbalDecays;
            maxMaterialModes = 10;
            localMaterialDecayBias = -0.00001f;
            localHighModeDamping = 0.00003f;
            localModeCouplingScale = 1.20f;
            break;
        case MaterialType::glass:
            ratios = &glassRatios;
            decays = &glassDecays;
            maxMaterialModes = 6;
            localMaterialDecayBias = -0.00006f;
            localHighModeDamping = 0.00008f;
            localModeCouplingScale = 1.10f;
            break;
        case MaterialType::aerogelCathedral:
            ratios = &aerogelRatios;
            decays = &aerogelDecays;
            maxMaterialModes = 10;
            localMaterialDecayBias = 0.00012f;
            localHighModeDamping = 0.00002f;
            localModeCouplingScale = 0.92f;
            break;
        case MaterialType::neutronPorcelain:
            ratios = &neutronRatios;
            decays = &neutronDecays;
            maxMaterialModes = 7;
            localMaterialDecayBias = -0.00014f;
            localHighModeDamping = 0.00014f;
            localModeCouplingScale = 1.18f;
            break;
        case MaterialType::plasmaIce:
            ratios = &plasmaIceRatios;
            decays = &plasmaIceDecays;
            maxMaterialModes = 10;
            localMaterialDecayBias = 0.00003f;
            localHighModeDamping = 0.00007f;
            localModeCouplingScale = 1.16f;
            break;
        case MaterialType::timeCrystal:
            ratios = &timeCrystalRatios;
            decays = &timeCrystalDecays;
            maxMaterialModes = 8;
            localMaterialDecayBias = 0.00006f;
            localHighModeDamping = 0.00005f;
            localModeCouplingScale = 1.25f;
            break;
        case MaterialType::wood:
        default:
            maxMaterialModes = 6;
            localMaterialDecayBias = -0.00010f;
            localHighModeDamping = 0.00012f;
            localModeCouplingScale = 0.95f;
            break;
    }

    materialModeCount = juce::jlimit (1, maxModes, maxMaterialModes);
    materialModeDecayBias = localMaterialDecayBias;
    materialHighModeDamping = localHighModeDamping;
    materialModeCouplingScale = localModeCouplingScale;

    activeModes = 0;

    for (int i = 0; i < maxModes; ++i)
    {
        if (i >= materialModeCount)
            break;

        const auto ratio = (*ratios)[i];
        if (ratio <= 0.0f)
            break;

        const auto modeFrequency = juce::jlimit (20.0f, 18000.0f, baseFrequency * ratio);
        phase[i] = 0.0f;
        phaseIncrement[i] = juce::MathConstants<float>::twoPi * modeFrequency / (float) sampleRate;

        const auto brightness = juce::jmap (hardness, 0.4f, 1.8f) * malletBrightness;
        const auto modeWeight = 1.0f / (1.0f + (float) i * 0.45f);
        modeAmplitude[i] = velocity * modeWeight;
        modeCoupling[i] = modeWeight * brightness * couplingScale * materialModeCouplingScale;

        const auto baseDecay = (*decays)[i];
        const auto hardnessLoss = hardness * 0.0003f * (float) i;
        const auto materialLoss = materialHighModeDamping * (float) i;
        modeDecay[i] = juce::jlimit (0.990f, 0.99995f, baseDecay + materialModeDecayBias - hardnessLoss - materialLoss);

        ++activeModes;
    }
}

void ModalVoice::configureSympatheticResonator (float baseFrequency, float velocity)
{
    static constexpr std::array<float, maxSympatheticModes> unisonRatios {
        0.50f, 0.99f, 1.50f, 2.01f, 2.98f, 4.02f, 5.35f, 6.81f
    };

    static constexpr std::array<float, maxSympatheticModes> fifthRatios {
        0.50f, 0.75f, 1.00f, 1.50f, 2.00f, 3.00f, 4.50f, 6.00f
    };

    static constexpr std::array<float, maxSympatheticModes> droneClusterRatios {
        0.50f, 0.94f, 0.99f, 1.02f, 1.07f, 1.50f, 2.01f, 2.96f
    };

    static constexpr std::array<float, maxSympatheticModes> ragaRatios {
        0.50f, 1.00f, 1.125f, 1.25f, 1.50f, 1.875f, 2.00f, 2.50f
    };

    static constexpr std::array<float, maxSympatheticModes> inharmonicCloudRatios {
        0.43f, 0.88f, 1.21f, 1.67f, 2.29f, 3.13f, 4.37f, 5.91f
    };

    static constexpr std::array<float, maxSympatheticModes> sympatheticBaseDecay {
        0.99966f, 0.99973f, 0.99976f, 0.99979f, 0.99982f, 0.99985f, 0.99987f, 0.99989f
    };

    const auto modeTarget = juce::jlimit (2, maxSympatheticModes, sympatheticCourses + 2);
    activeSympatheticModes = modeTarget;
    sympatheticLowpassState = 0.0f;
    ebowCarrierPhase = random.nextFloat() * juce::MathConstants<float>::twoPi;
    ebowFlutterPhase = random.nextFloat() * juce::MathConstants<float>::twoPi;
    ebowCarrierIncrement = juce::MathConstants<float>::twoPi
                           * juce::jlimit (18.0f, 2600.0f, baseFrequency)
                           / (float) sampleRate;
    ebowFlutterIncrement = juce::MathConstants<float>::twoPi * 0.23f / (float) sampleRate;

    const std::array<float, maxSympatheticModes>* tuningRatios = &unisonRatios;

    switch (sympatheticTuningMode)
    {
        case 1: tuningRatios = &fifthRatios; break;
        case 2: tuningRatios = &droneClusterRatios; break;
        case 3: tuningRatios = &ragaRatios; break;
        case 4: tuningRatios = &inharmonicCloudRatios; break;
        case 0:
        default: tuningRatios = &unisonRatios; break;
    }

    float materialDecayBias = -0.00025f;
    float materialBrightness = 1.0f;
    float materialInharmonicity = 0.12f;

    switch (sympatheticMaterial)
    {
        case 1: // Steel
            materialDecayBias = 0.00002f;
            materialBrightness = 1.35f;
            materialInharmonicity = 0.22f;
            break;
        case 2: // Nylon
            materialDecayBias = -0.00035f;
            materialBrightness = 0.72f;
            materialInharmonicity = 0.08f;
            break;
        case 3: // Aether filament
            materialDecayBias = 0.00012f;
            materialBrightness = 1.65f;
            materialInharmonicity = 0.36f;
            break;
        case 0: // Gut
        default:
            materialDecayBias = -0.00022f;
            materialBrightness = 0.88f;
            materialInharmonicity = 0.10f;
            break;
    }

    const auto courseCenter = ((float) sympatheticCourses - 1.0f) * 0.5f;
    const auto spacingCentsPerCourse = 1.2f + sympatheticSpacing * 14.0f;
    const auto detuneSpread = juce::jlimit (0.0f, 48.0f, sympatheticDetuneCents);

    for (int i = 0; i < maxSympatheticModes; ++i)
    {
        if (i >= activeSympatheticModes)
        {
            sympatheticAmplitude[(size_t) i] = 0.0f;
            sympatheticDecay[(size_t) i] = 0.0f;
            sympatheticCoupling[(size_t) i] = 0.0f;
            sympatheticPhaseIncrement[(size_t) i] = 0.0f;
            continue;
        }

        const auto index = (size_t) i;
        const auto ratio = (*tuningRatios)[index];
        const auto courseIndex = (float) (i % sympatheticCourses);
        const auto courseOffset = (courseIndex - courseCenter) * spacingCentsPerCourse;
        const auto randomCents = juce::jmap (random.nextFloat(), -detuneSpread, detuneSpread);
        const auto inharmonicCents = (float) i * (0.9f + sympatheticSpacing * 2.6f) * materialInharmonicity;
        const auto detuneRatio = std::pow (2.0f, (randomCents + courseOffset + inharmonicCents) / 1200.0f);
        const auto modeFrequency = juce::jlimit (20.0f, 16000.0f, baseFrequency * ratio * detuneRatio);
        sympatheticDetuneSignature[index] = random.nextFloat() * 2.0f - 1.0f;

        sympatheticPhase[index] = 0.0f;
        sympatheticPhaseIncrement[index] = juce::MathConstants<float>::twoPi * modeFrequency / (float) sampleRate;

        const auto modeWeight = 1.0f / (1.0f + (float) i * 0.55f);
        sympatheticAmplitude[index] = velocity * modeWeight * (0.045f + 0.02f * sympatheticSpacing) * materialBrightness;
        sympatheticCoupling[index] = modeWeight * (0.12f + 0.10f * sympatheticSpacing);

        const auto decayBias = juce::jmap (sympatheticDecayAmount, 0.0f, 1.0f, -0.0025f, 0.00003f);
        sympatheticDecay[index] = juce::jlimit (0.994f, 0.99997f, sympatheticBaseDecay[index] + decayBias + materialDecayBias);
    }
}

float ModalVoice::renderOneSample (float sympatheticMixValue,
                                   float sympatheticBrightnessValue,
                                   float ebowDriveValue,
                                   float ebowFocusValue,
                                   float sympatheticCouplingValue,
                                   float sympatheticSpacingValue,
                                   float sympatheticDetuneCentsValue)
{
    auto excitation = 0.0f;

    if (excitationSamplesRemaining > 0)
    {
        const auto white = (random.nextFloat() * 2.0f) - 1.0f;
        excitationLowpassState += (white - excitationLowpassState) * excitationLowpassAlpha;
        excitationHighpassState = excitationHighpassAlpha * (excitationHighpassState + white - excitationPrevInput);
        excitationPrevInput = white;

        const auto progress = 1.0f - ((float) excitationSamplesRemaining / (float) juce::jmax (1, excitationTotalSamples));
        const auto clickEnvelope = std::exp (-38.0f * progress);
        const auto click = white * excitationClickAmount * clickEnvelope;

        excitation = (excitationLowpassState * excitationLowpassMix
                      + excitationHighpassState * excitationHighpassMix
                      + click)
                     * excitationState;
        excitationState *= excitationDecay;
        --excitationSamplesRemaining;
    }

    auto sample = 0.0f;

    for (int i = 0; i < activeModes; ++i)
    {
        const auto index = (size_t) i;

        modeAmplitude[index] += excitation * modeCoupling[index] * 0.22f;
        modeAmplitude[index] *= modeDecay[index];

        phase[index] += phaseIncrement[index];
        if (phase[index] > juce::MathConstants<float>::twoPi)
            phase[index] -= juce::MathConstants<float>::twoPi;

        sample += modeAmplitude[index] * std::sin (phase[index]);
    }

    auto sympatheticSample = 0.0f;
    const auto sympatheticExcitation = excitation * 0.7f + sample * sympatheticCouplingValue * 0.17f;
    const auto focusSkew = juce::jmap (ebowFocusValue, 0.8f, 3.4f);
    const auto spacingDelta = sympatheticSpacingValue - sympatheticSpacing;
    const auto detuneDelta = sympatheticDetuneCentsValue - sympatheticDetuneCents;
    const auto courseCenter = ((float) sympatheticCourses - 1.0f) * 0.5f;
    const auto spacingCentsDeltaPerCourse = spacingDelta * 14.0f;

    const auto flutter = std::sin (ebowFlutterPhase);
    const auto carrier = std::sin (ebowCarrierPhase + flutter * 0.35f);
    const auto sourceFollower = juce::jlimit (-1.0f, 1.0f, sample * 1.8f + sympatheticLowpassState * 0.8f);
    const auto ebowDriver = (carrier * 0.62f + sourceFollower * 0.38f) * ebowDriveValue;

    for (int i = 0; i < activeSympatheticModes; ++i)
    {
        const auto index = (size_t) i;
        const auto normalizedIndex = (float) i / (float) juce::jmax (1, activeSympatheticModes - 1);
        const auto focusWeight = std::pow (1.0f - normalizedIndex, focusSkew);

        sympatheticAmplitude[index] += sympatheticExcitation * sympatheticCoupling[index];
        sympatheticAmplitude[index] += ebowDriver * (0.0045f + focusWeight * 0.018f);
        sympatheticAmplitude[index] *= sympatheticDecay[index];

        const auto courseIndex = (float) (i % sympatheticCourses);
        const auto courseOffsetDelta = (courseIndex - courseCenter) * spacingCentsDeltaPerCourse;
        const auto detuneOffsetDelta = sympatheticDetuneSignature[index] * detuneDelta;
        const auto modDetuneRatio = std::pow (2.0f, (courseOffsetDelta + detuneOffsetDelta) / 1200.0f);
        sympatheticPhase[index] += sympatheticPhaseIncrement[index] * modDetuneRatio;
        if (sympatheticPhase[index] > juce::MathConstants<float>::twoPi)
            sympatheticPhase[index] -= juce::MathConstants<float>::twoPi;

        sympatheticSample += sympatheticAmplitude[index] * std::sin (sympatheticPhase[index]);
    }

    ebowCarrierPhase += ebowCarrierIncrement;
    if (ebowCarrierPhase > juce::MathConstants<float>::twoPi)
        ebowCarrierPhase -= juce::MathConstants<float>::twoPi;

    ebowFlutterPhase += ebowFlutterIncrement;
    if (ebowFlutterPhase > juce::MathConstants<float>::twoPi)
        ebowFlutterPhase -= juce::MathConstants<float>::twoPi;

    const auto brightnessAlpha = juce::jmap (sympatheticBrightnessValue, 0.02f, 0.60f);
    sympatheticLowpassState += (sympatheticSample - sympatheticLowpassState) * brightnessAlpha;

    return sample + sympatheticLowpassState * sympatheticMixValue;
}

float ModalVoice::getNextLfoValue (float& lfoPhaseRef, float rateHz, int waveform, float& sampleAndHold)
{
    const auto lfoPhaseValue = lfoPhaseRef;
    auto value = 0.0f;

    switch (waveform)
    {
        case 1: // Triangle
            value = 1.0f - 4.0f * std::abs (lfoPhaseValue - 0.5f);
            break;
        case 2: // Sample and hold
            value = sampleAndHold;
            break;
        case 0:
        default:
                value = std::sin (juce::MathConstants<float>::twoPi * lfoPhaseValue);
            break;
    }

    lfoPhaseRef += rateHz / (float) sampleRate;
    if (lfoPhaseRef >= 1.0f)
    {
        lfoPhaseRef -= 1.0f;
        if (waveform == 2)
            sampleAndHold = random.nextFloat() * 2.0f - 1.0f;
    }

    return value;
}
