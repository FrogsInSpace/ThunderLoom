#pragma once
#include <stdint.h>
#include "wif/wif.h"

typedef struct
{
    float uscale;
    float vscale;
    float umax;
    float psi;
    float alpha;
    float beta;
    float delta_x;
    float specular_strength; //TODO(Vidar): Does this really belong here?
    float intensity_fineness;
    // These are set by calling one of the wcWeavePatternFrom* below
    // after all parameters above have been defined
    uint32_t pattern_height;
    uint32_t pattern_width;
    PaletteEntry * pattern_entry;
    float specular_normalization;
} wcWeaveParameters;

typedef struct
{
    float color_r, color_g, color_b;
    float normal_x, normal_y, normal_z;
    float u, v; //Segment uv coordinates (in angles)
    float length, width; //Segment length and width
    float x, y; //position within segment. 
    float total_x, total_y; //index for elements. 
    bool warp_above; 
} wcPatternData;

typedef struct
{
    float uv_x, uv_y;
    float wi_x, wi_y, wi_z;
    float wo_x, wo_y, wo_z;
} wcIntersectionData;

void wcWeavePatternFromWIF(wcWeaveParameters *params, const char *filename);
void wcWeavePatternFromData(wcWeaveParameters *params, uint8_t *pattern,
    float *warp_color, float *weft_color, uint32_t pattern_width,
    uint32_t pattern_height);

wcPatternData wcGetPatternData(wcIntersectionData intersection_data,
    const wcWeaveParameters *params);

float wcEvalSpecular(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params);
