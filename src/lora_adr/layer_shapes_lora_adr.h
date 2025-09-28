/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No 
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all 
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED 'AS IS' AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES 
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS 
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of 
* this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer 
*
* Changed from original python code to C source code.
* Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : layer_shapes_lora_adr.h
* Version      : 1.00
* Description  : Initializations
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 16.06.2017 1.00     First Release
***********************************************************************************************************************/

#include "Typedef.h"
#include <stdlib.h>
#ifndef LAYER_SHAPES_LORA_ADR_H_
#define LAYER_SHAPES_LORA_ADR_H_
 
TsOUT* dnn_compute_lora_adr(TsIN*, TsInt*);
 
TsInt8 sequential_1_1_dense_2_1_MatMul[8];
TsInt8 sequential_1_1_dense_2_1_MatMul[8];
TsInt8 sequential_1_1_dense_3_1_MatMul[4];
TsInt8 sequential_1_1_dense_3_1_MatMul[4];
TsInt8 StatefulPartitionedCall_1_0[4];
TsInt8 StatefulPartitionedCall_1_0[4];
 
struct shapes_lora_adr{
    TsInt sequential_1_1_dense_2_1_MatMul_shape[4];
    TsInt sequential_1_1_dense_3_1_MatMul_shape[4];
    TsInt StatefulPartitionedCall_1_0_shape;
};
 
struct shapes_lora_adr layer_shapes_lora_adr ={
    {1,2,2,8},
    {1,8,8,4},
    4
};
 
#endif
