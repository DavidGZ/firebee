#set disassemble-next-line on
define tr
	target remote | m68k-bdm-gdbserver pipe /dev/bdmcf3
	monitor bdm-reset
end
define tbtr
	target remote | m68k-bdm-gdbserver pipe /dev/tblcf3
end
source mcf5474.gdb

