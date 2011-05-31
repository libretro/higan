<?xml version="1.0" encoding="UTF-8"?>
<!--
    caligari's scanlines

    Copyright (C) 2011 caligari

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (caligari gave their consent to have this shader distributed under the GPL
    in this message:

        http://board.byuu.org/viewtopic.php?p=36219#p36219

        "As I said to Hyllian by PM, I'm fine with the GPL (not really a bi
        deal...)"
   )
    -->
<shader language="GLSL">
    <fragment><![CDATA[
        uniform sampler2D rubyTexture;
        uniform vec2 rubyInputSize;
        uniform vec2 rubyTextureSize;
        uniform vec2 rubyOutputSize;

	// Uncomment to use method 1 for simulating RGB triads.
        // #define TRIAD1

	// Uncomment to use method 2 for simulating RGB triads.
        // #define TRIAD2

        // Enable screen curvature.
	// #define CURVATURE

        // Controls the intensity of the barrel distortion used to emulate the
        // curvature of a CRT. 0.0 is perfectly flat, 1.0 is annoyingly
        // distorted, higher values are increasingly ridiculous.
        #define distortion 0.2

        // Uncomment to use neighbours from previous and next scanlines
        #define USE_ALL_NEIGHBOURS

        // 0.5 = same width as original pixel, 1.0-1.2 gives nice overlap
        #define SPOT_WIDTH      1.2

        // Shape of the spots: 1.0 = circle, 4.0 = ellipse with 2:1 aspect
        #define X_SIZE_ADJUST   2.0

        // To increase bloom / luminosity, decrease this parameter
        #define FACTOR_ADJUST   2.0

	// Defines the coarseness of the spots. Should be set to at least the
	// scale multiplier the output will be rendered at, no visible effect
	// beyond that point.
        #define SCALE   10.0

        // Apply radial distortion to the given coordinate.
        vec2 radialDistortion(vec2 coord)
        {
                coord *= rubyTextureSize / rubyInputSize;
                vec2 cc = coord - 0.5;
                float dist = dot(cc, cc) * distortion;
                return (coord + cc * (1.0 + dist) * dist) * rubyInputSize / rubyTextureSize;
        }

        #ifdef CURVATURE
        #	define TEXCOORDS       radialDistortion(gl_TexCoord[0].xy)
        #else
        #	define TEXCOORDS       gl_TexCoord[0].xy
        #endif // CURVATURE

        // Constants
        vec4 luminosity_weights = vec4(0.2126, 0.7152, 0.0722, 0.0);
        vec2 onex = vec2(1.0 / rubyTextureSize.x, 0.0);

        #ifdef USE_ALL_NEIGHBOURS
        vec2 oney = vec2(0.0, 1.0 / rubyTextureSize.y);
	#endif // USE_ALL_NEIGHBOURS

        float factor(float lumi, vec2 dxy)
        {
		float dist = sqrt(dxy.x*dxy.x + dxy.y*dxy.y * X_SIZE_ADJUST);

		return
			(2.0 + lumi)
			* (1.0 - smoothstep(0.0, SPOT_WIDTH, dist/SCALE))
			/ FACTOR_ADJUST;
        }

	void main(void)
	{
		vec2 coords_scaled = floor(TEXCOORDS * rubyTextureSize * SCALE);
		vec2 coords_snes = floor(coords_scaled / SCALE);
		vec2 coords_texture = (coords_snes + vec2(0.5))
			/ rubyTextureSize;

		vec2 ecart = coords_scaled - (SCALE * coords_snes
			+ vec2(SCALE * 0.5 - 0.5));

		vec4 color = texture2D(rubyTexture, coords_texture);
		float luminosity = dot(color, luminosity_weights);

		color *= factor(luminosity, ecart);

		// RIGHT NEIGHBOUR
		vec4 pcol = texture2D(rubyTexture, coords_texture + onex);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(-SCALE , 0.0));

		// LEFT NEIGHBOUR
		pcol = texture2D(rubyTexture, coords_texture - onex);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(SCALE , 0.0));

	#ifdef USE_ALL_NEIGHBOURS
		// TOP
		pcol = texture2D(rubyTexture, coords_texture + oney);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(0.0, -SCALE));

		// TOP-LEFT
		pcol = texture2D(rubyTexture, coords_texture + oney - onex);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(SCALE, -SCALE));

		// TOP-RIGHT
		pcol = texture2D(rubyTexture, coords_texture + oney + onex);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(-SCALE, -SCALE));

		// BOTTOM
		pcol = texture2D(rubyTexture, coords_texture - oney);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(0.0, SCALE));

		// BOTTOM-LEFT
		pcol = texture2D(rubyTexture, coords_texture - oney - onex);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(SCALE, SCALE));

		// BOTTOM-RIGHT
		pcol = texture2D(rubyTexture, coords_texture - oney + onex);
		luminosity = dot(pcol, luminosity_weights);
		color += pcol * factor(luminosity, ecart + vec2(-SCALE, SCALE));
	#endif // USE_ALL_NEIGHBOURS

	#ifdef TRIAD1
		vec2 coords_screen = floor(gl_TexCoord[0].xy * rubyOutputSize);

		float modulo = mod(coords_screen.y + coords_screen.x , 3.0);
		if (modulo == 0.0)
		    color.rgb *= vec3(1.0,0.5,0.5);
		else if  (modulo <= 1.0)
		    color.rgb *= vec3(0.5,1.0,0.5);
		else
		    color.rgb *= vec3(0.5,0.5,1.0);
	#endif // TRIAD1

	#ifdef TRIAD2
		color = clamp(color, 0.0, 1.0);

		vec2 coords_screen = floor(gl_TexCoord[0].xy * rubyOutputSize);

		float modulo = mod(coords_screen.x , 3.0);
		if (modulo == 0.0)            color.gb *= 0.8;
		else if (modulo == 1.0)      color.rb *= 0.8;
		else                                            color.rg *= 0.8;
	#endif // TRIAD2

		gl_FragColor = clamp(color, 0.0, 1.0);
        }
    ]]></fragment>
</shader>
