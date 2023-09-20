#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D renderTex;
uniform int screenWidth;
uniform int screenHeight;
uniform int crosshairType;

void main(){
    float crosshair_alpha = 0.8;

    float dist = length(gl_FragCoord.xy-vec2(screenWidth/2, screenHeight/2));

    FragColor = texture(renderTex, TexCoord);
    /*float crosshair_color = (FragColor.x + FragColor.y + FragColor.z) / 3;
    /*if(crosshair_color <= 0.5) crosshair_color = 1.0;
    /*else crosshair_color = 0.0;*/
    float crosshair_color = 1.0;

    if(dist <= 7){
	if( (crosshairType == 0 && dist >= 5) ||
		(crosshairType == 1 && ( int(gl_FragCoord.x) == int(screenWidth / 2) ||
		 int(gl_FragCoord.y) == int(screenHeight / 2)) )
	    ) FragColor = vec4(vec3(crosshair_color), crosshair_alpha);
	}
}
