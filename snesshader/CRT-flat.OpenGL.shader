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

        // Define some calculations that will be used in fragment shader.
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
            one = 1.0 / rubyTextureSize;

            // Texture coordinates of the texel we're drawing, and some
            // neighbours.
            // Since the texture coordinates can be linearily interpolated for
            // each fragment, we gain performance.
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

        // Abbreviations
        #define TEX2D(c) texture2D(rubyTexture,(c))
        #define PI 3.141592653589

        // We are emulating the behaviour of a CRT whose phosphors apply
        // a gamma of 2.7.
        #define inputGamma 2.7

        // We render our output to be displayed on monitors that have
        // a standard sRGB gamma of 2.2.
        #define outputGamma 2.2

        // Calculate the influence of a scanline on the current pixel.
        //
        // 'distance' is the distance in texture coordinates from the current
        // pixel to the scanline in question.
        // 'color' is the colour of the scanline at the horizontal location of
        // the current pixel.
        vec3 scanlineWeights(float distance, vec3 color)
        {
            // Compared to cgwg's original code, this attempts to avoid as many
            // divisions as possible.
            // Mostly rearranged constants here to make sure as few divisions
            // as possible take place.

            // The "width" of the scanline beam is set as 2*(1 + x^4) for
            // each RGB channel.
            // Usually multiply-add would be faster, but wasn't here for some
            // reason.
            vec3 wid = 2.0 * (1.0 + pow(color, vec3(4.0)));

            // The "weights" lines basically specify the formula that gives
            // you the profile of the beam, i.e. the intensity as
            // a function of distance from the vertical center of the
            // scanline. In this case, it is gaussian if width=2, and
            // becomes nongaussian for larger widths. Ideally this should
            // be normalized so that the integral across the beam is
            // independent of its width. That is, for a narrower beam
            // "weights" should have a higher peak at the center of the
            // scanline than for a wider beam.
            vec3 weights = vec3(distance * 3.33333);

            // Inverse square root seems to be faster than regular sqrt.
            // Probably due to the fact that this operation is very common for
            // normalizing vectors. (DOTP, RSQRT, MUL)
            weights = 1.7 * (exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid));

            return weights;
        }

        void main()
        {     
            // Here's a helpful diagram to keep in mind while trying to
            // understand the code:
            //
            //  |      |      |      |      |
            // -------------------------------
            //  |      |      |      |      |
            //  |  01  |  11  |  21  |  31  | <-- current scanline
            //  |      | @    |      |      |
            // -------------------------------
            //  |      |      |      |      |
            //  |  02  |  12  |  22  |  32  | <-- next scanline
            //  |      |      |      |      |
            // -------------------------------
            //  |      |      |      |      |
            //
            // Each character-cell represents a pixel on the output
            // surface, "@" represents the current pixel (always somewhere
            // in the bottom half of the current scan-line, or the top-half
            // of the next scanline). The grid of lines represents the
            // edges of the texels of the underlying texture.

            // Of all the pixels that are mapped onto the texel we are
            // currently rendering, which pixel are we currently rendering?
            vec2 uv_ratio = fract(ratio_scale);

            // Color of this line, color of the line below.
            vec3 col, col2;

            // Create a matrix from the colours of the texels on the current
            // scanline. This is done to optimize the operation performed in
            // original shader.
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

            // Create a coefficient vector based on the position of our pixel.
            // Add a small delta to avoid running into awkward divide by 0.0
            // situations.
            vec4 coeffs = vec4(
                    1.0 + uv_ratio.x,
                          uv_ratio.x,
                    1.0 - uv_ratio.x,
                    2.0 - uv_ratio.x
                ) + 0.01;

            // Calculate Lanczos scaling coefficients describing the effect
            // of various neighbour texels in a scanline on the current
            // pixel.
            coeffs = (sin(PI*coeffs) * sin(PI*coeffs*0.5)) / (coeffs*coeffs);
            coeffs = coeffs / dot(coeffs, vec4(1.0));

            // Calculate the effective color of the current and next
            // scanlines at the horizontal location of the current pixel,
            // using the Lanczos coefficients above. This is a matrix multiply
            // performing texes[0] * coeffs.x + texes[1] * coeffs.y + ...
            // A reasonable optimization.
            col = clamp(texes0 * coeffs, 0.0, 1.0).xyz;
            col2 = clamp(texes1 * coeffs, 0.0, 1.0).xyz;

            // Simulate the non-linearity of CRT phosphor before we
            // calculate how these colours interact.
            col = pow(col, vec3(inputGamma));
            col2 = pow(col2, vec3(inputGamma));

            // Calculate the influence of the current and next scanlines on
            // the current pixel.
            vec3 weights = scanlineWeights(uv_ratio.y, col);
            vec3 weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);
            vec3 mul_res = col * weights + col2 * weights2;

            // dot-mask emulation:
            // Output pixels are alternately tinted green and magenta. 
            vec3 dotMaskWeights = mix(
                    vec3(1.0, 0.7, 1.0),
                    vec3(0.7, 1.0, 0.7),
                    floor(mod(mod_factor, 2.0))
                );
            mul_res = dotMaskWeights * mul_res;

            // Convert the image gamma for display on our output device.
            gl_FragColor = vec4(pow(mul_res, vec3(1.0/outputGamma)), 1.0);
        }
    ]]></fragment>
</shader>
