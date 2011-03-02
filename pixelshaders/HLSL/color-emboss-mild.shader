<?xml version="1.0" encoding="UTF-8"?>
<!--
    Color Emboss Mild Shader
	Copyright (C) 2011 hunterk

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Requires bilinear filtering ('smooth video') to work right.
    -->
<shader language="HLSL">
  <source><![CDATA[
  //These variables will get set automatically
texture rubyTexture;

sampler s0 = sampler_state { texture = <rubyTexture>; };

float4 EmbossPass( in float2 Tex : TEXCOORD0 ) : COLOR0
{
	float4 Color = tex2D( s0, Tex );
	    Color = tex2D( s0, Tex.xy);
        Color -= tex2D( s0, Tex.xy+0.0001)*30.0f;
        Color += tex2D( s0, Tex.xy-0.0001)*30.0f;
		Color = Color/1.2;
	return Color;
}




Technique T0
{
    pass p0 { PixelShader = compile ps_2_0 EmbossPass(); }
}
  ]]></source>
</shader>
