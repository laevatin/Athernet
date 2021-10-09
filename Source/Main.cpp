/*
  ==============================================================================
    This file contains the basic startup code for a JUCE application.
  ==============================================================================
*/

#include "Audio.h"
#include <objbase.h>
#include <bitset>

#define PI acos(-1)

using namespace juce;

class Tester : public AudioIODeviceCallback {

public:
    Tester() {}

    void audioDeviceAboutToStart(AudioIODevice* device) override {}

    void audioDeviceStopped() override {}

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples) override {
        // Generate Sine Wave Data
        int freq = 9000; // Hz
        float amp = 0.7;
        int sampleRate = 48000;
        int channelNum = 1;
        float dPhasePerSample = 2 * PI * ((float)freq / (float)sampleRate);
        float initPhase = 0;
        float data;

        for (int i = 0; i < numSamples; i++) {
            data = amp * sin(dPhasePerSample * i + initPhase);
            // Write the sample into the output channel 
            outputChannelData[0][i] = data;
        }
    }


};

//==============================================================================
int main(int argc, char* argv[])
{
    MessageManager::getInstance();
    AudioIO audioIO;

    Random rand;
    constexpr int num = 100;
    Array<int> input;
    input.resize(num);

    rand.setSeedRandomly();
    std::cout << "input: ";
    for (int i = 0; i < num; i++)
    {
        if (rand.nextFloat() > 0.5) {
            std::cout << 1 << " ";
            input.set(i, 1);
        } 
        else {
            std::cout << 0 << " ";
            input.set(i, 0);
        }
    }
    std::cout << newLine;

    //bits.test()
    audioIO.setTransmitData(input);
    while (getchar()) 
    {
        std::cout << "output: ";
        for (int i = 0; i < input.size(); i++)
            std::cout << audioIO.getOutput(i) << " ";
        std::cout << newLine;
        audioIO.startTransmit();
    }

    return 0;
}