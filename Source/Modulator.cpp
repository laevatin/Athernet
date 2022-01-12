#include "Modulator.h"
#include "Config.h"
#include "mkl.h"

void Modulator::modulate(const DataType &data, int start, Frame &frame)
{
    for (int i = start; i < start + Config::BIT_PER_FRAME; i += Config::BAND_WIDTH)
    {
        /* gets 0 if i is out of bound */
        int8_t composed = data[i];
        composed = composed | (data[i + 1] << 1 | data[i + 2] << 2);
        std::cout << (int)composed << " ";

        frame.addSound(Config::modulateSound[composed]);
    }
	std::cout << newLine;
}

/* consume Config::BIT_LENGTH samples, `samples` should contain at least Config::BIT_LENGTH data */
void Modulator::demodulate(const float *samples, DataType &out)
{
    constexpr int MODULATE_NUM = 8;
    float data[MODULATE_NUM];

    for (int i = 0; i < MODULATE_NUM; i++) {
        data[i] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[i].getReadPointer(0), 1);
    }

    float max = 0;
    int maxi = -1;

    for (int i = 0; i < MODULATE_NUM; i++)
        if (data[i] > max)
        {
            max = data[i];
            maxi = i;
        }

    std::cout << maxi << "\n";
    if (maxi == 0 || maxi == 2 || maxi == 4 || maxi == 6)
        out.add((int8_t)0);
    else
        out.add((int8_t)1);

    if (maxi == 2 || maxi == 3 || maxi == 6 || maxi == 7)
        out.add((int8_t)1);
    else
        out.add((int8_t)0);

    if (maxi == 4 || maxi == 5 || maxi == 6 || maxi == 7)
        out.add((int8_t)1);
    else
        out.add((int8_t)0);
}