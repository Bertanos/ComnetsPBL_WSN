#include <stdbool.h>
#include "periph/i2c.h"

#define TEMP_SENSOR_CHIP_ID  (0x58) // The expected value of the chip id
#define TEMP_SENSOR_I2C_ADDR (0x76) // dec 118
#define TEMP_SENSOR_I2C_NUM  (0) 

/*
 * Registers
*/

// FILL IN THESE 
#define TEMP_SENSOR_REG_CHIP_ID    ()

#define TEMP_SENSOR_REG_TEMP_XLSB  ()
#define TEMP_SENSOR_REG_TEMP_LSB   ()
#define TEMP_SENSOR_REG_TEMP_MSB   ()
#define TEMP_SENSOR_REG_PRESS_XLSB ()
#define TEMP_SENSOR_REG_PRESS_LSB  ()
#define TEMP_SENSOR_REG_PRESS_MSB  ()
#define TEMP_SENSOR_REG_CONFIG     ()
#define TEMP_SENSOR_REG_CTRL_MEAS  ()
#define TEMP_SENSOR_REG_STATUS     ()
#define TEMP_SENSOR_REG_RESET      ()

// Calibration values
#define TEMP_SENSOR_REG_CAL_T1     ()
#define TEMP_SENSOR_REG_CAL_T2     ()
#define TEMP_SENSOR_REG_CAL_T3     ()

bool Sensor_Init(void);
