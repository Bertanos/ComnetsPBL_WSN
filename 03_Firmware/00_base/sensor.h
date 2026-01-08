#include <stdbool.h>
#include "periph/i2c.h"

#define TEMP_SENSOR_CHIP_ID  (0x58) // The expected value of the chip id
#define TEMP_SENSOR_I2C_ADDR (0x76) // dec 118
#define TEMP_SENSOR_I2C_NUM  (0)

/*
 * Registers
*/

// FILL IN THESE 
#define TEMP_SENSOR_REG_CHIP_ID    (0xD0)

#define TEMP_SENSOR_REG_TEMP_XLSB  (0xFC)
#define TEMP_SENSOR_REG_TEMP_LSB   (0xFB)
#define TEMP_SENSOR_REG_TEMP_MSB   (0xFA)
#define TEMP_SENSOR_REG_PRESS_XLSB ()
#define TEMP_SENSOR_REG_PRESS_LSB  ()
#define TEMP_SENSOR_REG_PRESS_MSB  ()
#define TEMP_SENSOR_REG_CONFIG     ()
#define TEMP_SENSOR_REG_CTRL_MEAS  (0xF4)
#define TEMP_SENSOR_REG_STATUS     (0xF3)
#define TEMP_SENSOR_REG_RESET      (0xE0)

// Calibration values
#define TEMP_SENSOR_REG_CAL_T1     (0x88)
#define TEMP_SENSOR_REG_CAL_T2     (0x8A)
#define TEMP_SENSOR_REG_CAL_T3     (0x8C)

bool Sensor_Init(void);
