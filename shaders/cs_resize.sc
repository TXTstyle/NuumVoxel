#include <bgfx_compute.sh>

IMAGE3D_WO(u_outputImage, r32f, 0);
IMAGE3D_RO(u_voxelTexture, r32f, 1);

void main() {
    ivec3 pixelCoords = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 imageSize = imageSize(u_outputImage);

    // Skip out-of-bounds pixels
    if (any(greaterThanEqual(pixelCoords, imageSize)))
        return;

    // Direct integer-coordinate read
    float pixelValue = imageLoad(u_voxelTexture, pixelCoords).r;

    // Write to output image
    imageStore(u_outputImage, pixelCoords, vec4(pixelValue, 0.0, 0.0, 0.0));
}

