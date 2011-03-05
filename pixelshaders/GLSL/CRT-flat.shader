<?xml version="1.0" encoding="UTF-8"?>
<!--
    Flat CRT shader

    Copyright (C) 2010, 2011 cgwg and Themaister

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have the original version of this shader
    distributed under the GPL in this message:

        http://board.byuu.org/viewtopic.php?p=26075#p26075

        "Feel free to distribute my shaders under the GPL. After all, the
        barrel distortion code was taken from the Curvature shader, which is
        under the GPL."
    )
    -->
<shader language="GLSL">
    <vertex><![CDATA[
        uniform vec2 rubyInputSize;
        uniform vec2 rubyOutputSize;
        uniform vec2 rubyTextureSize;

        varying vec2 one;
        varying vec2 c01, c11, c21, c31;
        varying vec2 c02, c12, c22, c32;
        varying vec2 ratio_scale;
        varying float mod_factor;

        void main()
        {
            // Do the standard vertex processing
            gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

            // Precalculate a bunch of useful values we'll need in the fragment
            // shader.

            // The size of one texel, in texture-coordinates
            one = 0.99 / rubyTextureSize; // avoid float rounding errors

            // Texture coordinates of the texel we're drawing, and some
            // neighbours.
            c11 = gl_MultiTexCoord0.xy;
            c01 = c11 + vec2(-one.x, 0.0);
            c21 = c11 + vec2(one.x, 0.0);
            c31 = c11 + vec2(2.0 * one.x, 0.0);

            c02 = c11 + vec2(-one.x, one.y);
            c12 = c11 + vec2(0.0, one.y);
            c22 = c11 + vec2(one.x, one.y);
            c32 = c11 + vec2(2.0 * one.x, one.y);

            // Texel coordinates of the texel we're drawing.
            ratio_scale = c11 * rubyTextureSize;

            // Resulting X pixel-coordinate of the pixel we're drawing.
            mod_factor =
                    c11.x * rubyOutputSize.x * rubyTextureSize.x
                    / rubyInputSize.x
                ;
        }
]]></vertex>
    <fragment><![CDATA[
        uniform sampler2D rubyTexture;
        uniform vec2 rubyInputSize;
        uniform vec2 rubyOutputSize;
        uniform vec2 rubyTextureSize;

        varying vec2 one;
        varying vec2 c01, c11, c21, c31;
        varying vec2 c02, c12, c22, c32;
        varying vec2 ratio_scale;
        varying float mod_factor;

        #define TEX2D(c) texture2D(rubyTexture,(c))
        #define PI 3.141592653589
        #define gamma 2.7

        // Returns a vec4 whose elements are 1.0 if the corresponding element
        // in data is less than the corresponding element in condition.
        vec4 less_than(vec4 data, vec4 condition)
        {
            vec4 ret = vec4(1.0) + condition - data;
            return clamp(floor(ret), 0.0, 1.0);
        }

        void main()
        {     
            // Of all the pixels that are mapped onto the texel we are
            // currently rendering, which pixel are we currently rendering?
            vec2 uv_ratio = fract(ratio_scale);

            // Color of this line, color of the line below.
            vec3 col, col2;

            // Create a matrix from the colours of the texels on the current
            // scanline.
            mat4 texes0 = mat4 (
                    TEX2D(c01),
                    TEX2D(c11),
                    TEX2D(c21),
                    TEX2D(c31)
                );

            // Create a matrix from the colours of the texels on the following
            // scanline.
            mat4 texes1 = mat4 (
                    TEX2D(c02),
                    TEX2D(c12),
                    TEX2D(c22),
                    TEX2D(c32)
                );

            vec4 coeffs = vec4(
                    1.0 + uv_ratio.x,
                          uv_ratio.x,
                    1.0 - uv_ratio.x,
                    2.0 - uv_ratio.x
                );
            coeffs = mix(
                    (sin(PI*coeffs) * sin(PI*coeffs*0.5)) / (coeffs*coeffs),
                    vec4(1.0),
                    less_than(abs(coeffs), vec4(0.01))
                ) / dot(coeffs, vec4(1.0));

            col = clamp(texes0 * coeffs, 0.0, 1.0).xyz;
            col2 = clamp(texes1 * coeffs, 0.0, 1.0).xyz;

            col = pow(col, vec3(gamma));
            col2 = pow(col2, vec3(gamma));

            vec3 wid = 2.0 * (1.0 + pow(col, vec3(4.0)));
            vec3 weights = vec3(uv_ratio.y * 3.33333);
            weights = 1.7 * (exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid));

            wid = 2.0 + (2.0 * pow(col2, vec3(4.0)));
            vec3 weights2 = vec3(3.3333 - (uv_ratio.y * 3.33333));
            weights2 = 1.7 * (exp(-pow(weights2 * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid));

            // dot-mask emulation:
            // Output pixels are alternately tinted green and magenta. 
            vec3 dotMaskWeights = mix(
                    vec3(1.0, 0.7, 1.0),
                    vec3(0.7, 1.0, 0.7),
                    floor(mod(mod_factor, 2.0))
                );

            vec3 mul_res = col * weights + col2 * weights2;
            gl_FragColor = vec4(pow(dotMaskWeights * mul_res,
                    vec3(1.0/2.2)), 1.0);
        }
    ]]></fragment>
</shader>
