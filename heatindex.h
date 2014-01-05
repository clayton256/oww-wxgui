/* heatindex.h
 */

typedef enum
{
    HI_NONE = 0,
    HI_CAUTION,
    HI_EXTREME_CAUTION,
    HI_DANGER,
    HI_EXTREME_DANGER
}HILevel_t;

#ifdef __cplusplus
extern "C" {
#endif

float heat_index(float temperature, float relativehumidity);
HILevel_t heat_index_level(float heat_index);
const char * heat_index_str(HILevel_t hi);

#ifdef __cplusplus
}
#endif


