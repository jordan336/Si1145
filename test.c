/* Jordan Ebel
 * July 2017   
 */

#include "si1145.h"

static const char *I2C_BUS="/dev/i2c-1";
static const int I2C_ADDR = 0x60;

int main(int argv, char **argc)
{
    uint16_t vis_data;
    uint16_t ir_data;
    uint16_t ps1_data;
    uint16_t ps2_data;
    uint16_t ps3_data;
    uint16_t uv_data;

    if (si1145_init(I2C_BUS, I2C_ADDR, SI1145_CONFIG_BIT_ALS | SI1145_CONFIG_BIT_UV) != SI1145_OK)
    {
        return 1;
    }

    if (si1145_measurement_auto(SI1145_MEASUREMENT_ALS) != SI1145_OK)
    {
        return 1;
    }

    int i = 0;
    while (i < 5)
    {
        sleep(2);
        if (si1145_get_vis_data(&vis_data) != SI1145_OK)
        {
            return 1; 
        }
        printf("VIS_DATA: 0x%x\n", vis_data);
        i++;
    }

    if (si1145_measurement_pause(SI1145_MEASUREMENT_ALS) != SI1145_OK)
    {
        return 1;
    }

    if (si1145_get_vis_data(&vis_data) != SI1145_OK ||
        si1145_get_ir_data(&ir_data) != SI1145_OK ||
        si1145_get_ps_data(&ps1_data, &ps2_data, &ps3_data) != SI1145_OK ||
        si1145_get_uv_data(&uv_data) != SI1145_OK)
    {
        return 1; 
    }
    printf("VIS_DATA: 0x%x\n", vis_data);
    printf("IR_DATA: 0x%x\n", ir_data);
    printf("PS1_DATA: 0x%x\n", ps1_data);
    printf("PS2_DATA: 0x%x\n", ps2_data);
    printf("PS3_DATA: 0x%x\n", ps3_data);
    printf("UV_DATA: 0x%x\n", uv_data);
    
    si1145_close();

    return 0;
}

