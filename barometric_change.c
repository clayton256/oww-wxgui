/*
 * Steady:                            Less than 0.1hPa      0.003in-Hg           0.08mm-Hg
 * Slowly rising or falling             0.15 to 1.5hPa  0.003 to 0.04in-Hg    0.08 to 1.1mm-Hg
 * Rising or falling                    1.6 to 3.5hPa   0.05 to 0.1in-Hg      1.2 to 2.6mm-Hg
 * Quickly rising or falling            3.6 to 6.0hPa   0.1 to 0.18in-Hg      2.7 to 4.5mm-Hg
 * Rapidly rising or falling          More than 6.0hPa      0.18in-Hg            4.5mm-Hg
 * (hPa == millibar)
 * To convert inches of mercury to millibars, multiply the inches value by 33.8637526
 * To convert millibars to inches of mercury, multiply the millibar value by 0.0295301
 */

#include <complex.h>

#define RAPIDLY_RISING  4
#define QUICKLY_RISING  3
#define RISING          2
#define SLOWLY_RISING   1
#define STEADY          0
#define SLOWLY_FALLING  -1
#define FALLING         -2
#define QUICKLY_FALLING -3
#define RAPIDLY_FALLING -4

char change_char[] = {'R','r','s','f','F'};

barometric_change(double current_reading)
{
    diff = past_reading - current_reading;
    if(0 > diff)
    {
        sign = -1;
    }
    else
    {
        sign = 1;
    }
    diff = cabs(diff);
    if(0.08 >= diff)
    {
        pressure_tendency = 0;
    }
    else if(1.1 >= diff)
    {
        pressure_tendency = 1;
    }else if(2.6 >= diff)
    {
        pressure_tendency = 2;
    }
    else if(4.5 >= diff)
    {
        pressure_tendency = 3;
    }
    else
    {
        pressure_tendency = 4;
    }

    return pressure_tendency * sign;
}



