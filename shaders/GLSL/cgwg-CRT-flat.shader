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
        uniform sampler2D rubyTexture;
        uniform vec2 rubyInputSize;
        uniform vec2 rubyOutputSize;
        uniform vec2 rubyTextureSize;

        #define TEX2D(c) texture2D(rubyTexture,(c))
        #define PI 3.141592653589
        #define phase 0.0
        #define gamma 2.7

        void main()
        {
                vec2 xy = gl_TexCoord[0].xy;

                vec2 one          = 1.0/rubyTextureSize;
                xy = xy + vec2(0.0 , -0.5 * (phase + (1.0-phase) * rubyInputSize.y/rubyOutputSize.y) * one.y);

                vec2 uv_ratio     = fract(xy*rubyTextureSize);
                xy.x = floor(xy.x/one.x)*one.x;

                vec4 col, col2;

                vec4 coeffs = vec4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);
                coeffs = mix((sin(PI * coeffs) * sin(PI * coeffs / 2.0)) / (coeffs * coeffs), vec4(1.0), lessThan(abs(coeffs), vec4(0.01)));
                coeffs = coeffs / (coeffs.x+coeffs.y+coeffs.z+coeffs.w);

                col  = clamp(coeffs.x * TEX2D(xy + vec2(-one.x,0.0)) + coeffs.y * TEX2D(xy) + coeffs.z * TEX2D(xy + vec2(one.x, 0.0)) + coeffs.w * TEX2D(xy + vec2(2.0 * one.x, 0.0)),0.0,1.0);
                col2 = clamp(coeffs.x * TEX2D(xy + vec2(-one.x,one.y)) + coeffs.y * TEX2D(xy + vec2(0.0, one.y)) + coeffs.z * TEX2D(xy + one) + coeffs.w * TEX2D(xy + vec2(2.0 * one.x, one.y)),0.0,1.0);
                col = pow(col, vec4(gamma));
                col2 = pow(col2, vec4(gamma));

                vec4 wid = 2.0 + 2.0 * pow(col, vec4(4.0));
                vec4 weights = vec4(uv_ratio.y/0.3);
                weights = 0.51*exp(-pow(weights*sqrt(2.0/wid),wid))/0.3/(0.6+0.2*wid);
                wid = 2.0 + 2.0 * pow(col2,vec4(4.0));
                vec4 weights2 = vec4((1.0-uv_ratio.y)/0.3);
                weights2 = 0.51*exp(-pow(weights2*sqrt(2.0/wid),wid))/0.3/(0.6+0.2*wid);

                vec4 mcol = vec4(1.0);
                if ( mod(gl_TexCoord[0].x*rubyOutputSize.x*rubyTextureSize.x/rubyInputSize.x,2.0) < 1.0)
                  mcol.g = 0.7;
                else
                  mcol.rb = vec2(0.7);

                gl_FragColor = pow(mcol*(col * weights + col2 * weights2), vec4(1.0/2.2));
        }
    ]]></fragment>
</shader>
