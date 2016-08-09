// Copyright (c) 2015-2016, XMOS Ltd, All rights reserved

#include <platform.h>
#include "dsp_qformat.h"
#include "dsp_math.h"
#include "dsp_filters.h"
#include "dsp_vector.h"
#include "dsp_statistics.h"
#include "dsp_adaptive.h"



int32_t dsp_adaptive_lms
(
    int32_t  source_sample,
    int32_t  reference_sample,
    int32_t* error_sample,
    int32_t* filter_coeffs,
    int32_t* state_data,
    int32_t  tap_count,
    int32_t  step_size,
    int32_t  q_format
) {
    int32_t output_sample, mu_err;
    
    // Output signal y[n] is computed via standard FIR filter:
    // y[n] = b[0] * x[n] + b[1] * x[n-1] + b[2] * x[n-2] + ...+ b[N-1] * x[n-N+1]

    output_sample = dsp_filters_fir( source_sample, filter_coeffs, state_data, tap_count, q_format );
    
    // Error equals difference between reference and filter output:
    // e[n] = d[n] - y[n]

    *error_sample = reference_sample - output_sample;
    
    // FIR filter coefficients b[k] are updated on a sample-by-sample basis:
    // b[k] = b[k] + mu_err * x[n-k] --- where mu_err = e[n] * step_size
    
    mu_err = dsp_math_multiply( *error_sample, step_size, q_format );
    dsp_vector_muls_addv( state_data, mu_err, filter_coeffs, filter_coeffs, tap_count, q_format );
        
    return output_sample;
}



int32_t dsp_adaptive_nlms
(
    int32_t  source_sample,
    int32_t  reference_sample,
    int32_t* error_sample,
    int32_t* filter_coeffs,
    int32_t* state_data,
    int32_t  tap_count,
    int32_t  step_size,
    int32_t  q_format
) {
    int32_t output_sample, energy, adjustment, ee, qq;
    
    // Output signal y[n] is computed via standard FIR filter:
    // y[n] = b[0] * x[n] + b[1] * x[n-1] + b[2] * x[n-2] + ...+ b[N-1] * x[n-N+1]

    output_sample = dsp_filters_fir( source_sample, filter_coeffs, state_data, tap_count, q_format );
    
    // Error equals difference between reference and filter output:
    // e[n] = d[n] - y[n]

    *error_sample = reference_sample - output_sample;
    
    // Compute the instantaneous enegry: E = x[n]^2 + x[n-1]^2 + ... + x[n-N+1]^2
    energy = dsp_vector_power( state_data, tap_count, q_format );
    //printf( "E = %08x %f\n", energy, F31(energy) );
    
    // adjustment = error * mu / energy
    
    // Adjust energy q_format to account for range of reciprocal
    for( qq = q_format, ee = energy; qq >= 0 && !(ee & 0x80000000); --qq ) ee <<= 1;
    energy = energy >> (q_format - qq);
    // Saturate the reciprocal value to max value for the given q_format
    if( energy < (1 << (31-(31-qq)*2)) ) energy = (1 << (31-(31-qq)*2)) + 0;

    energy = dsp_math_divide( (1 << qq), energy, qq );
    adjustment = dsp_math_multiply( *error_sample, step_size, q_format );
    adjustment = dsp_math_multiply( energy, adjustment, qq + q_format - q_format );
    
    // FIR filter coefficients b[k] are updated on a sample-by-sample basis:
    // b[k] = b[k] + mu_err * x[n-k] --- where mu_err = e[n] * step_size
    
    dsp_vector_muls_addv( state_data, adjustment, filter_coeffs, filter_coeffs, tap_count, q_format );
        
    return output_sample;
}
