#include "Physical/Modulator.h"
#include "Config.h"
#include "mkl.h"

void Modulator::modulate(const DataType &data, int start, Frame &frame)
{
    for (int i = start; i < start + Config::BIT_PER_FRAME; i += Config::BAND_WIDTH)
    {
        /* gets 0 if i is out of bound */
        uint8_t composed = data[i];
        composed = composed | (data[i + 1] << 1);
        std::cout << (int)data[i] << (int)data[i + 1];

        frame.addSound(Config::modulateSound[composed]);
    }
	std::cout << newLine;
}

/* consume Config::BIT_LENGTH samples, `samples` should contain at least Config::BIT_LENGTH data */
void Modulator::demodulate(const float *samples, DataType &out)
{
    float data[4];
    data[0] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[0].getReadPointer(0), 1);
    data[1] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[1].getReadPointer(0), 1);
    data[2] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[2].getReadPointer(0), 1);
    data[3] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[3].getReadPointer(0), 1);

    float max = 0;
    int maxi = -1;

    for (int i = 0; i < 4; i++)
        if (data[i] > max)
        {
            max = data[i];
            maxi = i;
        }

    // std::cout << maxi << "\n";
    if (maxi == 0 || maxi == 2)
        out.add((uint8_t)0);
    else
        out.add((uint8_t)1);

    if (maxi == 2 || maxi == 3)
        out.add((uint8_t)1);
    else
        out.add((uint8_t)0);
}
