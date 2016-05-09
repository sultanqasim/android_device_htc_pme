#!/system/bin/sh

resetreason=`getprop ro.boot.resetreason`

dst_folder="/data/misc/radio/minidump"
src_file="/dev/block/bootdevice/by-name/ramdump"

rm -rf $dst_folder

if [ -e $src_file ] && [ "$resetreason" == "exception" ]; then
	mkdir $dst_folder
	chown radio.radio $dst_folder
	chmod 0700 $dst_folder
	cd $dst_folder
	parse_minidump $src_file > PARSE_LOG.BIN
	rm HB_LAST.BIN
	rm HB_LOG.BIN
	rm RAMOOPS.BIN
	chown radio.radio "*.BIN"
	chmod 0400 "*.BIN"
fi
