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
/**********************************************************************************************************************
* Copyright 2015 Google Inc. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
***********************************************************************************************************************/

/***********************************************************************************************************************
* File Name    : network_lora_adr.c
* Version      : 1.00
* Description  : Definitions of all functions
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 16.06.2017 1.00     First Release
***********************************************************************************************************************/

#include "stdlib.h"
#include "Typedef.h"
#include "math.h"
#include "layer_graph_lora_adr.h"
#define C_INT8 1
/***********************************************************************************************************************
* Function Name: innerproduct
* Description  : - Fully connected layer
*                - Performs dot product of data and weights and add them up with biases
*                   (Matrix Multiplication of data and weights and addition of biases)
* Arguments    : data           - Array of input data
*                weights        - Array of weights (transposed)
*                biases 		- Array of biases
*                scale          - Pointer to the multiplier value
*                shift          - Pointer to the shift value
*                offset         - Pointer to the zero point value  
*                out            - Placeholder for the output
*                shapes         - Dimensions of data and weights (N, D, F, D)
*                errorcode - errorcode if any issue
* Return Value : no return value
***********************************************************************************************************************/
void innerproduct(TsInt8 *data, const TsInt8 *weights, const TsInt32 *biases, const TsInt32 *scale, const TsInt8 *shift, const TsInt8 *offset,TsInt8 *out,TsInt *shapes,TsInt *errorcode){
    if (*errorcode!=1)
    {
        TsInt iColumn;
        TsInt iInneritr;
        TsInt D = shapes[1];
        TsInt F = shapes[3];

        TsInt8 in_offset = offset[0];       // input offset
        TsInt8 out_offset = offset[1];      // output offset
        
        // Execute C Innerproduct or CCRX complier or CCRL complier    
        TPrecision dSum = 0;
        TPrecision floor_val, diff;
        TPrecision right_shift;
        TsInt64 quantized_val, a_64, b_64, ab_64;
        TsInt32 nudge, ab_x2_high32;

        quantized_val = scale[0];

        right_shift = (TPrecision)(1 << shift[0]);

        for(iColumn=0; iColumn<F; iColumn++)
        {
            dSum = 0;
            for(iInneritr=0; iInneritr<D;iInneritr++)
            {
                dSum += (TPrecision)(data[iInneritr]- in_offset) * weights[(iInneritr*F)+iColumn];
            }
            if(biases)
            {
                dSum = dSum +(TPrecision)biases[iColumn];				// output
            }

            a_64 = (TsInt32)dSum;
            b_64 = (TsInt32)quantized_val;
            ab_64 = a_64 * b_64;
            
            if (ab_64 >= 0)
                nudge = 1073741824;//(1 << 30);
            else
                nudge = -1073741823;//(1 - (1 << 30));

            ab_x2_high32 = (TsInt32)((ab_64 + nudge) / 2147483648);//(1ll << 31);
            dSum = (TPrecision)(ab_x2_high32)/right_shift;

            floor_val =(TPrecision)(floor(dSum));
            diff = dSum - floor_val;
            if (diff < 0.5f)
            {
                dSum = floor_val;
            }
            else
            {
                dSum = floor_val + 1.0f;
            }
            dSum = (dSum + out_offset);
            if (dSum < -128)
            {
                out[iColumn] = -128;
            }
            else if(dSum > 127)
            {
                out[iColumn] = 127;      // output
            }
            else
            {
                out[iColumn] = (TsInt8)dSum;
            }
        }
    }
    
}

/***********************************************************************************************************************
* Function Name: softmax
* Description  : - Activation function
*                - Squashes an array of arbitrary real values to an array of real values 
*                  in the range(0, 1) that add up to 1	
* Arguments    : dData      - Array of input data
*                multiplier - Pointer to the scale multiplier value
*                offset     - Pointer to the zero point value  
*                dOut       - Pointer to the output data
*                iShapes	- Size of the input array
*                errorcode - errorcode if any issue
* Return Value : no return value
***********************************************************************************************************************/
void softmax(TsInt8 *dData, const TPrecision *multiplier, const TsInt8 *offset,TsInt8 *dOut,TsInt iShapes,TsInt *errorcode){

    if (*errorcode!=1)
    {
        TPrecision dMax, dvalue, dCal, floor_val, diff, dSum = 0;
        TsInt iRow;

        dMax = dData[0];
        for (iRow = 1; iRow < iShapes; iRow++)
        {
            if (dData[iRow] > dMax)
            {
                dMax = (dData[iRow]);
            }
        }
        for (iRow = 0; iRow < iShapes; iRow++)
        {
            dvalue = (((dData[iRow])) - dMax)*multiplier[0];
            dSum = (TPrecision)(dSum + exp(dvalue));
        }
        for (iRow = 0; iRow < iShapes; iRow++)
        {
            dCal = (dData[iRow] - dMax)*multiplier[0];
            dvalue =(TPrecision)(exp(dCal)/dSum);    
            dvalue = dvalue/multiplier[1];     
            floor_val = (TPrecision)(floor(dvalue));
            diff = dvalue - floor_val;
            if (diff < 0.5f)
            {
                dvalue = floor_val;
            }
            else
            {
                dvalue = floor_val + 1.0f;
            }
            dvalue = dvalue+offset[1];
            if (dvalue < -128)
            {
                dOut[iRow] = -128;
            }
            else if(dvalue > 127)
            {
                dOut[iRow] = 127;
            }
            else
            {
                dOut[iRow] =(TsInt8) dvalue;
            }
        }
    }
    

}



