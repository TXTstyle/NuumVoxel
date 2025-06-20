$ input a_texcoord0

#include <bgfx_compute.sh>

IMAGE2D_WO ( u_outputImage, rgba8, 0 ) ; // output image at binding 0
SAMPLER3D(s_voxelTexture, 1); // 3D texture at binding 1
BUFFER_RO(paletteBuffer, vec4, 2); // palette buffer at binding 2

uniform vec4 u_camPos; // camera position
uniform mat3 u_camMat; // inverse view matrix
uniform vec4 u_gridSize; // xyz: grid dimensions, w: scale factor

// Safer division that avoids dividing by zero
vec3 safeDiv(vec3 a, vec3 b) {
    return vec3(
        abs(b.x) < 1e-6 ? sign(b.x) * 1e6 : a.x / b.x,
        abs(b.y) < 1e-6 ? sign(b.y) * 1e6 : a.y / b.y,
        abs(b.z) < 1e-6 ? sign(b.z) * 1e6 : a.z / b.z
    );
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_outputImage);
    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y)
        return;

    vec3 camPos = u_camPos.xyz;

    // Volume bounds and grid size from uniforms
    vec3 gridSize = u_gridSize.xyz; // voxel grid size
    vec3 u_volumeMin = vec3(-1.0, 0.0, -1.0) * gridSize * 0.0625 * u_gridSize.w; // volume min bounds
    vec3 u_volumeMax = vec3(1.0, 2.0, 1.0) * gridSize * 0.0625 * u_gridSize.w; // volume max bounds

    // Calculate screen coordinates and ray direction
    float fov = 45.0; // Field of view
    vec2 uv = ((vec2(pixelCoords) + 0.5) / vec2(imageSize)) * 2.0 - 1.0;
    uv.y = -uv.y; // Invert y-axis for OpenGL
    float aspect = float(imageSize.x) / float(imageSize.y);
    float scale = tan(radians(fov) * 0.5);
    uv.x *= aspect * scale;
    uv.y *= scale;
    vec3 rayDirCam = normalize(vec3(uv, -1.0)); // Ray direction in camera space
    vec3 rayDir = normalize(u_camMat * rayDirCam); // Ray direction in world space

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
    const vec3 fromColor = vec3(0.16, 0.29, 0.48);
    const vec3 toColor = vec3_splat(0.058);
    float mixFactor = smoothstep(0.0, 1.0, 1 - (vec2(pixelCoords) / vec2(imageSize)).y);
    const vec4 bgColor = vec4(mix(toColor, fromColor, mixFactor), 1.0);
    if (tmax <= tmin)
    {
        imageStore(u_outputImage, pixelCoords, bgColor);
        return;
    }

    // Start position at entry point with slight offset to avoid boundary issues
    vec3 pos = camPos + rayDir * (tmin + epsilon);

    // Calculate voxel size
    vec3 voxelSize = (u_volumeMax - u_volumeMin) / gridSize;

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
    #if 0
    // Show ray entry points (useful for debugging)
    imageStore(u_outputImage, pixelCoords, vec4(normalize(pos - u_volumeMin) * 0.5 + 0.5, 1.0));
    return;
    #endif

    vec3 hitNormal = vec3(0.0);
    float eps = 1e-5;
    if (abs(tmin - t0s.x) < eps) hitNormal.x = 1.0;
    if (abs(tmin - t1s.x) < eps) hitNormal.x = -1.0;
    if (abs(tmin - t0s.y) < eps) hitNormal.y = 1.0;
    if (abs(tmin - t1s.y) < eps) hitNormal.y = -1.0;
    if (abs(tmin - t0s.z) < eps) hitNormal.z = 1.0;
    if (abs(tmin - t1s.z) < eps) hitNormal.z = -1.0;
    // Ray marching through the grid
    const int maxSteps = 256;
    for (int i = 0; i < maxSteps; ++i)
    {
        // Check if current voxel is inside the grid
        if (any(lessThan(voxel, ivec3(0))) || any(greaterThanEqual(voxel, ivec3(gridSize))))
            break;

        // Sample voxel data
        vec3 texCoord = (vec3(voxel) + vec3(0.5)) / gridSize;
        float voxelValue = texture3D(s_voxelTexture, texCoord);

        // If we hit a solid voxel, render it
        if (voxelValue > 0.003) // 1/255
        {
            // Fetch color from palette buffer
            float index = voxelValue * 255.0; // Voxel values are in [0, 1]
            vec4 color = paletteBuffer[int(index)];
            // Lambertian shading based on normal
            float lightIntensity = max(dot(hitNormal, rayDir), 0.1);
            color.rgb *= lightIntensity;
            imageStore(u_outputImage, pixelCoords, color);
            return;
        }

        // Find the next voxel to step to
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            voxel.x += step.x;
            tMax.x += deltaT.x;
            hitNormal = vec3(step.x, 0.0, 0.0);
        }
        else if (tMax.y < tMax.z)
        {
            voxel.y += step.y;
            tMax.y += deltaT.y;
            hitNormal = vec3(0.0, step.y, 0.0);
        }
        else
        {
            voxel.z += step.z;
            tMax.z += deltaT.z;
            hitNormal = vec3(0.0, 0.0, step.z);
        }
    }

    // Ray-plane intersection, with xz plane grid visualization
    const float cellSize = 8.0f * 1 / u_gridSize.w; // Number of grid cells per unit
    const float u_lineWidth = 0.03f; // Width of the grid lines
    const float planeY = 0.0;
    float denom = rayDir.y;
    if (abs(denom) > epsilon) {
        float tPlane = (planeY - camPos.y) / denom;

        if (tPlane >= tmin && tPlane <= tmax) {
            vec3 intersectionPoint = camPos + tPlane * rayDir;

            // Compute grid position in XZ plane
            vec2 gridPos = intersectionPoint.xz * cellSize;

            // Compute the distance to the nearest grid line (using fract)
            vec2 gridLineDist = abs(fract(gridPos - 0.5) - 0.5);

            // Minimum distance to grid line on either axis
            float lineDist = min(gridLineDist.x, gridLineDist.y);

            // Smooth line thickness
            float linee = smoothstep(0.0, u_lineWidth, lineDist);

            // Lines are white and background is bgcolor
            vec3 color = mix(vec3(1.0), bgColor.rgb, linee);

            imageStore(u_outputImage, pixelCoords, vec4(vec3(color), 1.0));
            return;
        }
    }

    // If we didn't hit anything
    imageStore(u_outputImage, pixelCoords, bgColor);
}
