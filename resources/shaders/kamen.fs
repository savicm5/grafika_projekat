#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
in vec2 koordTeksture;

uniform sampler2D tekstura;

void main()
{
    FragColor = texture(tekstura, koordTeksture);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        if(brightness > 0.8)
            BrightColor = vec4(FragColor.rgb, 1.0);
    	else
    		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
//     FragColor=vec4(1.0f, 1.0f, 0.0f, 1.0f);
}