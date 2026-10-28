#include "BenchMesh.h"
#include "Draw.h"
#include "glad.h"
#include <cmath>
#include <algorithm>
#include <cstring>

BenchMesh::BenchMesh() {
    posAttr       = new Attribute<vec3>();
    normAttr      = new Attribute<vec3>();
    weightAttr    = new Attribute<vec4>();
    influenceAttr = new Attribute<ivec4>();
    indexBuf      = new IndexBuffer();
}

BenchMesh::~BenchMesh() {
    delete posAttr;
    delete normAttr;
    delete weightAttr;
    delete influenceAttr;
    delete indexBuf;
}

// Assign skinning weights for vertex at normalized position u in [0,1]
// across the bone chain. Produces exactly numInfluences non-zero weights.
void BenchMesh::AssignWeights(int vi, float u, int numBones, int numInfluences) {
    // Map u -> bone position [0, numBones-1]
    float bonePos = u * (numBones - 1);
    int   nearBone = (int)(bonePos + 0.5f);
    if (nearBone >= numBones) nearBone = numBones - 1;

    // Build a sorted list of (distance, boneIndex) for all bones,
    // pick the `numInfluences` nearest, assign linear-falloff weights.
    struct BD { float dist; int idx; };
    std::vector<BD> dists(numBones);
    for (int b = 0; b < numBones; ++b) {
        dists[b] = { fabsf(bonePos - b), b };
    }
    std::sort(dists.begin(), dists.end(), [](const BD& a, const BD& b){ return a.dist < b.dist; });

    // Up to 4 influences (shader always reads ivec4/vec4).
    float w[4] = {0,0,0,0};
    int   j[4] = {0,0,0,0};
    int   k = std::min(numInfluences, 4);

    float sum = 0.0f;
    for (int i = 0; i < k; ++i) {
        j[i] = dists[i].idx;
        // Linear falloff: weight = max(0, 1 - distance)
        float d = dists[i].dist;
        w[i] = (d < 1.0f) ? (1.0f - d) : (1.0f / (1.0f + d));
        sum += w[i];
    }
    // Normalize
    if (sum > 1e-6f) {
        for (int i = 0; i < k; ++i) w[i] /= sum;
    } else {
        w[0] = 1.0f;
    }

    weights[vi]    = vec4(w[0], w[1], w[2], w[3]);
    influences[vi] = ivec4(j[0], j[1], j[2], j[3]);
}

void BenchMesh::Generate(int targetVertices, int numBones, int numInfluences) {
    // Create a grid of quads along X axis (bone chain direction).
    // Grid: segmentsX columns, segmentsY rows.
    // Vertex count = (segX+1)*(segY+1), try to match targetVertices.
    // Use a 4:1 aspect ratio for segments to simulate a long bar.
    int segY = (int)sqrtf((float)targetVertices / 4.0f);
    if (segY < 1) segY = 1;
    int segX = (targetVertices / (segY + 1)) - 1;
    if (segX < 1) segX = 1;

    int Vx = segX + 1;
    int Vy = segY + 1;
    int totalVerts = Vx * Vy;

    positions.resize(totalVerts);
    normals.resize(totalVerts, vec3(0, 1, 0));
    weights.resize(totalVerts);
    influences.resize(totalVerts);

    for (int iy = 0; iy < Vy; ++iy) {
        float v = (float)iy / (float)segY;
        for (int ix = 0; ix < Vx; ++ix) {
            float u = (float)ix / (float)segX;
            int vi = iy * Vx + ix;
            positions[vi] = vec3(u * 10.0f - 5.0f, v * 2.0f - 1.0f, 0.0f);
            normals[vi]   = vec3(0, 0, 1);
            AssignWeights(vi, u, numBones, numInfluences);
        }
    }

    // Build index buffer (two triangles per quad).
    indices.clear();
    indices.reserve(segX * segY * 6);
    for (int iy = 0; iy < segY; ++iy) {
        for (int ix = 0; ix < segX; ++ix) {
            int i00 = iy * Vx + ix;
            int i10 = iy * Vx + ix + 1;
            int i01 = (iy+1) * Vx + ix;
            int i11 = (iy+1) * Vx + ix + 1;
            indices.push_back(i00); indices.push_back(i10); indices.push_back(i11);
            indices.push_back(i00); indices.push_back(i11); indices.push_back(i01);
        }
    }

    Upload();
}

void BenchMesh::Upload() {
    posAttr->Set(positions);
    normAttr->Set(normals);
    weightAttr->Set(weights);
    influenceAttr->Set(influences);
    indexBuf->Set(indices);
}

void BenchMesh::UpdateGPU() {
    posAttr->Set(positions);
    normAttr->Set(normals);
}

void BenchMesh::Bind(int pos, int norm, int /*texCoord*/, int weight, int influence) {
    if (pos >= 0)       posAttr->BindTo(pos);
    if (norm >= 0)      normAttr->BindTo(norm);
    if (weight >= 0)    weightAttr->BindTo(weight);
    if (influence >= 0) influenceAttr->BindTo(influence);
}

void BenchMesh::UnBind(int pos, int norm, int /*texCoord*/, int weight, int influence) {
    if (pos >= 0)       posAttr->UnBindFrom(pos);
    if (norm >= 0)      normAttr->UnBindFrom(norm);
    if (weight >= 0)    weightAttr->UnBindFrom(weight);
    if (influence >= 0) influenceAttr->UnBindFrom(influence);
}

void BenchMesh::Draw() {
    if (!indices.empty()) {
        ::Draw(*indexBuf, DrawMode::Triangles);
    } else {
        ::Draw((unsigned int)positions.size(), DrawMode::Triangles);
    }
}
