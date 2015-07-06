varying vec3 MC_position;
varying vec3 MC_eye;
varying vec3 light_pos;

uniform vec3 volScale;

uniform float sampleRate;

uniform vec2 xSliceBounds;
uniform vec2 ySliceBounds;
uniform vec2 zSliceBounds;

uniform sampler3D volumeTex;
uniform sampler2D randTex;
uniform sampler1D cmapTex;
uniform sampler1D amapTex;
uniform sampler3D normalTex;

// material properties
vec3 Ka = vec3(1.0, 1.0, 1.0); // ambient
vec3 Kd = vec3(1.0, 1.0, 1.0); // diffuse
vec3 Ks = vec3(1.0, 1.0, 1.0); // specular
float n = 100.0; // shininess

// light properties
vec3 ambientLight = vec3(.005, .005, .005);
vec3 ambient = Ka * ambientLight;

void main()
{ 
  vec3 rayPos = gl_TexCoord[0].stp;
  vec3 rayStep = normalize((MC_position - MC_eye) * volScale) * .001 * sampleRate;
  
  rayPos += (rayStep * texture2D(randTex, vec2(gl_FragCoord.x/128.0, gl_FragCoord.y/128.0)).a);
  vec4 rayColor = vec4(0.0, 0.0, 0.0, 0.0);
  
  vec3 V = normalize(MC_position - MC_eye);
  vec3 L = normalize(light_pos);
  vec3 H = normalize(L + V);
  
  while (rayPos.x <= 1.0 && rayPos.x >= 0.0 &&
  	 rayPos.y <= 1.0 && rayPos.y >= 0.0 &&
  	 rayPos.z <= 1.0 && rayPos.z >= 0.0 && rayColor.a < .97)
    {
      /*
      if ((rayPos.x > xSliceBounds.y) || (rayPos.x < xSliceBounds.x)
	  || (rayPos.y > ySliceBounds.y) || (rayPos.y < ySliceBounds.x)
	  || (rayPos.z > zSliceBounds.y) || (rayPos.z < zSliceBounds.x)) {
	rayPos += rayStep;
	continue;
      }
      */

      float scalar = texture3D(volumeTex, rayPos).a;
      float alpha = texture1D(amapTex, scalar).a;

      if (alpha > .003) {

	vec4  norm = vec4(texture3D(normalTex, rayPos));

	float magDiff = norm.a * 8.0;
	vec3  N = norm.xyz;

	vec3  cmapColor = vec3(texture1D(cmapTex, scalar));

	float diffuseLight = max(dot(L, N), 0.0);
	vec3  diffuse      = cmapColor * diffuseLight; // also should mult by Kd, but it's 1.0

	float specularLight = pow(max(dot(H, N), 0.0), n);
	vec3  specular      = vec3(specularLight); // also should mult by Ks, but it's 1.0
	
	vec3 ambient = ambientLight; // also should mult by Ka, but it's 1.0
	
	alpha *= sampleRate;
	alpha = min(alpha, 1.0);

	rayColor += (1.0-rayColor.a) * vec4(((ambient + diffuse + specular)*alpha*magDiff), alpha*magDiff);
      }

      rayPos += rayStep;
    }
  
  gl_FragColor = rayColor;
}

