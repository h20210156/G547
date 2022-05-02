Writing IIO driver for ADXL34
Summmary



Hardware Design
Schematic




![image](https://user-images.githubusercontent.com/99975058/166179958-523af4c3-549a-4c53-b326-de2ca9dadc97.png)
























Setup








Steps to get data from ADXL345 accelerometer in Raspberry Pi 

1.make

2.sudo modprobe  industrialio

3.sudo insmod adxl.ko

4.cd sys/bus/iio/devices/

5.cd  iio\:device0

6.cat in_accel_x_raw

7.cat in_accel_y_raw

8.cat in_accel_z_raw
