/* Jordan Ebel
 * July 2017   
 */

#include "si1145.h"

static const char *I2C_BUS="/dev/i2c-1";
static const int I2C_ADDR = 0x60;

int main(int argv, char **argc)
{
    if (si1145_init(I2C_BUS, I2C_ADDR, SI1145_CONFIG_BIT_ALS | SI1145_CONFIG_BIT_UV) != SI1145_OK)
    {
        return 1;
    }

    si1145_close();

    return 0;
}

