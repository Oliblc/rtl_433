
    Decoder for 'FT1211R remote'.

    Copyright (C) 2026 Olivier Blanc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

*/

#include "decoder.h"

/**
Decoder for 'FT1211R remote'.

The device uses PWM encoding,
- 0 is encoded as 924 us pulse and 292 us gap,
- 1 is encoded as 312 us pulse and 908 us gap.

A transmission starts with a pulse of 328 us,
there a 5 repeated packets, each with a 6124 us gap.

Data layout:
    AAAAAAAAAAAAAAAAAAAABBBB8

- A: 20 bit Address / id
- B: 4-bit buttoncode
- 8: Always 8

view at https://triq.org/pdv/#AAB00B04010134039016AC27148255+AAB02304040134039016AC27148190818190818190908181818190909081818181818190818255+AAB02304010134039016AC27148190818190818190908181818190909081818181818190818355
Attempting demodulation... short_width: 312, long_width: 928, reset_limit: 5812, sync_width: 0
Use a flex decoder with -X 'n=name,m=OOK_PWM,s=312,l=928,r=5812,g=920,t=246,y=0'
[pulse_slicer_pwm] Analyzer Device
codes     : {1}8, {25}b678fd8, {25}b678fd8, {25}b678fd8, {25}b678fd8, {25}b678fd8

Example:
		rtl_433 -R 0 -X 'n=ft1211r,m=OOK_PWM,s=312,l=928,r=5812,g=920,t=246,y=0,match={20}0xb678f,rows=6,
		get=address:@0:{20};%x,get=command:@20:{4},get=msgcount:@24:{4}:%d,unique'
*/

static int ft1211r_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{

    if(bitbuffer->num_rows != 6)
        return DECODE_ABORT_LENGTH;

    int row = bitbuffer_find_repeated_row(bitbuffer, 3, 25);
    if (row < 0 || bitbuffer->bits_per_row[row] > 28 + 16) {
        return DECODE_ABORT_LENGTH;
    }
     uint8_t const preamble[] = {
            0xb6, 0x78, 0xf      // preamble
    };

    // Validate message and reject it as fast as possible : check for preamble
    unsigned start_pos = bitbuffer_search(bitbuffer, row, 0, preamble, sizeof(preamble) * 5);

    if (start_pos == bitbuffer->bits_per_row[row]) {
        return DECODE_ABORT_EARLY; // no preamble detected
    }

    uint8_t *b  = bitbuffer->bb[row];

    int address = (b[0] << 12) + (b[1] << 4) + (b[2] >> 4);    // @0 {20};
   int button  = b[2] & 0x0f; // @20 {4}
    char const *button_str;

    switch (button) {
    case 0xc:
        button_str = "FAN On/Off";
        break;
    case 0x2:
        button_str = "Light On/Off";
        break;
    case 0xa:
        button_str = "Speed 1";
        break;
    case 0x5:
        button_str = "Speed 2";
        break;
    case 0x9:
        button_str = "Speed 3";
        break;
    case 0x6:
        button_str = "Speed 4";
        break;
    case 0xd:
        button_str = "Speed 5";
        break;
    case 0x3:
        button_str = "Forward/Reverse";
        break;
    case 0xe:
        button_str = "1h";
        break;
    case 0xb:
        button_str = "4h";
        break;
    case 0x7:
        button_str = "8h";
        break;
    default:
        button_str = "Unknown";
        break;
    }
        decoder_log(decoder, 1, __func__, "fin");

    /* clang-format off */
    data_t *data = data_make(
            "model",        "",                 DATA_STRING, "FT1211R",
            "id",           "Transmitter ID",   DATA_INT,    address,
            "button",       "Button",           DATA_STRING, button_str,
            "button_code",  "Button Code",      DATA_INT,    button,
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "button",
        "button_code",
        NULL,
};

r_device const ft1211r = {
        .name        = "FT1211R remote",
        .modulation  = OOK_PULSE_PWM,
        .short_width = 312,
        .long_width  = 928,
        .gap_limit   = 920,
        .sync_width  = 0,
        .reset_limit = 5812,
        .decode_fn   = &ft1211r_decode,
        .disabled    = 0, // disabled and hidden, use 0 if there is a MIC, 1 otherwise
        .fields      = output_fields,
};
