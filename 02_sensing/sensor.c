#include "sensor.h"
#include "shell.h"
#include <stdlib.h>


i2c_t i2cDevice;

static uint8_t i2c_buf[128U];

static uint16_t dig_T1 = 0; // Reference manual chapter 3.11.2
static int16_t  dig_T2 = 0;
static int16_t  dig_T3 = 0;

static inline void _print_i2c_read(i2c_t dev, uint16_t *reg, uint8_t *buf, int len)
{
    printf("Success: i2c_%i read %i byte(s) ", dev, len);
    if (reg != NULL) {
        printf("from reg 0x%02x ", *reg);
    }
    printf(": [");
    for (int i = 0; i < len; i++) {
        if (i != 0) {
            printf(", ");
        }
        printf("0x%02x", buf[i]);
    }
    printf("]\n");
}

static int cmd_i2c_write_reg(int argc, char **argv)
{
    int res;
    uint16_t addr;
    uint16_t reg;
    uint8_t flags = 0;
    uint8_t data;
    int dev;

    dev = 0;
    addr = atoi(argv[2]);
    reg = atoi(argv[3]);
    data = atoi(argv[4]);

    printf("Command: i2c_write_reg(%i, 0x%02x, 0x%02x, 0x%02x, [0x%02x", dev, addr, reg, flags, data);
    puts("])");
    res = i2c_write_reg(dev, addr, reg, data, flags);

    if (res == 0) {
        printf("Success: i2c_%i wrote 1 byte\n", dev);
        return 0;
    }
    return res;
}

static int cmd_i2c_read_reg(int argc, char **argv)
{
    int res;
    uint16_t addr;
    uint16_t reg;
    uint8_t flags = 0;
    uint8_t data;
    int dev;

    dev = 0;
    addr = atoi(argv[2]);
    reg = atoi(argv[3]);
    
    printf("Command: i2c_read_reg(%i, 0x%02x, 0x%02x, 0x%02x)\n", dev, addr, reg, flags);
    res = i2c_read_reg(dev, addr, reg, &data, flags);

    if (res == 0) {
        _print_i2c_read(dev, &reg, &data, 1);
        return 0;
    }
    return res;
}

static int cmd_i2c_read_regs(int argc, char **argv)
{
    int res;
    uint16_t addr;
    uint16_t reg;
    uint8_t flags = 0;
    int len;
    int dev;

    dev = 0;
    addr = atoi(argv[2]);
    reg = atoi(argv[3]);
    len = atoi(argv[4]);

    if (len < 1 || len > (int)128U) {
        puts("Error: invalid LENGTH parameter given");
        return 1;
    }
    else {
        printf("Command: i2c_read_regs(%i, 0x%02x, 0x%02x, %i, 0x%02x)\n", dev, addr, reg, len, flags);
        res = i2c_read_regs(dev, addr, reg, i2c_buf, len, flags);
    }

    if (res == 0) {
        _print_i2c_read(dev, &reg, i2c_buf, len);
        return 0;
    }
    return res;
}

bool Sensor_GetChipId(uint8_t *id)
{
	int res;
	uint16_t addr = TEMP_SENSOR_I2C_ADDR;
    uint16_t reg = TEMP_SENSOR_REG_CHIP_ID;
    uint8_t data;
	int dev = 0;
	int flags = 0;

	res = i2c_read_reg(dev, addr, reg, &data, flags);

    if (res == 0) {
        *id = data;
        return true;
    }
    return false;
}

bool Sensor_Reset(void)
{
  // From the datasheet:
  // "The “reset” register contains the soft reset word reset[7:0]. If the value 0xB6 is written to the register,
  // the device is reset using the complete power-on-reset procedure. Writing other values than 0xB6 has
  // no effect. The readout value is always 0x00"
  //
  // Optional //
  return true;
}

bool Sensor_GetStatus(uint8_t *status)
{
  // The “status” register contains two bits which indicate the status of the device.
  // Optional
}

bool Sensor_DoTemperatureReading(int32_t *reading)
{
	int res;
	uint16_t addr = TEMP_SENSOR_I2C_ADDR;
	uint16_t regs[] = {TEMP_SENSOR_REG_TEMP_MSB, TEMP_SENSOR_REG_TEMP_LSB, TEMP_SENSOR_REG_TEMP_XLSB};
	uint16_t results[] = {0, 0, 0};
	uint8_t data;
	int dev = 0;
	int flags = 0;
	int size_regs = sizeof(regs) / sizeof(regs[0]);

	for (int i = 0; i < size_regs; i++) {
		res = i2c_read_reg(dev, addr, regs[i], &data, flags);
		results[i] = data;
	}

	*reading = results[0]*256*16 + results[1]*16 + results[2]/16;

	if (res != 0) {
		return false;
	}
	return true;
}

int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
	int32_t t_fine, var1, var2, T;
	var1 = ((((adc_T >> 3) - ((int32_t) dig_T1 << 1))) * ((int32_t) dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t) dig_T1)) * ((adc_T >> 4) - ((int32_t) dig_T1))) >> 12) * ((int32_t) dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}

bool Sensor_EnableSampling(void)
{
    int res;
    uint16_t addr = TEMP_SENSOR_I2C_ADDR;
    uint16_t reg = TEMP_SENSOR_REG_CTRL_MEAS;
    uint8_t mode = 35; //001 000 11
    int dev = 0;
    int flags = 0;

    res = i2c_write_reg(dev, addr, reg, mode, flags);

    if (res == 0) {
        printf("Success: i2c_%i wrote 1 byte and enabled sampling\n", dev);
        return true;
    }
    return false;
}

bool Sensor_LoadCalibrationData(void) {

	int res;
	uint16_t addr = TEMP_SENSOR_I2C_ADDR;
	uint16_t regs[] = {TEMP_SENSOR_REG_CAL_T1, TEMP_SENSOR_REG_CAL_T2, TEMP_SENSOR_REG_CAL_T3};
	uint16_t results[] = {0, 0, 0};
	uint16_t data;
	int dev = 0;
	int flags = 0;
	int len = 2;
	int size_regs = sizeof(regs) / sizeof(regs[0]);

	for (int i = 0; i < size_regs; i++) {
		res = i2c_read_regs(dev, addr, regs[i], &data, len, flags);
		results[i] = data;
	}

	if (res != 0) {
		return false;
	}

	dig_T1 = results[0];
	dig_T2 = results[1];
	dig_T3 = results[2];

	return true;
}

bool test(void) {

	int res;
	uint16_t addr = TEMP_SENSOR_I2C_ADDR;
	uint16_t reg = TEMP_SENSOR_REG_CAL_T1;
	uint16_t data;
	uint16_t t1;
	int dev = 0;
	int flags = 0;
	int len = 2;

	res = i2c_read_regs(dev, addr, reg, &data, len, flags);

	printf("t1: %i", data);
}

bool Sensor_Init(void)
{
  if ((TEMP_SENSOR_I2C_NUM < 0) || (TEMP_SENSOR_I2C_NUM >= (int)I2C_NUMOF)) {
    printf("I2C device with number \"%d\" not found\n", TEMP_SENSOR_I2C_NUM);
    return false;
  }
  i2cDevice = I2C_DEV(TEMP_SENSOR_I2C_NUM);
  i2c_acquire(i2cDevice);
  bool ret = Sensor_EnableSampling();
  ret |= Sensor_LoadCalibrationData();
  return ret;
}

void Sensor_Deinit(void)
{
  i2c_release(i2cDevice);
}

int Sensor_CmdHandler(int argc, char **argv)
{
  if (argc < 2)
  {
    goto usage;
  }
  if (strncmp(argv[1], "id", 16) == 0)
  {
    uint8_t id = 0xff;
    bool ret = Sensor_GetChipId(&id);
    printf("0x%x (%s) \n", id, (id == TEMP_SENSOR_CHIP_ID) ? "CORRECT" : "INCORRECT");

  }
  else if (strncmp(argv[1], "readreg", 16) == 0)
  {
    return cmd_i2c_read_reg(argc-1, argv);
  }
  else if (strncmp(argv[1], "writereg", 16) == 0)
  {
    return cmd_i2c_write_reg(argc, argv);
  }
  else if (strncmp(argv[1], "readregs", 16) == 0)
  {
    return cmd_i2c_read_regs(argc, argv);
  }
  else if (strncmp(argv[1], "sample", 16) == 0)
  {
    int32_t reading = 0;
    Sensor_DoTemperatureReading(&reading);
    printf("dig_T1 %d dig_T2 %d dig_T3 %d\n", dig_T1, dig_T2, dig_T3);
    printf("Reading %d (0x%x)\n", reading, reading);
    reading = bmp280_compensate_T_int32(reading);
	printf("Compensated Temperature: %d (0x%x)\n", reading, reading);
  }
  else if (strncmp(argv[1], "test", 16) == 0)
    {
    test();
    }
  return 0;

  usage:
  printf("Usage: sensor <id|readreg|readregs|writereg>\n");
  return 1;
}
SHELL_COMMAND(sensor, "Sensor cmd handler", Sensor_CmdHandler);
