#include "Physical/Modulator.h"
#include "Config.h"

float m_dot2(int length, const float *x, const float *y) {
    float sum = 0;
    for (int i = 0; i < length; i++)
        sum += x[i] * y[i];
    return sum;
}

/* consume Config::BIT_LENGTH samples, `samples` should contain at least Config::BIT_LENGTH data */
void Modulator::demodulate(const float *samples, DataType &out) {
    float data[4];

    data[0] = m_dot2(Config::BIT_LENGTH, samples, Config::modulateSound[0].getReadPointer(0));
    data[1] = m_dot2(Config::BIT_LENGTH, samples, Config::modulateSound[1].getReadPointer(0));
    data[2] = m_dot2(Config::BIT_LENGTH, samples, Config::modulateSound[2].getReadPointer(0));
    data[3] = m_dot2(Config::BIT_LENGTH, samples, Config::modulateSound[3].getReadPointer(0));

    float max = 0;
    int maxi = -1;

    for (int i = 0; i < 4; i++)
        if (data[i] > max) {
            max = data[i];
            maxi = i;
        }

    // std::cout << maxi << "\n";
    if (maxi == 0 || maxi == 2)
        out.add((uint8_t) 0);
    else
        out.add((uint8_t) 1);

    if (maxi == 2 || maxi == 3)
        out.add((uint8_t) 1);
    else
        out.add((uint8_t) 0);
}

void Modulator::modulate(AudioFrame &audioFrame, const DataType &bitArray) {
    for (int i = 0; i < bitArray.size(); i += Config::BAND_WIDTH) {
        /* gets 0 if i is out of bound */
        uint8_t composed = bitArray[i];
        composed = composed | (bitArray[i + 1] << 1);

        audioFrame.addSound(Config::modulateSound[composed]);
    }
}
