#pragma once
#ifndef _H_BENCH_MESH_
#define _H_BENCH_MESH_

#include "vec3.h"
#include "vec4.h"
#include "Attribute.h"
#include "IndexBuffer.h"
#include <vector>

// BenchMesh: procedurally generated skinned mesh for GPU benchmarking.
// Generates a grid-based mesh (bent ribbon/bar shape) with synthetic weights
// from a bone chain. The mesh is sized to have approximately `targetVertices`
// vertices. Bones are indexed 0..numBones-1 in a simple chain (parent = i-1).
// Weights fall off linearly from the nearest bone and are normalized to
// exactly `numInfluences` non-zero influences.
class BenchMesh {
public:
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec4> weights;
    std::vector<ivec4> influences;
    std::vector<unsigned int> indices;

    Attribute<vec3>*  posAttr;
    Attribute<vec3>*  normAttr;
    Attribute<vec4>*  weightAttr;
    Attribute<ivec4>* influenceAttr;
    IndexBuffer*      indexBuf;

    BenchMesh();
    ~BenchMesh();

    // Generate the mesh. After calling, uploads to GPU.
    void Generate(int targetVertices, int numBones, int numInfluences);

    // Re-upload current vertex data to GPU (after CPU skinning).
    void UpdateGPU();

    // Bind/unbind vertex attributes to shader locations.
    void Bind(int pos, int norm, int texCoord, int weight, int influence);
    void UnBind(int pos, int norm, int texCoord, int weight, int influence);
    void Draw();

    int VertexCount() const { return (int)positions.size(); }
    int IndexCount()  const { return (int)indices.size(); }

private:
    void Upload();
    void AssignWeights(int vertexIndex, float u, int numBones, int numInfluences);
};

#endif
