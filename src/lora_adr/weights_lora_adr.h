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
* File Name    : weights_lora_adr.h
* Version      : 1.00
* Description  : Weights and biases of the layers
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 16.06.2017 1.00     First Release
***********************************************************************************************************************/

#ifndef WEIGHTS_LORA_ADR_H_
#define WEIGHTS_LORA_ADR_H_
// Input, Output scale and zero point values of the model
const TPrecision ip_op_scale[] = {0.364705890417099,0.00390625};
const TsInt8 ip_op_zero_point[] = {124,-128};
// Weights and bias tensor values including zero point and multiplier values for required layers
const TsInt32 sequential_1_1_dense_2_1_MatMul_multiplier[] = {1773113165};
const TsInt8 sequential_1_1_dense_2_1_MatMul_shift[] = {7};
const TsInt8 sequential_1_1_dense_2_1_MatMul_offset[] = {124,-128};
const TsInt8 sequential_1_1_dense_2_1_MatMul_weights[] = {4,-1,127,54,127,64,-58,-52,-127,-127,-10,127,105,-127,127,127};
const TsInt32 sequential_1_1_dense_2_1_MatMul_biases[] = {-195,0,0,0,0,0,-97,119};
const TsInt32 sequential_1_1_dense_3_1_MatMul_multiplier[] = {1201562361};
const TsInt8 sequential_1_1_dense_3_1_MatMul_shift[] = {6};
const TsInt8 sequential_1_1_dense_3_1_MatMul_offset[] = {-128,127};
const TsInt8 sequential_1_1_dense_3_1_MatMul_weights[] = {51,9,72,127,33,-29,-42,-13,96,-127,127,1,118,-11,50,17,-95,-21,77,-75,-13,25,28,58,-127,-105,7,-43,53,19,-72,-20};
const TsInt32 sequential_1_1_dense_3_1_MatMul_biases[] = {404,440,-260,-183};
const TPrecision StatefulPartitionedCall_1_0_multiplier[] = {0.038429293781518936,0.00390625};
const TsInt8 StatefulPartitionedCall_1_0_offset[] = {127,-128};

#endif