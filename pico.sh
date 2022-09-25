#!/bin/sh

ARG="$1"

openocd_command() {
	openocd -f debug/interface/ft232h-module-swd.cfg -f debug/target/rp2040.cfg -c "$1"
}


case $ARG in
	"reset")
		openocd_command "init; reset halt; rp2040.core1 arp_reset assert 0; rp2040.core0 arp_reset assert 0; exit"
		;;
	"flash")
		openocd_command "program build/vgmplayer.elf verify reset exit"
		;;
esac
