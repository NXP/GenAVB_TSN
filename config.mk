-include local_config.mk

target_list?= linux_imx6

CFLAGS:=-O2 -Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement
release_mode:=2
