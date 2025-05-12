$input a_texcoord0

#include <bgfx_compute.sh>

IMAGE2D_WO(u_outputImage, rgba32f, 0); // output image at binding 0
SAMPLER3D(s_voxelTexture, 1);          // 3D texture at binding 1

uniform vec4 u_cameraPos;             // camera position

vec3 screenToWorldRay(vec2 uv, vec3 camPos)
{
    vec4 ndc = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 world = mul(u_invViewProj, ndc);
    return normalize(world.xyz / world.w - camPos);
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_outputImage);
    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y)
        return;

    vec3 camPos = u_cameraPos.xyz;
    vec3 u_volumeMin = vec3(0.0, 0.0, 0.0); // volume min bounds
    vec3 u_volumeMax = vec3(1.0, 1.0, 1.0); // volume max bounds
    vec3 u_voxelGridSize = vec3_splat(16.0); // voxel grid size

    vec2 uv = vec2(pixelCoords) / vec2(imageSize);
    vec3 rayDir = screenToWorldRay(uv, camPos);

    // Ray-box intersection
    vec3 invDir = 1.0 / rayDir;
    vec3 t0s = (u_volumeMin - camPos) * invDir;
    vec3 t1s = (u_volumeMax - camPos) * invDir;
    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);
    float tmin = max(0.0, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
    float tmax = min(tbigger.x, min(tbigger.y, tbigger.z));
    if (tmax < tmin)
    {
        imageStore(u_outputImage, pixelCoords, vec4_splat(0.0));
        return;
    }

    vec3 pos = camPos + rayDir * tmin;
    vec3 voxelSize = (u_volumeMax - u_volumeMin) / vec3(u_voxelGridSize);
    ivec3 voxel = ivec3(floor((pos - u_volumeMin) / voxelSize));

    vec3 deltaT = abs(voxelSize / rayDir);
    ivec3 step = ivec3(sign(rayDir));
    vec3 nextVoxelBoundary = u_volumeMin + (vec3(voxel) + step * 0.5 + 0.5) * voxelSize;
    vec3 tMax = abs((nextVoxelBoundary - pos) / rayDir);

    for (int i = 0; i < 256; ++i)
    {
        if (any(lessThan(voxel, ivec3(0))) || any(greaterThanEqual(voxel, u_voxelGridSize)))
            break;

        vec3 texCoord = (vec3(voxel) + 0.5) / vec3(u_voxelGridSize);
        float voxelValue = texture3D(s_voxelTexture, texCoord).r;

        if (voxelValue > 0.1)
        {
            imageStore(u_outputImage, pixelCoords, vec4(vec3(voxelValue), 1.0));
            return;
        }

        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            voxel.x += step.x;
            tMax.x += deltaT.x;
        }
        else if (tMax.y < tMax.z)
        {
            voxel.y += step.y;
            tMax.y += deltaT.y;
        }
        else
        {
            voxel.z += step.z;
            tMax.z += deltaT.z;
        }
    }

    imageStore(u_outputImage, pixelCoords, vec4(0.0));
}
