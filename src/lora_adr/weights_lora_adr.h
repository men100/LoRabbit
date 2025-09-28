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
extern const TPrecision ip_op_scale[];
extern const TsInt8 ip_op_zero_point[];
// Weights and bias tensor values including zero point and multiplier values for required layers
extern const TsInt32 sequential_1_dense_1_MatMul_multiplier[];
extern const TsInt8 sequential_1_dense_1_MatMul_shift[];
extern const TsInt8 sequential_1_dense_1_MatMul_offset[];
extern const TsInt8 sequential_1_dense_1_MatMul_weights[];
extern const TsInt32 sequential_1_dense_1_MatMul_biases[];
extern const TsInt32 sequential_1_dense_1_2_MatMul_multiplier[];
extern const TsInt8 sequential_1_dense_1_2_MatMul_shift[];
extern const TsInt8 sequential_1_dense_1_2_MatMul_offset[];
extern const TsInt8 sequential_1_dense_1_2_MatMul_weights[];
extern const TsInt32 sequential_1_dense_1_2_MatMul_biases[];
extern const TsInt32 sequential_1_dense_2_1_MatMul_multiplier[];
extern const TsInt8 sequential_1_dense_2_1_MatMul_shift[];
extern const TsInt8 sequential_1_dense_2_1_MatMul_offset[];
extern const TsInt8 sequential_1_dense_2_1_MatMul_weights[];
extern const TsInt32 sequential_1_dense_2_1_MatMul_biases[];
extern const TPrecision StatefulPartitionedCall_1_0_multiplier[];
extern const TsInt8 StatefulPartitionedCall_1_0_offset[];

#endif
