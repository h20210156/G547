Writing IIO driver for ADXL345

Summmary

Using IIO subsystems, I2C interfacing of ADXL345 is done. At probe function, memory allocation for IIO device is done and IIO device registration is done. At remove function, IIO device is unregistered and memory is freed. ADXL345 has 3 channels ie x,y,z axis data. The axis data is read from the register addresses 0x32, 0x34, 0x36 using read raw function. Register addresses are available in ADXL345 datasheet.


Hardware Design
Schematic




![image](https://user-images.githubusercontent.com/99975058/166179958-523af4c3-549a-4c53-b326-de2ca9dadc97.png)
























Setup



![1651461371775](https://user-images.githubusercontent.com/99975058/166180312-64189e07-7b4d-40e9-ae55-33113b32811d.jpg)





Steps to get data from ADXL345 accelerometer in Raspberry Pi 

1.make

2.sudo modprobe  industrialio

3.sudo insmod adxl.ko

4.cd sys/bus/iio/devices/

5.cd  iio\:device0

6.cat in_accel_x_raw

7.cat in_accel_y_raw

8.cat in_accel_z_raw
