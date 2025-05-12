$ input a_texcoord0

#include <bgfx_compute.sh>

IMAGE2D_WO (u_outputImage, rgba32f, 0 ) ; // output image at binding 0
SAMPLER3D(s_voxelTexture, 1); // 3D texture at binding 1

uniform vec4 u_camPos; // camera position
uniform vec4 u_gridSize; // xyz: grid dimensions, w: scale factor
uniform mat4 u_viewInv; // inverse view matrix
uniform mat4 u_projInv; // inverse projection matrix

// Convert screen coordinates to world ray direction using view and projection matrices
vec3 screenToWorldRay(vec2 uv, mat4 u_projInv, mat4 u_viewInv) {
    // Transform from screen space to clip space (-1 to 1)
    vec4 clipPos = vec4(uv * 2.0 - 1.0, 1.0, 0.0);
    
    // Transform from clip space to view space
    vec4 viewPos = mul(u_projInv, clipPos);
    viewPos /= viewPos.w;
    
    // Ray in view space points forward
    vec4 viewRay = vec4(viewPos.xy, -1.0, 0.0);
    
    // Transform ray to world space
    vec4 worldRay = mul(u_viewInv, viewRay);
    
    return normalize(worldRay.xyz);
}

// Safer division that avoids dividing by zero
vec3 safeDiv(vec3 a, vec3 b) {
    return vec3(
        abs(b.x) < 1e-6 ? sign(b.x) * 1e6 : a.x / b.x,
        abs(b.y) < 1e-6 ? sign(b.y) * 1e6 : a.y / b.y,
        abs(b.z) < 1e-6 ? sign(b.z) * 1e6 : a.z / b.z
    );
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_outputImage);
    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y)
        return;

    // Get camera position from inverse view matrix
    vec3 camPos = u_camPos.xyz;

    // Volume bounds and grid size from uniforms
    vec3 u_volumeMin = vec3(-1.0, 0.0, -1.0) * u_gridSize.w; // volume min bounds
    vec3 u_volumeMax = vec3(1.0, 2.0, 1.0) * u_gridSize.w; // volume max bounds
    vec3 u_voxelGridSize = u_gridSize.xyz; // voxel grid size

    // Calculate screen coordinates and ray direction
    vec2 uv = vec2(pixelCoords) / vec2(imageSize);
    vec3 rayDir = screenToWorldRay(uv, u_projInv, u_viewInv);

    // imageStore(u_outputImage, pixelCoords, vec4(rayDir * 0.5 + 0.5, 1.0));
    // return;

    // Apply epsilon to avoid floating point precision errors
    const float epsilon = 1e-4;

    // Ray-box intersection
    vec3 invDir = safeDiv(vec3(1.0), rayDir);
    vec3 t0s = (u_volumeMin - camPos) * invDir;
    vec3 t1s = (u_volumeMax - camPos) * invDir;
    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);
    float tmin = max(0.0, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
    float tmax = min(tbigger.x, min(tbigger.y, tbigger.z));

    // Early exit if ray doesn't hit volume
    if (tmax <= tmin)
    {
        imageStore(u_outputImage, pixelCoords, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }

    // Start position at entry point with slight offset to avoid boundary issues
    vec3 pos = camPos + rayDir * (tmin + epsilon);

    // Calculate voxel size
    vec3 voxelSize = (u_volumeMax - u_volumeMin) / u_voxelGridSize;

    // Calculate initial voxel indices
    ivec3 voxel = ivec3(floor((pos - u_volumeMin) / voxelSize));

    // DDA setup
    vec3 deltaT = abs(safeDiv(voxelSize, rayDir));
    ivec3 step = ivec3(sign(rayDir));

    // Calculate initial tMax values
    vec3 voxelBoundary = vec3(0.0);
    for (int i = 0; i < 3; i++) {
        voxelBoundary[i] = u_volumeMin[i] + (rayDir[i] > 0.0 ?
                (voxel[i] + 1) * voxelSize[i] :
                voxel[i] * voxelSize[i]);
    }

    vec3 tMax = abs(safeDiv(voxelBoundary - pos, rayDir));

    // Debug visualization
    // #if 0
    // // Show ray entry points (useful for debugging)
    // imageStore(u_outputImage, pixelCoords, vec4(normalize(pos - u_volumeMin) * 0.5 + 0.5, 1.0));
    // return;
    // #endif

    // Ray marching through the grid
    const int maxSteps = 256;
    for (int i = 0; i < maxSteps; ++i)
    {
        // Check if current voxel is inside the grid
        if (any(lessThan(voxel, ivec3(0))) || any(greaterThanEqual(voxel, ivec3(u_voxelGridSize))))
            break;

        // Sample voxel data
        vec3 texCoord = (vec3(voxel) + vec3(0.5)) / u_voxelGridSize;
        vec4 voxelValue = texture3D(s_voxelTexture, texCoord);

        // If we hit a solid voxel, render it
        if (voxelValue.a > 0.1)
        {
            imageStore(u_outputImage, pixelCoords, vec4(voxelValue.rgb, 1.0));
            return;
        }

        // Find the next voxel to step to
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

    // If we didn't hit anything, output background color
    imageStore(u_outputImage, pixelCoords, vec4(0.0, 0.0, 0.0, 1.0));
}
