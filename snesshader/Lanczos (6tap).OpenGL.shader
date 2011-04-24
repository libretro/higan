<?xml version="1.0" encoding="UTF-8"?>
<!--
    Copyright (C) 2010 Team XBMC
    http://www.xbmc.org
    Copyright (C) 2011 Stefanos A.
    http://www.opentk.com

This Program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This Program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XBMC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
http://www.gnu.org/copyleft/gpl.html
-->
<!--
    From this forum post:

	http://board.byuu.org/viewtopic.php?p=33597
-->
<shader language="GLSL">
    <vertex><![CDATA[
	void main()
	{
	    gl_TexCoord[0] = gl_MultiTexCoord0;         //center
	    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	}
    ]]></vertex>

    <fragment><![CDATA[
	uniform sampler2D rubyTexture;
	uniform vec2 rubyTextureSize;

	const float PI = 3.1415926535897932384626433832795;
	
	float sinc(float x)
	{
	    return sin(x * PI) / (x * PI);
	}
	
	float weight(float x, float radius)
	{
	    float ax = abs(x);

	    if (x == 0.0)
		return 1.0;
	    else if (ax < radius)
		return sinc(x) * sinc(x / radius);
	    else
		return 0.0;
	}

	vec3 weight3(float x)
	{
	    return vec3(
		weight(x * 2.0 + 0.0 * 2.0 - 3.0, 3.0),
		weight(x * 2.0 + 1.0 * 2.0 - 3.0, 3.0),
		weight(x * 2.0 + 2.0 * 2.0 - 3.0, 3.0));
	}

	vec3 pixel(float xpos, float ypos)
	{
	    return texture2D(rubyTexture, vec2(xpos, ypos)).rgb;
	}

	vec3 line(float ypos, vec3 xpos1, vec3 xpos2, vec3 linetaps1, vec3 linetaps2)
	{
	    return
		pixel(xpos1.r, ypos) * linetaps1.r +
		pixel(xpos1.g, ypos) * linetaps2.r +
		pixel(xpos1.b, ypos) * linetaps1.g +
		pixel(xpos2.r, ypos) * linetaps2.g +
		pixel(xpos2.g, ypos) * linetaps1.b +
		pixel(xpos2.b, ypos) * linetaps2.b; 
	}

	void main()
	{
	    vec2 stepxy = 1.0 / rubyTextureSize.xy;
	    vec2 pos = gl_TexCoord[0].xy + stepxy * 0.5;
	    vec2 f = fract(pos / stepxy);

	    vec3 linetaps1   = weight3((1.0 - f.x) / 2.0);
	    vec3 linetaps2   = weight3((1.0 - f.x) / 2.0 + 0.5);
	    vec3 columntaps1 = weight3((1.0 - f.y) / 2.0);
	    vec3 columntaps2 = weight3((1.0 - f.y) / 2.0 + 0.5);

	    // make sure all taps added together is exactly 1.0, otherwise some
	    // (very small) distortion can occur
	    float suml =
		linetaps1.r +
		linetaps1.g +
		linetaps1.b +
		linetaps2.r +
		linetaps2.g +
		linetaps2.b;
	    float sumc =
		columntaps1.r +
		columntaps1.g +
		columntaps1.b +
		columntaps2.r +
		columntaps2.g +
		columntaps2.b;
	    linetaps1 /= suml;
	    linetaps2 /= suml;
	    columntaps1 /= sumc;
	    columntaps2 /= sumc;

	    vec2 xystart = (-2.5 - f) * stepxy + pos;
	    vec3 xpos1 = vec3(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0);
	    vec3 xpos2 = vec3(xystart.x + stepxy.x * 3.0, xystart.x + stepxy.x * 4.0, xystart.x + stepxy.x * 5.0);

	    gl_FragColor.rgb =
		line(xystart.y                 , xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
		line(xystart.y + stepxy.y      , xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
		line(xystart.y + stepxy.y * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
		line(xystart.y + stepxy.y * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
		line(xystart.y + stepxy.y * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
		line(xystart.y + stepxy.y * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

	    gl_FragColor.a = 1.0;
	}
    ]]></fragment>
</shader>
