	.text
	.align	2
_start:
	li	sp,65536
	jal	main
_hw_shutdown:
	jal	_hw_shutdown
