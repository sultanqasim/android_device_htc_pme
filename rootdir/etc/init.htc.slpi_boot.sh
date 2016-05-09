#!/system/bin/sh
echo 1 > /sys/kernel/boot_slpi/boot
sleep 0.5
echo fpc1020_reset_active > /sys/devices/soc/soc:fp_fpc1155/pinctl_set

