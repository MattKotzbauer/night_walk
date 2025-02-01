// shader.frag (Fragment Shader)

#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec2 FragPos;

uniform sampler2D gameTexture;
uniform sampler2D angleTexture;

struct Light{
  vec2 position;
  vec3 color;
  float intensity;
  float radius;
};

#define MAX_LIGHTS 8
uniform Light lights[MAX_LIGHTS];
uniform int numActiveLights;
uniform float ambientStrength;

void main(){
  // Get base color and angle from textures
  vec4 baseColor = texture(gameTexture, TexCoord);
  vec4 angleData = texture(angleTexture, TexCoord);

  if(angleData.a == 0u){ FragColor = baseColor; return; }  

  /* 
  if(angleData.a == 0u){
        vec3 finalColor = baseColor.rgb;
        
        // Simple distance-based lighting for surfaces without normals
        for(int i = 0; i < numActiveLights; ++i) {
            vec2 lightDir = lights[i].position - FragPos;
            float distance = length(lightDir);
            
            if(distance > lights[i].radius) { continue; }
            
            // Simple radial attenuation without normal calculations
            float attenuation = 1.0 - smoothstep(0.0, lights[i].radius, distance);
            finalColor += baseColor.rgb * lights[i].color * lights[i].intensity * .* attenuation;
        }
        
        finalColor = min(finalColor, vec3(1.0));
        FragColor = vec4(finalColor, baseColor.a);
        return;
	}*/

  float surfaceAngleRaw = (angleData.r + angleData.g) * 255.0;

  // Convert stored angle to direction vector
  float angleRadians = radians(surfaceAngleRaw);
  vec2 surfaceNormal = vec2(cos(angleRadians), sin(angleRadians));

  // Start with ambient light
  // vec3 finalColor = baseColor.rgb * ambientStrength;
  vec3 finalColor = baseColor.rgb;

  // Process each active light
  for(int i = 0; i < numActiveLights; ++i){
    // Calculate light direction and distance
    vec2 lightDir = lights[i].position - FragPos;
    float distance = length(lightDir);

    // Skip if fragment is outside light radius
    if(distance > lights[i].radius) { continue; }
	    
    // Normalize light direction
    lightDir = normalize(lightDir);

    // Calculate diffuse factor based on angle
    float diff = max(dot(surfaceNormal, lightDir), 0.0);

    // Calculate attenuation based on distance
    float attenuation = 1.0 - smoothstep(1.0, lights[i].radius, distance);	    

    // (Add to this light's contribution)
    finalColor += baseColor.rgb * lights[i].color * diff * lights[i].intensity * attenuation;
  }

  // Ensure that we don't exceed max brightness
  finalColor = min(finalColor, vec3(1.0));
  FragColor = vec4(finalColor, baseColor.a);

}
