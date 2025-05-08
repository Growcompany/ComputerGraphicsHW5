//
//  sphere_scene.c
//  Rasterizer
//
//

#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Simple 3D vertex
struct Vec3 { float x, y, z; };

// Mesh globals
int   gNumVertices = 0;
int   gNumTriangles = 0;
Vec3* gVertexArray = nullptr;
int* gIndexBuffer = nullptr;

// Generate unit-sphere mesh
void create_scene() {
    int width = 32;
    int height = 16;

    gNumVertices = (height - 2) * width + 2;
    gNumTriangles = (height - 2) * (width - 1) * 2;

    gVertexArray = new Vec3[gNumVertices];
    gIndexBuffer = new int[3 * gNumTriangles];

    int t = 0;
    // middle latitude rings
    for (int j = 1; j < height - 1; ++j) {
        float theta = float(j) / (height - 1) * M_PI;
        for (int i = 0; i < width; ++i) {
            float phi = float(i) / (width - 1) * 2.0f * M_PI;
            gVertexArray[t].x = sinf(theta) * cosf(phi);
            gVertexArray[t].y = cosf(theta);
            gVertexArray[t].z = -sinf(theta) * sinf(phi);
            ++t;
        }
    }
    // north pole
    gVertexArray[t].x = 0.0f;
    gVertexArray[t].y = 1.0f;
    gVertexArray[t].z = 0.0f;
    ++t;
    // south pole
    gVertexArray[t].x = 0.0f;
    gVertexArray[t].y = -1.0f;
    gVertexArray[t].z = 0.0f;
    ++t;

    // build index buffer
    t = 0;
    for (int j = 0; j < height - 3; ++j) {
        int row0 = j * width;
        int row1 = (j + 1) * width;
        for (int i = 0; i < width - 1; ++i) {
            // first tri
            gIndexBuffer[t++] = row0 + i;
            gIndexBuffer[t++] = row1 + i + 1;
            gIndexBuffer[t++] = row0 + i + 1;
            // second tri
            gIndexBuffer[t++] = row0 + i;
            gIndexBuffer[t++] = row1 + i;
            gIndexBuffer[t++] = row1 + i + 1;
        }
    }
    // connect poles
    for (int i = 0; i < width - 1; ++i) {
        // north
        gIndexBuffer[t++] = (height - 2) * width;
        gIndexBuffer[t++] = i;
        gIndexBuffer[t++] = i + 1;
        // south
        gIndexBuffer[t++] = (height - 2) * width + 1;
        gIndexBuffer[t++] = (height - 3) * width + i + 1;
        gIndexBuffer[t++] = (height - 3) * width + i;
    }
}

    // The index buffer has now been generated. Here's how to use to determine
    // the vertices of a triangle. Suppose you want to determine the vertices
    // of triangle i, with 0 <= i < gNumTriangles. Define:
    //
    // k0 = gIndexBuffer[3*i + 0]
    // k1 = gIndexBuffer[3*i + 1]
    // k2 = gIndexBuffer[3*i + 2]
    //
    // Now, the vertices of triangle i are at positions k0, k1, and k2 (in that
    // order) in the vertex array (which you should allocate yourself at line
    // 27).
    //
    // Note that this assumes 0-based indexing of arrays (as used in C/C++,
    // Java, etc.) If your language uses 1-based indexing, you will have to
    // add 1 to k0, k1, and k2.

