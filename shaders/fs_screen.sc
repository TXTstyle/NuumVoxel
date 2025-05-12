$input v_texcoord0
#include <bgfx_shader.sh>

// Match the uniform names from C++ code
SAMPLER2D(u_texture, 0);

void main() {
    // Sample G-buffer
    vec4 albedoData = texture2D(u_texture, v_texcoord0);
    
    // Gamma correction for better visual results
    vec3 finalColor = pow(abs(albedoData.rgb), vec3_splat(1.0/2.2));
    
    gl_FragColor = vec4(finalColor, 1.0);
}
