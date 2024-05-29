#!/bin/sh

D_DEPTH_=$(grep D_DEPTH_= ../CFLAGS|cut -d'=' -f2)
D_COLOR_CHANNELS_=$(grep D_COLOR_CHANNELS_= ../CFLAGS|cut -d'=' -f2)
D_PPC_=$(grep D_PPC_= ../CFLAGS|cut -d'=' -f2)
D_MAX_ROWS_=$(grep D_MAX_ROWS_= ../CFLAGS|cut -d'=' -f2)
D_MAX_COLS_=$(grep D_MAX_COLS_= ../CFLAGS|cut -d'=' -f2)
D_MAX_STRIDE_=$(grep D_MAX_STRIDE_= ../CFLAGS|cut -d'=' -f2)
D_FP_T_=$(grep D_FP_T_= ../CFLAGS|cut -d'=' -f2)

D_ID_=$(cat ../ID)
D_TOP_=$(cat ../TOP)_Id${D_ID_}

CFLAGS=""
CFLAGS="$CFLAGS -DD_DEPTH_=$D_DEPTH_"
CFLAGS="$CFLAGS -DD_COLOR_CHANNELS_=$D_COLOR_CHANNELS_"
CFLAGS="$CFLAGS -DD_PPC_=$D_PPC_"
CFLAGS="$CFLAGS -DD_MAX_ROWS_=$D_MAX_ROWS_"
CFLAGS="$CFLAGS -DD_MAX_COLS_=$D_MAX_COLS_"
CFLAGS="$CFLAGS -DD_MAX_STRIDE_=$D_MAX_STRIDE_"
CFLAGS="$CFLAGS -DD_FP_T_=$D_FP_T_"
CFLAGS="$CFLAGS -DD_TOP_=$D_TOP_"
