/*
 *
 */

#include "si1145.h"

typedef enum
{
    SI1145_REG_PART_ID  = 0x00,
    SI1145_REG_REV_ID   = 0x01,
    SI1145_REG_SEQ_ID   = 0x02,
    SI1145_REG_HW_KEY   = 0x07,
    SI1145_REG_UCOEF0   = 0x13,
    SI1145_REG_UCOEF1   = 0x14,
    SI1145_REG_UCOEF2   = 0x15,
    SI1145_REG_UCOEF3   = 0x16,
    SI1145_REG_PARAM_WR = 0x17,
    SI1145_REG_COMMAND  = 0x18,
    SI1145_REG_RESPONSE = 0x20,
    SI1145_REG_PARAM_RD = 0x2E,
} SI1145_REG;

typedef enum
{
    SI1145_RAM_CHLIST           = 0x01,
    SI1145_RAM_ALS_VIS_ADC_MISC = 0x12,
    SI1145_RAM_ALS_IR_ADC_MISC  = 0x1F
} SI1145_RAM;

typedef enum
{
    SI1145_CMD_PARAM_GET = 0x80,
    SI1145_CMD_PARAM_SET = 0xA0
} SI1145_CMD;

#define SI1145_CONST_REV_ID    0x00
#define SI1145_CONST_SEQ_ID    0x08
#define SI1145_CONST_HW_KEY    0x17
#define SI1145_CONST_PART_ID   0x45
#define SI1145_CONST_VIS_RANGE 0x20
#define SI1145_CONST_IR_RANGE  0x20
#define SI1145_CONST_UCOEF0    0x7B
#define SI1145_CONST_UCOEF1    0x6B
#define SI1145_CONST_UCOEF2    0x01
#define SI1145_CONST_UCOEF3    0x00

#define SI1145_ISSET(map, bit) ((map & bit) != 0)

static I2C_SPA_PARAMS params;

static SI1145_RC si1145_check_reg(SI1145_REG reg, uint8_t expected);
static SI1145_RC si1145_read_reg(SI1145_REG reg, uint8_t *data);
static SI1145_RC si1145_write_reg(SI1145_REG reg, uint8_t data);
static SI1145_RC si1145_check_ram(SI1145_RAM ram, uint8_t expected);
static SI1145_RC si1145_read_ram(SI1145_RAM ram, uint8_t *data);
static SI1145_RC si1145_write_ram(SI1145_RAM ram, uint8_t data);
static SI1145_RC si1145_check_status(void);
static SI1145_RC si1145_send_cmd(SI1145_CMD cmd, uint8_t cmd_low_bits);

static SI1145_RC si1145_check_reg(SI1145_REG reg, uint8_t expected)
{
    uint8_t data;

    if (si1145_read_reg(reg, &data) != I2C_SPA_OK)
    {
        return SI1145_FAILURE;
    }

    return data == expected ? SI1145_OK : SI1145_FAILURE;
}

static SI1145_RC si1145_read_reg(SI1145_REG reg, uint8_t *data)
{
    return i2c_spa_read(&params, reg, 1, data);
}

static SI1145_RC si1145_write_reg(SI1145_REG reg, uint8_t data)
{
    uint8_t read_data;

    if (i2c_spa_write(&params, reg, 1, &data) != I2C_SPA_OK)
    {
        return SI1145_FAILURE;
    }

    return si1145_check_reg(reg, data);
}

static SI1145_RC si1145_check_ram(SI1145_RAM ram, uint8_t expected)
{
    uint8_t data;

    if (si1145_read_ram(ram, &data) != SI1145_OK)
    {
        return SI1145_FAILURE;
    }

    return data == expected ? SI1145_OK : SI1145_FAILURE;
}

static SI1145_RC si1145_read_ram(SI1145_RAM ram, uint8_t *data)
{
    SI1145_RC rc = SI1145_OK;

    if (si1145_send_cmd(SI1145_CMD_PARAM_GET, ram) != SI1145_OK)
    {
        return SI1145_FAILURE;
    }

    if ((rc = si1145_check_status()) != SI1145_OK)
    {
        return rc;
    }

    if (si1145_read_reg(SI1145_REG_PARAM_RD, data) != SI1145_OK)
    {
        return SI1145_FAILURE;
    }

    return SI1145_OK;
}

static SI1145_RC si1145_write_ram(SI1145_RAM ram, uint8_t data)
{
    SI1145_RC rc = SI1145_OK;

    if (si1145_write_reg(SI1145_REG_PARAM_WR, data) != SI1145_OK ||
        si1145_send_cmd(SI1145_CMD_PARAM_SET, ram) != SI1145_OK)
    {
        return SI1145_FAILURE;
    }

    if ((rc = si1145_check_status()) != SI1145_OK)
    {
        return rc;
    }

    return si1145_check_ram(ram, data);
}

static SI1145_RC si1145_check_status(void)
{
    uint8_t data;
    if (si1145_read_reg(SI1145_REG_RESPONSE, &data) != SI1145_OK)
    {
        return SI1145_FAILURE;
    }

    if ((data & 0xF0) == 0x00)
    {
        return SI1145_OK;
    }

    switch (data)
    {
        case 0x80: return SI1145_INVALID_CMD;
        case 0x88: return SI1145_OVRFLW_PS1;
        case 0x89: return SI1145_OVRFLW_PS2;
        case 0x8A: return SI1145_OVRFLW_PS3;
        case 0x8C: return SI1145_OVRFLW_VIS;
        case 0x8D: return SI1145_OVRFLW_IR;
        case 0x8E: return SI1145_OVRFLW_AUX;
        default:   return SI1145_FAILURE;
    }
}

static SI1145_RC si1145_send_cmd(SI1145_CMD cmd, uint8_t cmd_low_bits)
{
    uint8_t command_reg_val = (cmd | cmd_low_bits);

    if (si1145_write_reg(SI1145_REG_COMMAND, 0x00) != SI1145_OK ||
        si1145_write_reg(SI1145_REG_COMMAND, command_reg_val) != SI1145_OK )
    {
        return SI1145_FAILURE;
    }

    return SI1145_OK;
}

SI1145_RC si1145_init(const char *bus, uint8_t addr, uint8_t config_bitmap)
{
    uint8_t chlist = 0x0;
    params.dev_addr = addr;
#ifdef PLATFORM_LINUX
    params.bus = bus;
#endif

    if (i2c_spa_init(&params))
    {
        return SI1145_FAILURE;
    }

    /* Check device */
    if (si1145_check_reg(SI1145_REG_PART_ID, SI1145_CONST_PART_ID) != SI1145_OK)
    {
        printf("Failed to initialize SI1145 (%s)\n", "Part Id");
        return SI1145_FAILURE;
    }

    if (si1145_check_reg(SI1145_REG_REV_ID, SI1145_CONST_REV_ID) != SI1145_OK)
    {
        printf("Failed to initialize SI1145 (%s)\n", "Rev Id");
        return SI1145_FAILURE;
    }

    if (si1145_check_reg(SI1145_REG_SEQ_ID, SI1145_CONST_SEQ_ID) != SI1145_OK)
    {
        printf("Failed to initialize SI1145 (%s)\n", "Seq Id");
        return SI1145_FAILURE;
    }

    /* HW key */
    if (si1145_write_reg(SI1145_REG_HW_KEY, SI1145_CONST_HW_KEY) != SI1145_OK)
    {
        printf("Failed to initialize SI1145 (%s)\n", "HW Key");
        return SI1145_FAILURE;
    }

    /* Proximity sensor */
    if (SI1145_ISSET(config_bitmap, SI1145_CONFIG_BIT_PS))
    {
        chlist |= 0x07;
    }

    /* Ambient light sensor */
    if (SI1145_ISSET(config_bitmap, SI1145_CONFIG_BIT_ALS))
    {
        chlist |= 0x30;
    }

    /* UV calculation */
    if (SI1145_ISSET(config_bitmap, SI1145_CONFIG_BIT_UV))
    {
        chlist |= 0x80;

        /* Set VIS_RANGE */
        if (si1145_write_ram(SI1145_RAM_ALS_VIS_ADC_MISC, SI1145_CONST_VIS_RANGE) != SI1145_OK)
        {
            printf("Failed to initialize SI1145 (%s)\n", "ALS VIS Range");
            return SI1145_FAILURE;
        }

        /* Set IR_RANGE */
        if (si1145_write_ram(SI1145_RAM_ALS_IR_ADC_MISC, SI1145_CONST_IR_RANGE) != SI1145_OK)
        {
            printf("Failed to initialize SI1145 (%s)\n", "ALS IR Range");
            return SI1145_FAILURE;
        }

        /* Default UV calibration coefficients */
        if (si1145_write_reg(SI1145_REG_UCOEF0, SI1145_CONST_UCOEF0) != SI1145_OK ||
            si1145_write_reg(SI1145_REG_UCOEF1, SI1145_CONST_UCOEF1) != SI1145_OK ||
            si1145_write_reg(SI1145_REG_UCOEF2, SI1145_CONST_UCOEF2) != SI1145_OK ||
            si1145_write_reg(SI1145_REG_UCOEF3, SI1145_CONST_UCOEF3) != SI1145_OK)
        {
            printf("Failed to initialize SI1145 (%s)\n", "UCOEF");
            return SI1145_FAILURE;
        }
    }

    /* CHLIST */
    if (si1145_write_ram(SI1145_RAM_CHLIST, chlist) != SI1145_OK)
    {
        printf("Failed to initialize SI1145 (%s)\n", "CHLIST");
        return SI1145_FAILURE;
    }

    uint8_t data;
    if (si1145_read_ram(SI1145_RAM_CHLIST, &data) != SI1145_OK)
    {
        printf("read ram");
        return SI1145_FAILURE;
    }
    printf("CHLIST: 0x%x\n", data);

    if (si1145_read_ram(SI1145_RAM_ALS_VIS_ADC_MISC, &data) != SI1145_OK)
    {
        printf("read ram");
        return SI1145_FAILURE;
    }
    printf("RAM ALS VIS ADC: 0x%x\n", data);

    return SI1145_OK;
}

SI1145_RC si1145_close()
{
    i2c_spa_close(&params);
    return SI1145_OK;
}

