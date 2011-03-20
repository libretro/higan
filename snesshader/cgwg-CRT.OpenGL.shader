<?xml version="1.0" encoding="UTF-8"?>
<!--
    cgwg's CRT shader

    Copyright (C) 2010 cgwg

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have their code distributed under the GPL in
    this message:

        http://board.byuu.org/viewtopic.php?p=26075#p26075

        "Feel free to distribute my shaders under the GPL. After all, the
        barrel distortion code was taken from the Curvature shader, which is
        under the GPL."
    )
    -->
<shader language="GLSL">
    <fragment><![CDATA[
        #version 130
        uniform sampler2D rubyTexture;
        uniform vec2 rubyInputSize;
        uniform vec2 rubyOutputSize;
        uniform vec2 rubyTextureSize;

        // Abbreviations
        #define TEX2D(c) texture2D(rubyTexture,(c))
        #define PI 3.141592653589

        // Adjusts the vertical position of scanlines. Useful if the output
        // pixel size is large compared to the scanline width (so, scale
        // factors less than 4x or so). Ranges from 0.0 to 1.0.
        #define phase 0.0

        // We are emulating the behaviour of a CRT whose phosphors apply
        // a gamma of 2.7.
        #define inputGamma 2.7

        // We render our output to be displayed on monitors that have
        // a standard sRGB gamma of 2.2.
        #define outputGamma 2.2

        // Controls the intensity of the barrel distortion used to emulate the
        // curvature of a CRT. 0.0 is perfectly flat, 1.0 is annoyingly
        // distorted, higher values are increasingly ridiculous.
        #define distortion 0.2

        // Apply barrel distortion to the given coordinate.
        vec2 barrelDistortion(vec2 coord) {
                vec2 cc = coord*rubyTextureSize/rubyInputSize - 0.5;
                float dist = dot(cc, cc);
                return coord + (cc * (dist + distortion * dist * dist) * distortion)*rubyInputSize/rubyTextureSize;
        }

        // Calculate the influence of a scanline on the current pixel.
        //
        // 'distance' is the distance in texture coordinates from the current
        // pixel to the scanline in question.
        // 'color' is the colour of the scanline at the horizontal location of
        // the current pixel.
        vec4 scanlineWeights(float distance, vec4 color)
        {
                // The "width" of the scanline beam is set as 2*(1 + x^4) for
                // each RGB channel.
                vec4 wid = 2.0 + 2.0 * pow(color, vec4(4.0));

                // The "weights" lines basically specify the formula that gives
                // you the profile of the beam, i.e. the intensity as
                // a function of distance from the vertical center of the
                // scanline. In this case, it is gaussian if width=2, and
                // becomes nongaussian for larger widths. Ideally this should
                // be normalized so that the integral across the beam is
                // independent of its width. That is, for a narrower beam
                // "weights" should have a higher peak at the center of the
                // scanline than for a wider beam.
                vec4 weights = vec4(distance/0.3);
                weights = 0.51*exp(-pow(weights*sqrt(2.0/wid),wid))/0.3/(0.6+0.2*wid);
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

                // The size of one texel, in texture-coordinates.
                vec2 one          = 1.0/rubyTextureSize;

                // Texture coordinates of the texel containing the active pixel
                vec2 xy = barrelDistortion(gl_TexCoord[0].xy);
                xy = xy + vec2(0.0 , -0.5 * (phase + (1.0-phase) * rubyInputSize.y/rubyOutputSize.y) * one.y);

                // Of all the pixels that are mapped onto the texel we are
                // currently rendering, which pixel are we currently rendering?
                vec2 uv_ratio     = fract(xy*rubyTextureSize);

                // Snap to the left-hand edge of the underlying texel.
                xy.x = floor(xy.x/one.x)*one.x;

                // col will store the effective color of the current scanline
                // at the horizontal location of the current output pixel; col2
                // stores the effective colour of the next scanline at the same
                // place.
                vec4 col, col2;

                // Calculate Lanczos scaling coefficients describing the effect
                // of various neighbour texels in a scanline on the current
                // pixel.
                vec4 coeffs = vec4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);
                coeffs = mix((sin(PI * coeffs) * sin(PI * coeffs / 2.0)) / (coeffs * coeffs), vec4(1.0), lessThan(abs(coeffs), vec4(0.01)));
                coeffs = coeffs / (coeffs.x+coeffs.y+coeffs.z+coeffs.w);

                // Calculate the effective colour of the current and next
                // scanlines at the horizontal location of the current pixel,
                // using the Lanczos coefficients above.
                col  = clamp(coeffs.x * TEX2D(xy + vec2(-one.x,0.0)) + coeffs.y * TEX2D(xy) + coeffs.z * TEX2D(xy + vec2(one.x, 0.0)) + coeffs.w * TEX2D(xy + vec2(2.0 * one.x, 0.0)),0.0,1.0);
                col2 = clamp(coeffs.x * TEX2D(xy + vec2(-one.x,one.y)) + coeffs.y * TEX2D(xy + vec2(0.0, one.y)) + coeffs.z * TEX2D(xy + one) + coeffs.w * TEX2D(xy + vec2(2.0 * one.x, one.y)),0.0,1.0);

                // Simulate the non-linearity of CRT phosphor before we
                // calculate how these colours interact.
                col = pow(col, vec4(inputGamma));
                col2 = pow(col2, vec4(inputGamma));

                // Calculate the influence of the current and next scanlines on
                // the current pixel.
                vec4 weights = scanlineWeights(uv_ratio.y, col);
                vec4 weights2 = scanlineWeights(1.0-uv_ratio.y, col2);
                vec3 mul_res = (col * weights + col2 * weights2).xyz;

                // mod_factor is the x-coordinate of the current output pixel.
                float mod_factor = gl_TexCoord[0].x*rubyOutputSize.x*rubyTextureSize.x/rubyInputSize.x;
                // dot-mask emulation:
                // Output pixels are alternately tinted green and magenta.
                vec3 dotMaskWeights = mix(
                        vec3(1.0, 0.7, 1.0),
                        vec3(0.7, 1.0, 0.7),
                        floor(mod(mod_factor, 2.0))
                    );
                mul_res = dotMaskWeights * mul_res;

                // Convert the image gamma for display on our output device.
                mul_res = pow(mul_res, vec3(1.0/outputGamma));

                gl_FragColor = vec4(mul_res, 1.0);
        }
    ]]></fragment>
</shader>
