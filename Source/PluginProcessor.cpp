#include "PluginProcessor.h"
#include "PluginEditor.h"

ModelingSynthTwoAudioProcessor::ModelingSynthTwoAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    initialiseUserPreferences();
    loadLastPresetDirectoryFromPreferences();
    synth.setCurrentPlaybackSampleRate (44100.0);

    for (int i = 0; i < 12; ++i)
        synth.addVoice (new ModalVoice (parameters, modulationMonitor));

    synth.addSound (new ModalSound());
    prepareFx (44100.0, 512, 2);

    initialisePresetBank();
    applyPreset (0);
}

const juce::StringArray& ModelingSynthTwoAudioProcessor::getPresetNames() const
{
    return presetNames;
}

const ModulationMonitor& ModelingSynthTwoAudioProcessor::getModulationMonitor() const
{
    return modulationMonitor;
}

void ModelingSynthTwoAudioProcessor::setParameterById (const juce::String& parameterId, float value)
{
    if (auto* parameter = parameters.getParameter (parameterId))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

void ModelingSynthTwoAudioProcessor::applyPreset (int presetIndex)
{
    if (! juce::isPositiveAndBelow (presetIndex, (int) presetBank.size()))
        return;

    const auto& preset = presetBank[(size_t) presetIndex];

    setParameterById ("material", (float) preset.material);
    setParameterById ("malletMaterial", (float) preset.malletMaterial);
    setParameterById ("malletHardness", preset.malletHardness);
    setParameterById ("attack", preset.attack);
    setParameterById ("decay", preset.decay);
    setParameterById ("sustain", preset.sustain);
    setParameterById ("release", preset.release);
    setParameterById ("filterCutoff", preset.filterCutoff);
    setParameterById ("filterResonance", preset.filterResonance);
    setParameterById ("reverbMix", preset.reverbMix);
    setParameterById ("reverbRoomSize", preset.reverbRoomSize);
    setParameterById ("reverbDamping", preset.reverbDamping);
    setParameterById ("delayTimeMs", preset.delayTimeMs);
    setParameterById ("delayFeedback", preset.delayFeedback);
    setParameterById ("delayMix", preset.delayMix);
    setParameterById ("sympatheticMix", preset.sympatheticMix);
    setParameterById ("sympatheticDecay", preset.sympatheticDecay);
    setParameterById ("sympatheticBrightness", preset.sympatheticBrightness);
    setParameterById ("sympatheticCoupling", preset.sympatheticCoupling);
    setParameterById ("sympatheticMaterial", (float) preset.sympatheticMaterial);
    setParameterById ("sympatheticCourses", preset.sympatheticCourses);
    setParameterById ("sympatheticSpacing", preset.sympatheticSpacing);
    setParameterById ("sympatheticTuningMode", (float) preset.sympatheticTuningMode);
    setParameterById ("sympatheticDetuneCents", preset.sympatheticDetuneCents);
    setParameterById ("ebowDrive", preset.ebowDrive);
    setParameterById ("ebowFocus", preset.ebowFocus);
    setParameterById ("lfoRateHz", preset.lfoRateHz);
    setParameterById ("lfoDepth", preset.lfoDepth);
    setParameterById ("lfoEnabled", preset.lfoEnabled ? 1.0f : 0.0f);
    setParameterById ("lfoWaveform", (float) preset.lfoWaveform);
    setParameterById ("lfo2RateHz", preset.lfo2RateHz);
    setParameterById ("lfo2Depth", preset.lfo2Depth);
    setParameterById ("lfo2Enabled", preset.lfo2Enabled ? 1.0f : 0.0f);
    setParameterById ("lfo2Waveform", (float) preset.lfo2Waveform);
    setParameterById ("matrixDest1", (float) preset.matrixDest1);
    setParameterById ("matrixAmount1", preset.matrixAmount1);
    setParameterById ("matrixDest2", (float) preset.matrixDest2);
    setParameterById ("matrixAmount2", preset.matrixAmount2);
    setParameterById ("matrixDest3", (float) preset.matrixDest3);
    setParameterById ("matrixAmount3", preset.matrixAmount3);
    setParameterById ("matrixDest4", (float) preset.matrixDest4);
    setParameterById ("matrixAmount4", preset.matrixAmount4);
    setParameterById ("matrixDest5", (float) preset.matrixDest5);
    setParameterById ("matrixAmount5", preset.matrixAmount5);
    setParameterById ("matrixDest6", (float) preset.matrixDest6);
    setParameterById ("matrixAmount6", preset.matrixAmount6);
    setParameterById ("outputGainDb", preset.outputGainDb);
    setParameterById ("overdriveDriveDb", preset.overdriveDriveDb);
    setParameterById ("overdriveTone", preset.overdriveTone);
    setParameterById ("overdriveMix", preset.overdriveMix);
    setParameterById ("overdriveOutputDb", preset.overdriveOutputDb);
}

void ModelingSynthTwoAudioProcessor::randomizeParameters()
{
    juce::Random random;

    auto randomInRange = [&random] (float min, float max)
    {
        return min + random.nextFloat() * (max - min);
    };

    setParameterById ("material", (float) random.nextInt (8));
    setParameterById ("malletMaterial", (float) random.nextInt (4));
    setParameterById ("malletHardness", randomInRange (0.08f, 0.98f));

    setParameterById ("attack", randomInRange (0.001f, 0.45f));
    setParameterById ("decay", randomInRange (0.08f, 4.2f));
    setParameterById ("sustain", randomInRange (0.0f, 0.85f));
    setParameterById ("release", randomInRange (0.05f, 5.8f));

    setParameterById ("filterCutoff", randomInRange (120.0f, 9000.0f));
    setParameterById ("filterResonance", randomInRange (0.25f, 3.5f));

    setParameterById ("reverbMix", randomInRange (0.0f, 0.75f));
    setParameterById ("reverbRoomSize", randomInRange (0.15f, 1.0f));
    setParameterById ("reverbDamping", randomInRange (0.05f, 0.9f));

    setParameterById ("delayTimeMs", randomInRange (25.0f, 820.0f));
    setParameterById ("delayFeedback", randomInRange (0.05f, 0.70f));
    setParameterById ("delayMix", randomInRange (0.0f, 0.45f));

    setParameterById ("sympatheticMix", randomInRange (0.0f, 0.8f));
    setParameterById ("sympatheticDecay", randomInRange (0.12f, 0.95f));
    setParameterById ("sympatheticBrightness", randomInRange (0.08f, 0.95f));
    setParameterById ("sympatheticCoupling", randomInRange (0.05f, 0.9f));
    setParameterById ("sympatheticMaterial", (float) random.nextInt (4));
    setParameterById ("sympatheticCourses", (float) (1 + random.nextInt (6)));
    setParameterById ("sympatheticSpacing", randomInRange (0.0f, 1.0f));
    setParameterById ("sympatheticTuningMode", (float) random.nextInt (5));
    setParameterById ("sympatheticDetuneCents", randomInRange (0.0f, 28.0f));

    setParameterById ("ebowDrive", randomInRange (0.0f, 0.85f));
    setParameterById ("ebowFocus", randomInRange (0.1f, 0.95f));

    setParameterById ("lfoRateHz", randomInRange (0.05f, 12.0f));
    setParameterById ("lfoDepth", randomInRange (0.0f, 0.9f));
    setParameterById ("lfoEnabled", random.nextBool() ? 1.0f : 0.0f);
    setParameterById ("lfoWaveform", (float) random.nextInt (3));
    setParameterById ("lfo2RateHz", randomInRange (0.05f, 12.0f));
    setParameterById ("lfo2Depth", randomInRange (0.0f, 0.9f));
    setParameterById ("lfo2Enabled", random.nextBool() ? 1.0f : 0.0f);
    setParameterById ("lfo2Waveform", (float) random.nextInt (3));

    setParameterById ("matrixDest1", (float) random.nextInt (11));
    setParameterById ("matrixAmount1", randomInRange (-0.85f, 0.85f));
    setParameterById ("matrixDest2", (float) random.nextInt (11));
    setParameterById ("matrixAmount2", randomInRange (-0.85f, 0.85f));
    setParameterById ("matrixDest3", (float) random.nextInt (11));
    setParameterById ("matrixAmount3", randomInRange (-0.85f, 0.85f));
    setParameterById ("matrixDest4", (float) random.nextInt (11));
    setParameterById ("matrixAmount4", randomInRange (-0.85f, 0.85f));
    setParameterById ("matrixDest5", (float) random.nextInt (11));
    setParameterById ("matrixAmount5", randomInRange (-0.85f, 0.85f));
    setParameterById ("matrixDest6", (float) random.nextInt (11));
    setParameterById ("matrixAmount6", randomInRange (-0.85f, 0.85f));

    setParameterById ("outputGainDb", randomInRange (-18.0f, -4.0f));

    setParameterById ("overdriveDriveDb", randomInRange (0.0f, 24.0f));
    setParameterById ("overdriveTone", randomInRange (0.2f, 0.9f));
    setParameterById ("overdriveMix", randomInRange (0.0f, 0.55f));
    setParameterById ("overdriveOutputDb", randomInRange (-6.0f, 4.0f));
}

bool ModelingSynthTwoAudioProcessor::savePresetToFile (const juce::File& file)
{
    auto stateTree = parameters.copyState();
    stateTree.removeProperty (lastPresetDirectoryKey, nullptr);
    auto xml = std::unique_ptr<juce::XmlElement> (stateTree.createXml());

    if (xml == nullptr)
        return false;

    while (auto* oldMetadata = xml->getChildByName ("PresetMetadata"))
        xml->removeChildElement (oldMetadata, true);

    if (auto* metadata = xml->createNewChildElement ("PresetMetadata"))
    {
        metadata->setAttribute ("plugin", JucePlugin_Name);
        metadata->setAttribute ("version", JucePlugin_VersionString);
        metadata->setAttribute ("savedAt", juce::Time::getCurrentTime().toISO8601 (true));
    }

    if (! xml->writeTo (file))
        return false;

    setLastPresetDirectory (file.getParentDirectory());
    return true;
}

bool ModelingSynthTwoAudioProcessor::loadPresetFromFile (const juce::File& file)
{
    std::unique_ptr<juce::XmlElement> xmlState (juce::XmlDocument::parse (file));

    if (xmlState == nullptr || ! xmlState->hasTagName (parameters.state.getType()))
        return false;

    auto restoredState = juce::ValueTree::fromXml (*xmlState);
    restoredState.removeProperty (lastPresetDirectoryKey, nullptr);
    parameters.replaceState (restoredState);
    setLastPresetDirectory (file.getParentDirectory());
    return true;
}

juce::File ModelingSynthTwoAudioProcessor::getLastPresetDirectory() const
{
    return lastPresetDirectory;
}

void ModelingSynthTwoAudioProcessor::setLastPresetDirectory (const juce::File& directory)
{
    if (directory.isDirectory())
    {
        lastPresetDirectory = directory;
        storeLastPresetDirectoryInPreferences();
        return;
    }

    if (directory.existsAsFile())
    {
        lastPresetDirectory = directory.getParentDirectory();
        storeLastPresetDirectoryInPreferences();
    }
}

void ModelingSynthTwoAudioProcessor::initialiseUserPreferences()
{
    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
   #if JUCE_LINUX || JUCE_BSD
    options.folderName = "~/.config";
   #else
    options.folderName = "";
   #endif

    appProperties.setStorageParameters (options);
}

void ModelingSynthTwoAudioProcessor::loadLastPresetDirectoryFromPreferences()
{
    lastPresetDirectory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    if (auto* settings = appProperties.getUserSettings())
    {
        const auto savedDirectory = settings->getValue (lastPresetDirectoryKey);
        if (savedDirectory.isNotEmpty())
            setLastPresetDirectory (juce::File (savedDirectory));
    }
}

void ModelingSynthTwoAudioProcessor::storeLastPresetDirectoryInPreferences()
{
    if (auto* settings = appProperties.getUserSettings())
        settings->setValue (lastPresetDirectoryKey, lastPresetDirectory.getFullPathName());
}

void ModelingSynthTwoAudioProcessor::initialisePresetBank()
{
    presetBank = {
        { "Wood Block - Tight Studio", 0, 0.44f, 0.003f, 0.28f, 0.05f, 0.21f, 1700.0f, 0.65f, 0.08f, 0.25f, 0.62f, 120.0f, 0.18f, 0.10f, 0.18f, 0.44f, 0.33f, 0.27f, 0.08f, 0.52f, -8.0f },
        { "Gong - Temple Bloom", 1, 0.52f, 0.005f, 2.40f, 0.20f, 3.50f, 1600.0f, 1.35f, 0.43f, 0.88f, 0.30f, 480.0f, 0.42f, 0.26f, 0.34f, 0.70f, 0.40f, 0.50f, 0.24f, 0.62f, -12.0f },
        { "Cymbal - Dust Trail", 2, 0.77f, 0.001f, 1.30f, 0.14f, 1.70f, 4200.0f, 0.95f, 0.28f, 0.58f, 0.25f, 180.0f, 0.27f, 0.20f, 0.21f, 0.48f, 0.56f, 0.36f, 0.16f, 0.38f, -13.0f },
        { "Glass - Hollow Prism", 3, 0.68f, 0.002f, 1.90f, 0.10f, 2.10f, 3100.0f, 1.85f, 0.34f, 0.65f, 0.19f, 360.0f, 0.34f, 0.23f, 0.29f, 0.61f, 0.50f, 0.43f, 0.28f, 0.57f, -11.0f },
        { "Aerogel Cathedral - Floating Choir", 4, 0.22f, 0.020f, 4.80f, 0.35f, 5.80f, 1400.0f, 1.15f, 0.62f, 1.00f, 0.08f, 640.0f, 0.47f, 0.30f, 0.62f, 0.93f, 0.28f, 0.70f, 0.76f, 0.80f, -14.0f },
        { "Neutron Porcelain - Crater Bell", 5, 0.91f, 0.001f, 0.75f, 0.07f, 1.10f, 6100.0f, 2.10f, 0.18f, 0.46f, 0.41f, 95.0f, 0.22f, 0.16f, 0.15f, 0.35f, 0.72f, 0.26f, 0.13f, 0.34f, -16.0f },
        { "Plasma Ice - Arc Snow", 6, 0.63f, 0.003f, 2.60f, 0.18f, 2.90f, 5200.0f, 1.55f, 0.40f, 0.76f, 0.16f, 290.0f, 0.39f, 0.31f, 0.47f, 0.82f, 0.64f, 0.58f, 0.54f, 0.61f, -13.5f },
        { "Time Crystal - Recursive Chime", 7, 0.49f, 0.006f, 3.20f, 0.33f, 4.40f, 2400.0f, 1.95f, 0.46f, 0.84f, 0.20f, 510.0f, 0.51f, 0.28f, 0.53f, 0.88f, 0.36f, 0.66f, 0.65f, 0.75f, -15.0f }
    };

    presetNames.clearQuick();
    for (const auto& preset : presetBank)
        presetNames.add (preset.name);
}

void ModelingSynthTwoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    prepareFx (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void ModelingSynthTwoAudioProcessor::releaseResources()
{
}

bool ModelingSynthTwoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ModelingSynthTwoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (getSampleRate() <= 0.0)
    {
        buffer.clear();
        return;
    }

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    buffer.clear();
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    processOverdrive (buffer);
    processDelay (buffer);
    processReverb (buffer);
    processCompressor (buffer);
}

void ModelingSynthTwoAudioProcessor::prepareFx (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused (samplesPerBlock);

    const auto maxDelaySeconds = 2.0;
    const auto delayBufferSize = (int) (sampleRate * maxDelaySeconds);

    delayBufferL.assign ((size_t) delayBufferSize, 0.0f);
    delayBufferR.assign ((size_t) delayBufferSize, 0.0f);
    delayWritePosition = 0;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) juce::jmax (samplesPerBlock, 32);
    spec.numChannels = (juce::uint32) juce::jmax (numChannels, 1);

    reverb.prepare (spec);
    reverb.reset();

    compressor.prepare (spec);
    compressor.reset();

    overdrivePreEmphasisState.assign ((size_t) juce::jmax (numChannels, 1), 0.0f);
    overdriveToneState.assign ((size_t) juce::jmax (numChannels, 1), 0.0f);
}

void ModelingSynthTwoAudioProcessor::processOverdrive (juce::AudioBuffer<float>& buffer)
{
    const auto mix = juce::jlimit (0.0f, 1.0f, parameters.getRawParameterValue ("overdriveMix")->load());

    if (mix <= 0.0001f)
        return;

    const auto driveDb = parameters.getRawParameterValue ("overdriveDriveDb")->load();
    const auto driveGain = juce::Decibels::decibelsToGain (driveDb);
    const auto tone = juce::jlimit (0.0f, 1.0f, parameters.getRawParameterValue ("overdriveTone")->load());
    const auto outputDb = parameters.getRawParameterValue ("overdriveOutputDb")->load();
    const auto outputGain = juce::Decibels::decibelsToGain (outputDb);

    const auto sampleRate = juce::jmax (1.0, getSampleRate());
    const auto toneCutoffHz = 650.0f + (tone * tone * 9500.0f);
    const auto toneAlpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * toneCutoffHz / (float) sampleRate);
    const auto preEmphasis = 0.24f + (tone * 0.48f);
    const auto asymmetry = 0.08f + ((1.0f - tone) * 0.17f);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        auto& preState = overdrivePreEmphasisState[(size_t) channel];
        auto& toneState = overdriveToneState[(size_t) channel];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto dry = samples[i];

            const auto pre = dry + preEmphasis * (dry - preState);
            preState = dry;

            const auto staged = pre * driveGain;
            const auto positive = std::tanh (staged + asymmetry);
            const auto negative = std::tanh (staged - (asymmetry * 0.65f));
            const auto clipped = 0.5f * (positive + negative);

            toneState += toneAlpha * (clipped - toneState);
            const auto wet = toneState * outputGain;

            samples[i] = (dry * (1.0f - mix)) + (wet * mix);
        }
    }
}

void ModelingSynthTwoAudioProcessor::processDelay (juce::AudioBuffer<float>& buffer)
{
    const auto delayMix = juce::jlimit (0.0f, 0.92f, parameters.getRawParameterValue ("delayMix")->load());
    const auto delayFeedback = parameters.getRawParameterValue ("delayFeedback")->load();
    const auto delayTimeMs = parameters.getRawParameterValue ("delayTimeMs")->load();

    if (delayMix <= 0.0001f)
        return;

    const auto sampleRate = getSampleRate();
    const auto delayInSamples = (int) juce::jlimit (1.0f,
                                                    (float) delayBufferL.size() - 1.0f,
                                                    (delayTimeMs / 1000.0f) * (float) sampleRate);

    const auto numSamples = buffer.getNumSamples();
    const auto delayBufferSize = (int) delayBufferL.size();

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto readPosition = (delayWritePosition + delayBufferSize - delayInSamples) % delayBufferSize;

        const auto dryL = left[i];
        const auto wetL = delayBufferL[(size_t) readPosition];
        delayBufferL[(size_t) delayWritePosition] = dryL + (wetL * delayFeedback);

        left[i] = dryL * (1.0f - delayMix) + wetL * delayMix;

        if (right != nullptr)
        {
            const auto dryR = right[i];
            const auto wetR = delayBufferR[(size_t) readPosition];
            delayBufferR[(size_t) delayWritePosition] = dryR + (wetR * delayFeedback);

            right[i] = dryR * (1.0f - delayMix) + wetR * delayMix;
        }

        delayWritePosition = (delayWritePosition + 1) % delayBufferSize;
    }
}

void ModelingSynthTwoAudioProcessor::processReverb (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::Reverb::Parameters params;
    params.roomSize = parameters.getRawParameterValue ("reverbRoomSize")->load();
    params.damping = parameters.getRawParameterValue ("reverbDamping")->load();
    params.wetLevel = juce::jlimit (0.0f, 0.88f, parameters.getRawParameterValue ("reverbMix")->load());
    params.dryLevel = 1.0f - params.wetLevel;
    params.width = 0.9f;
    params.freezeMode = 0.0f;

    reverb.setParameters (params);

    auto block = juce::dsp::AudioBlock<float> (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);
}

void ModelingSynthTwoAudioProcessor::processCompressor (juce::AudioBuffer<float>& buffer)
{
    const auto thresholdDb = parameters.getRawParameterValue ("compThresholdDb")->load();
    const auto ratio = parameters.getRawParameterValue ("compRatio")->load();
    const auto attackMs = parameters.getRawParameterValue ("compAttackMs")->load();
    const auto releaseMs = parameters.getRawParameterValue ("compReleaseMs")->load();
    const auto makeupDb = parameters.getRawParameterValue ("compMakeupDb")->load();

    compressor.setThreshold (thresholdDb);
    compressor.setRatio (ratio);
    compressor.setAttack (attackMs);
    compressor.setRelease (releaseMs);

    auto block = juce::dsp::AudioBlock<float> (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    compressor.process (context);

    const auto makeup = juce::Decibels::decibelsToGain (makeupDb);
    buffer.applyGain (makeup);
}

juce::AudioProcessorEditor* ModelingSynthTwoAudioProcessor::createEditor()
{
    return new ModelingSynthTwoAudioProcessorEditor (*this);
}

bool ModelingSynthTwoAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String ModelingSynthTwoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ModelingSynthTwoAudioProcessor::acceptsMidi() const
{
    return true;
}

bool ModelingSynthTwoAudioProcessor::producesMidi() const
{
    return false;
}

bool ModelingSynthTwoAudioProcessor::isMidiEffect() const
{
    return false;
}

double ModelingSynthTwoAudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int ModelingSynthTwoAudioProcessor::getNumPrograms()
{
    return 1;
}

int ModelingSynthTwoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ModelingSynthTwoAudioProcessor::setCurrentProgram (int)
{
}

const juce::String ModelingSynthTwoAudioProcessor::getProgramName (int)
{
    return {};
}

void ModelingSynthTwoAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void ModelingSynthTwoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto stateTree = parameters.copyState();
    stateTree.removeProperty (lastPresetDirectoryKey, nullptr);
    std::unique_ptr<juce::XmlElement> xml (stateTree.createXml());
    copyXmlToBinary (*xml, destData);
}

void ModelingSynthTwoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            auto restoredState = juce::ValueTree::fromXml (*xmlState);
            restoredState.removeProperty (lastPresetDirectoryKey, nullptr);
            parameters.replaceState (restoredState);
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ModelingSynthTwoAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "material", "Material", juce::StringArray {
            "Wood Block",
            "Gong",
            "Cymbal",
            "Glass",
            "Aerogel Cathedral",
            "Neutron Porcelain",
            "Plasma Ice",
            "Time Crystal"
        }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "malletHardness", "Mallet Hardness", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "malletMaterial", "Mallet Material", juce::StringArray { "Felt", "Rubber", "Wood", "Metal" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack", juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.35f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "decay", "Decay", juce::NormalisableRange<float> (0.001f, 6.0f, 0.0f, 0.35f), 0.7f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sustain", "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release", juce::NormalisableRange<float> (0.005f, 8.0f, 0.0f, 0.35f), 1.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "filterCutoff", "Filter Cutoff", juce::NormalisableRange<float> (60.0f, 12000.0f, 0.01f, 0.25f), 1800.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "filterResonance", "Filter Resonance", juce::NormalisableRange<float> (0.2f, 8.0f, 0.001f, 0.4f), 0.85f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbMix", "Reverb Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbRoomSize", "Reverb Room Size", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbDamping", "Reverb Damping", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "delayTimeMs", "Delay Time", juce::NormalisableRange<float> (1.0f, 1200.0f, 0.01f, 0.35f), 260.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "delayFeedback", "Delay Feedback", juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "delayMix", "Delay Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.18f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticMix", "Sympathetic Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticDecay", "Sympathetic Decay", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticBrightness", "Sympathetic Brightness", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticCoupling", "Sympathetic Coupling", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.40f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "sympatheticMaterial", "Sympathetic Material", juce::StringArray { "Gut", "Steel", "Nylon", "Aether Filament" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticCourses", "Sympathetic Courses", juce::NormalisableRange<float> (1.0f, 6.0f, 1.0f), 3.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticSpacing", "Sympathetic Spacing", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "sympatheticTuningMode", "Sympathetic Tuning Mode", juce::StringArray {
            "Unison Stack",
            "Perfect Fifths",
            "Drone Cluster",
            "Raga Resonance",
            "Inharmonic Cloud"
        }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sympatheticDetuneCents", "Sympathetic Detune Spread", juce::NormalisableRange<float> (0.0f, 36.0f, 0.01f), 7.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ebowDrive", "Ebow Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.20f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ebowFocus", "Ebow Focus", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.56f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lfoRateHz", "LFO Rate", juce::NormalisableRange<float> (0.05f, 20.0f, 0.001f, 0.35f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lfoDepth", "LFO Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "lfoEnabled", "LFO Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "lfoWaveform", "LFO Waveform", juce::StringArray { "Sine", "Triangle", "Sample & Hold" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lfo2RateHz", "LFO 2 Rate", juce::NormalisableRange<float> (0.05f, 20.0f, 0.001f, 0.35f), 0.60f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lfo2Depth", "LFO 2 Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "lfo2Enabled", "LFO 2 Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "lfo2Waveform", "LFO 2 Waveform", juce::StringArray { "Sine", "Triangle", "Sample & Hold" }, 0));

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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "matrixDest1", "Matrix Destination 1", matrixDestinations, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "matrixAmount1", "Matrix Amount 1", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "matrixDest2", "Matrix Destination 2", matrixDestinations, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "matrixAmount2", "Matrix Amount 2", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "matrixDest3", "Matrix Destination 3", matrixDestinations, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "matrixAmount3", "Matrix Amount 3", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "matrixDest4", "Matrix Destination 4", matrixDestinations, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "matrixAmount4", "Matrix Amount 4", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "matrixDest5", "Matrix Destination 5", matrixDestinations, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "matrixAmount5", "Matrix Amount 5", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "matrixDest6", "Matrix Destination 6", matrixDestinations, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "matrixAmount6", "Matrix Amount 6", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGainDb", "Output Gain", juce::NormalisableRange<float> (-24.0f, 6.0f, 0.01f), -9.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compThresholdDb", "Compressor Threshold", juce::NormalisableRange<float> (-48.0f, 0.0f, 0.01f), -16.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compRatio", "Compressor Ratio", juce::NormalisableRange<float> (1.0f, 16.0f, 0.01f), 3.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compAttackMs", "Compressor Attack", juce::NormalisableRange<float> (1.0f, 200.0f, 0.01f, 0.5f), 15.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compReleaseMs", "Compressor Release", juce::NormalisableRange<float> (20.0f, 1000.0f, 0.01f, 0.4f), 180.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compMakeupDb", "Compressor Makeup", juce::NormalisableRange<float> (0.0f, 18.0f, 0.01f), 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "overdriveDriveDb", "Overdrive Drive", juce::NormalisableRange<float> (0.0f, 36.0f, 0.01f), 8.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "overdriveTone", "Overdrive Tone", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "overdriveMix", "Overdrive Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "overdriveOutputDb", "Overdrive Output", juce::NormalisableRange<float> (-18.0f, 12.0f, 0.01f), 0.0f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ModelingSynthTwoAudioProcessor();
}
