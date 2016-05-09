#!/system/bin/sh
# Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Code Aurora nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#set diag permissions
    chown radio.qcom_diag /dev/htc_diag
    chown radio.qcom_diag /dev/diag
    chown radio.qcom_diag /dev/diag_mdm
    chown radio.qcom_diag /dev/htcdiag
    chown radio.qcom_diag /dev/diag_arm9
    chown radio.qcom_diag /dev/btdiag
    chown radio.qcom_diag /dev/diag_qsc
	chown system.system /sys/devices/platform/android_usb/lock_speed
    chmod 0660 /dev/htc_diag
    chmod 0660 /dev/diag
    chmod 0660 /dev/diag_mdm
    chmod 0660 /dev/htcdiag
    chmod 0660 /dev/diag_arm9
    chmod 0660 /dev/btdiag
    chmod 0660 /dev/diag_qsc

# soc_ids for 8916/8939 differentiation
if [ -f /sys/devices/soc0/soc_id ]; then
    soc_id=`cat /sys/devices/soc0/soc_id`
else
    soc_id=`cat /sys/devices/system/soc/soc0/id`
fi

# enable rps cpus on msm8939/msm8909/msm8929 target
setprop sys.usb.rps_mask 0
case "$soc_id" in
    "239" | "241" | "263" | "268" | "269" | "270")
        setprop sys.usb.rps_mask 10
    ;;
    "245" | "258" | "259" | "265" | "275")
        setprop sys.usb.rps_mask 4
    ;;
esac

# just for msm8916
case "$soc_id" in
	"206")
		echo 0 > /sys/module/g_android/parameters/u_ether_rx_pending_thld
	;;
esac
