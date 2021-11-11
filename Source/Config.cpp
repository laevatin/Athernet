#include "Config.h"
#include "Physical/Audio.h"

AudioType Config::header;
std::vector<AudioType> Config::modulateSound;

/* Initialize configuration by its constructor. */
static Config config;

void Config::initPreamble()
{
    // generate the header
    float startFreq = 6000;
    float endFreq   = 16000;

    float f_p[HEADER_LENGTH];
    float omega[HEADER_LENGTH];
    Config::header.setSize(1, HEADER_LENGTH);

    f_p[0] = startFreq;
    for (int i = 1; i < HEADER_LENGTH / 2; i++)
        f_p[i] = f_p[i - 1] + (endFreq - startFreq) / (HEADER_LENGTH / 2);

    f_p[HEADER_LENGTH / 2] = endFreq;
    for (int i = HEADER_LENGTH / 2 + 1; i < HEADER_LENGTH; i++)
        f_p[i] = f_p[i - 1] - (endFreq - startFreq) / (HEADER_LENGTH / 2);

    omega[0] = 0;
    for (int i = 1; i < HEADER_LENGTH; i++)
        omega[i] = omega[i - 1] + (f_p[i] + f_p[i - 1]) / 2.0 * (1.0 / Config::SAMPLE_RATE);

    for (int i = 0; i < HEADER_LENGTH; i++)
        Config::header.setSample(0, i, sin(2 * PI * omega[i]));
}

void Config::initAudio()
{
    int base = 5600;
    AudioType audio1 = generateSound(base, Config::BIT_LENGTH, 0);
    AudioType audio2 = generateSound(base, Config::BIT_LENGTH, PI);
    AudioType audio3 = generateSound(base * 2, Config::BIT_LENGTH, 0);
    AudioType audio4 = generateSound(base * 2, Config::BIT_LENGTH, PI);

    AudioType tmp;
    tmp.setSize(1, Config::BIT_LENGTH);
    for (int i = 0; i < Config::BIT_LENGTH; i++)
        tmp.setSample(0, i, audio1.getSample(0, i) + audio3.getSample(0, i));
    modulateSound.push_back(tmp);
    for (int i = 0; i < Config::BIT_LENGTH; i++)
        tmp.setSample(0, i, audio1.getSample(0, i) + audio4.getSample(0, i));
    modulateSound.push_back(tmp);
    for (int i = 0; i < Config::BIT_LENGTH; i++)
        tmp.setSample(0, i, audio2.getSample(0, i) + audio3.getSample(0, i));
    modulateSound.push_back(tmp);
    for (int i = 0; i < Config::BIT_LENGTH; i++)
        tmp.setSample(0, i, audio2.getSample(0, i) + audio4.getSample(0, i));
    modulateSound.push_back(tmp);
}

AudioType Config::generateSound(int freq, int length, float initPhase)
{
    AudioType sound;
    sound.setSize(1, length);
    auto soundPointer = sound.getWritePointer(0);

    for (int i = 0; i < length; i++)
        soundPointer[i] = sin((float)i * 2 * PI * ((float)freq / (float)Config::SAMPLE_RATE) + initPhase);

    return std::move(sound);
}
