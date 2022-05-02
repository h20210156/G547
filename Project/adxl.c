#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

#define ADXL345_REG_DEVID		0x00
#define ADXL345_REG_POWER_CTL		0x2D
#define ADXL345_REG_DATA_FORMAT		0x31
#define ADXL345_REG_DATAX0		0x32
#define ADXL345_REG_DATAY0		0x34
#define ADXL345_REG_DATAZ0		0x36

#define ADXL345_POWER_CTL_MEASURE	BIT(3)
#define ADXL345_POWER_CTL_STANDBY	0x00

#define ADXL345_DATA_FORMAT_FULL_RES	BIT(3)
#define ADXL345_DATA_FORMAT_2G		0
#define ADXL345_DATA_FORMAT_4G		1
#define ADXL345_DATA_FORMAT_8G		2
#define ADXL345_DATA_FORMAT_16G		3

#define ADXL345_DEVID			0xE5

struct adxl345_data {
	struct i2c_client *client;
	u8 data_range;
};

#define ADXL345_CHANNEL(reg, axis) {					\
	.type = IIO_ACCEL,						\
	.modified = 1,							\
	.channel2 = IIO_MOD_##axis,					\
	.address = reg,							\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),			\
}

static const struct iio_chan_spec adxl345_channels[] = {
	ADXL345_CHANNEL(ADXL345_REG_DATAX0, X),
	ADXL345_CHANNEL(ADXL345_REG_DATAY0, Y),
	ADXL345_CHANNEL(ADXL345_REG_DATAZ0, Z),
};

static int adxl345_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	struct adxl345_data *data = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = i2c_smbus_read_word_data(data->client, chan->address);

		if (ret < 0)
			return ret;

		*val = sign_extend32(le16_to_cpu(ret), 12);
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static const struct iio_info adxl345_info = {
	//.driver_module	= THIS_MODULE,
	.read_raw	= adxl345_read_raw,
};

static int adxl345_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct adxl345_data *data;
	struct iio_dev *indio_dev;
	int ret;

	ret = i2c_smbus_read_byte_data(client, ADXL345_REG_DEVID);
	if (ret < 0) {
		dev_err(&client->dev, "Error reading device ID: %d\n", ret);
		return ret;
	}

	if (ret != ADXL345_DEVID) {
		dev_err(&client->dev, "Invalid device ID: %d\n", ret);
		return -ENODEV;
	}

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	data->client = client;
	/* Enable full-resolution mode */
	data->data_range = ADXL345_DATA_FORMAT_FULL_RES;

	ret = i2c_smbus_write_byte_data(data->client, ADXL345_REG_DATA_FORMAT,
					data->data_range);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to set data range: %d\n", ret);
		return ret;
	}

	indio_dev->dev.parent = &client->dev;
	indio_dev->name = id->name;
	indio_dev->info = &adxl345_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = adxl345_channels;
	indio_dev->num_channels = ARRAY_SIZE(adxl345_channels);

	/* Enable measurement mode */
	ret = i2c_smbus_write_byte_data(data->client, ADXL345_REG_POWER_CTL,
					ADXL345_POWER_CTL_MEASURE);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to enable measurement mode: %d\n",
			ret);
		return ret;
	}

	return iio_device_register(indio_dev);
}

static int adxl345_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct adxl345_data *data = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);

	return i2c_smbus_write_byte_data(data->client, ADXL345_REG_POWER_CTL,
					 ADXL345_POWER_CTL_STANDBY);
}

static const struct i2c_device_id adxl345_i2c_id[] = {
	{ "adxl345", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, adxl345_i2c_id);

static struct i2c_driver adxl345_driver = {
	.driver = {
		.name	= "adxl345",
	},
	.probe		= adxl345_probe,
	.remove		= adxl345_remove,
	.id_table	= adxl345_i2c_id,
};

module_i2c_driver(adxl345_driver);

MODULE_AUTHOR("Mark Ronald P <h20210156@pilani.bits-pilani.ac.in>");
MODULE_DESCRIPTION("ADXL345 Accelerometer driver");
MODULE_LICENSE("GPL v2");
