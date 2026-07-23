/** @file
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
- 0 is encoded as 924 us pulse and 252 us gap,
- 1 is encoded as 312 us pulse and 756 us gap.

A transmission starts with a pulse of 0 us,
there a 5 repeated packets, each with a 5812 us gap.

Data layout:
    AAAAAAAAAAAAAAAAAAAABBBB8

- A: 20 bit Address / id
- B: 4-bit buttoncode
- 8: Always 8

Example:
		rtl_433 -R 0 -X 'n=name,m=OOK_PWM,s=312,l=924,r=5808,g=920,t=245,y=0,match={20}0xb678f,rows=6,
		get=address:@0:{20};%x,get=command:@20:{4},get=msgcount:@24:{4}:%d,unique'

*/

static int ft1211r(r_device *decoder, bitbuffer_t *bitbuffer)
{
    int row = bitbuffer_find_repeated_row(bitbuffer, 5, 28);
    if (row < 0) {
        return DECODE_ABORT_LENGTH;
    }

    uint8_t *b  = bitbuffer->bb[row];

    if (b[0] != 0xb678f) {
        return DECODE_ABORT_EARLY; // Messages start of 0xAA not found
    }

    int address = (b[0] << 16) + (b[1] << 12) + (b[2] << 8) + (b[3] << 4) + b[4];    // @0 {20};
    int button  = b[5]; // @20 {4}
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
        .short_width = 256,
        .long_width  = 756,
        .gap_limit   = 5812,
        .sync_width  = 0,
        .reset_limit = 8800,
        .decode_fn   = &ft1211r,
        .disabled    = 0, // disabled and hidden, use 0 if there is a MIC, 1 otherwise
        .fields      = output_fields,
};
