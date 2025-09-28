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
* File Name    : dnn_compute_lora_adr.c
* Version      : 1.00
* Description  : The function calls
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 16.06.2017 1.00     First Release
***********************************************************************************************************************/

 
#include "layer_shapes_lora_adr.h"
#include "layer_graph_lora_adr.h"
#include "weights_lora_adr.h"
 
TsOUT* dnn_compute_lora_adr(TsIN* serving_default_keras_tensor_3_0, TsInt *errorcode)
{
  *errorcode = 0;
   
  innerproduct(serving_default_keras_tensor_3_0,sequential_1_1_dense_2_1_MatMul_weights,sequential_1_1_dense_2_1_MatMul_biases,sequential_1_1_dense_2_1_MatMul_multiplier,sequential_1_1_dense_2_1_MatMul_shift,sequential_1_1_dense_2_1_MatMul_offset,sequential_1_1_dense_2_1_MatMul,layer_shapes_lora_adr.sequential_1_1_dense_2_1_MatMul_shape,errorcode);
   
  innerproduct(sequential_1_1_dense_2_1_MatMul,sequential_1_1_dense_3_1_MatMul_weights,sequential_1_1_dense_3_1_MatMul_biases,sequential_1_1_dense_3_1_MatMul_multiplier,sequential_1_1_dense_3_1_MatMul_shift,sequential_1_1_dense_3_1_MatMul_offset,sequential_1_1_dense_3_1_MatMul,layer_shapes_lora_adr.sequential_1_1_dense_3_1_MatMul_shape,errorcode);
  softmax(sequential_1_1_dense_3_1_MatMul,StatefulPartitionedCall_1_0_multiplier,StatefulPartitionedCall_1_0_offset,StatefulPartitionedCall_1_0,layer_shapes_lora_adr.StatefulPartitionedCall_1_0_shape,errorcode);
  return(StatefulPartitionedCall_1_0);
}
