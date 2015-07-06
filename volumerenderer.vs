varying vec3 MC_position;
varying vec3 MC_eye;

varying vec3 light_pos;

void main()
{
  MC_position = gl_Vertex.xyz;
  MC_eye = vec3(gl_ModelViewMatrixInverse*vec4(0.0, 0.0, 0.0, 1.0));

  gl_TexCoord[0] = gl_MultiTexCoord0;

  //light_pos = normalize(vec3(gl_ModelViewMatrixInverse*vec4(1.0, 1.0, 1.0, 1.0)));
  light_pos = -vec3(gl_ModelViewMatrixInverse*vec4(20.0, 20.0, 10.0, 1.0));

  gl_Position = ftransform();
}

