#include "Typedef.h"

typedef struct 
{   const TsInt8 *input_weight;
	const TsInt8 *forgot_weight;
	const TsInt8 *cellstate_weight;
	const TsInt8 *output_weight;
}unidirectional_LSTM_weights;

typedef struct 
{   const TsInt32 *input_bias;
	const TsInt32 *forgot_bias;
	const TsInt32 *cellstate_bias;
	const TsInt32 *output_bias;
}unidirectional_LSTM_bias;

typedef struct 
{   const TPrecision *input_weight;
	const TPrecision *forgot_weight;
	const TPrecision *cellstate_weight;
	const TPrecision *output_weight;
}LSTM_weights;

typedef struct 
{   const TPrecision *input_bias;
	const TPrecision *forgot_bias;
	const TPrecision *cellstate_bias;
	const TPrecision *output_bias;
}LSTM_bias;

typedef struct 
{   const TPrecision *input_weight;
	const TPrecision *forgot_weight;
	const TPrecision *cellstate_weight;
}GRU_weights;

typedef struct 
{   const TPrecision *input_bias;
	const TPrecision *forgot_bias;
	const TPrecision *cellstate_bias;
}GRU_bias;