/* heatindex.c 

#gcc heatindex.c -o heatindex -I.

Calculation of Heat Index

The Heat Index (HI) is an index that combines air temperature and relative humidity to determine the human-perceived equivalent temperature (i.e., how hot it feels).

The formula for approximating the heat index in degrees Fahrenheit, to within ±1.3 °F is presented below. 

HI = c1 + c2T + c3R + c4TR + c5T^2 + c6R^2 + c7T^2R + c8TR^2 + c9T^2R^2
Where:
HI = Heat Inbdex in F degrees
T = Ambient dry-bulb Temperature in F
R = Relative Humidity in percent
c1 = -42.379
c2 = 2.04901523
c3 = 10.14333127
c4 = -0.22475541
c5 = -6.83783x10-3
c6 = -5.481717x10-2
c7 = 1.22874x10-3
c8 = 8.5282x10-4
c9 = -1.99x10-6


The above formula for approximately HI is useful only when the temperature is minimally 80 °F with a relative humidity of 40% or higher.

Various physical effects in relation to HI are provided in the table below.

HI (Shade)	Possible Physical Effects
80–90 °F	Caution:  Fatigue is possible with prolonged exposure and activity
90–105 °F	Extreme Caution:  Sunstroke, heat cramps, and heat exhaustion are possible
105–130 °F	Danger:  Sunstroke, heat cramps, and heat exhaustion likely; Heat stroke is possible
> 130 °F	Extreme Danger:  Heat stroke or sunstroke likely with continued exposure

---------------------------------------------------------------------------------------
 If the RH is less than 13% and the temperature is between 80 and 112 degrees F, then the following adjustment is subtracted from HI:
ADJUSTMENT = [(13-RH)/4]*SQRT{[17-ABS(T-95.)]/17}
where ABS and SQRT are the absolute value and square root functions, respectively.  On the other hand, if the RH is greater than 85% and the temperature is between 80 and 87 degrees F, then the following adjustment is added to HI:
ADJUSTMENT = [(RH-85)/10] * [(87-T)/5]
The Rothfusz regression is not appropriate when conditions of temperature and humidity warrant a heat index value below about 80 degrees F. In those cases, a simpler formula (not shown) is applied to calculate values consistent with Steadman's results. This regression is not valid for extreme temperature and relative humidity conditions beyond the range of data considered by Steadman.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <heatindex.h>

#define HI_WITH_ADJUSTMENTS

#define C1  -42.379
#define C2  2.04901523
#define C3  10.14333127
#define C4  -0.22475541
#define C5  -6.83783E-3
#define C6  -5.481717E-2
#define C7  1.22874E-3
#define C8  8.5282E-4
#define C9  -1.99E-6


float heat_index(float temperature, float relative_humidity)
{
    float heat_index = C1 + (C2 * temperature) + /* c1 + c2T + */
        (C3 * relative_humidity) + /*  c3R + */
        (C4 * temperature * relative_humidity) + /* c4TR + */
        (C5 * temperature * temperature) + /* c5T^2 + */
        (C6 * relative_humidity * relative_humidity) + /* c6R^2 +*/
        (C7 * temperature * temperature * relative_humidity) + /*  c7T^2R + */
        (C8 * temperature * relative_humidity * relative_humidity) + /* c8TR^2 + */
        (C9 * temperature * temperature * relative_humidity * relative_humidity); /* c9T^2R^2 */
#ifdef HI_WITH_ADJUSTMENTS
//#warning building with adjustments
    if(relative_humidity < 13.0 && (temperature > 80.0 && temperature < 112.0))
        heat_index -= ((13.0-relative_humidity)/4.0)*sqrt((17.0-(temperature-95.0))/17.0);
    else if (relative_humidity > 85.0 && (temperature > 80.0 && temperature < 87.0))
        heat_index += ((relative_humidity-85.0)/10.0) * ((87.0-temperature)/5.0);
	else
		return temperature;
#endif
    return heat_index;
}


HILevel_t heat_index_level(float heat_index)
{
    if( heat_index < 80)
        return HI_NONE;
    else if( heat_index >= 80 && heat_index < 90)
        return HI_CAUTION;
    else if( heat_index >= 90 && heat_index < 105)
        return HI_EXTREME_CAUTION;
    else if( heat_index >= 105 && heat_index < 130)
        return HI_DANGER;
    else
        return HI_EXTREME_DANGER;
}


const char * heat_index_strings[] = {
	"No Heat Index",
	"Caution",
	"Extreme Caution",
	"Danger",
	"Extreme Danger"
};

const char * heat_index_str(HILevel_t hi)
{
	return heat_index_strings[hi];
}


#ifdef TEST_MAIN
#warning building with test program
int main(int argc, void ** argv)
{
    int hi = heat_index_level(heat_index(90, 45));
    switch(hi)
    {
        case HI_NONE:
            printf("HEAT INDEX None\n");
            break;
        case HI_CAUTION:
            printf("HEAT INDEX Caution\n");
            break;
        case HI_EXTREME_CAUTION:
            printf("HEAT INDEX Extreme Caution\n");
            break;
        case HI_DANGER:
            printf("HEAT INDEX Danger\n");
            break;
        case HI_EXTREME_DANGER:
            printf("HEAT INDEX Extreme Danger\n");
            break;
        default:
            printf("HEAT INDEX Shouldn't get here\n");
            break;
    }

    printf("%s\n", heat_index_str(HI_NONE));
    printf("%s\n", heat_index_str(HI_CAUTION));
    printf("%s\n", heat_index_str(HI_EXTREME_CAUTION));
    printf("%s\n", heat_index_str(HI_DANGER));
    printf("%s\n", heat_index_str(HI_EXTREME_DANGER));

    int i, j;
    printf("T,R,HI\n");
    for(i=79; i<=135; i++)
        for(j=39; j<=100; j++)
            printf("%d,%d,%2.2f\n", i, j, heat_index((float)i, (float)j));

    printf("%s\n", (HI_NONE==heat_index_level(heat_index(79, 45)))?"PASSED":"FAILED");
    printf("%s\n", (HI_CAUTION==heat_index_level(heat_index(80, 45)))?"PASSED":"FAILED");
    printf("%s\n", (HI_EXTREME_CAUTION==heat_index_level(heat_index(90, 45)))?"PASSED":"FAILED");
    printf("%s\n", (HI_DANGER==heat_index_level(heat_index(105, 45)))?"PASSED":"FAILED");
    printf("%s\n", (HI_EXTREME_DANGER==heat_index_level(heat_index(130, 45)))?"PASSED":"FAILED");
    printf("%2.2f,%2.2f,%2.2f\n", 32.3, 95.6, heat_index(32.3, 95.6));
    return 0;
}
#endif


