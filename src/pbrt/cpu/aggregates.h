// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_CPU_AGGREGATES_H
#define PBRT_CPU_AGGREGATES_H

#include <pbrt/pbrt.h>

#include <pbrt/cpu/primitive.h>
#include <pbrt/util/parallel.h>

#include <atomic>
#include <memory>
#include <vector>
#include <pbrt/options.h>

namespace pbrt {

Primitive CreateAccelerator(const std::string &name, std::vector<Primitive> prims,
                            const ParameterDictionary &parameters);

struct BVHBuildNode;
struct BVHPrimitive;
struct LinearBVHNode;
struct MortonPrimitive;

// BVHAggregate Definition
class BVHAggregate {
  public:
    // BVHAggregate Public Types
    enum class SplitMethod { SAH, HLBVH, Middle, EqualCounts };

    // BVHAggregate Public Methods
    BVHAggregate(std::vector<Primitive> p, int maxPrimsInNode = 1,
                 SplitMethod splitMethod = SplitMethod::SAH);

    static BVHAggregate *Create(std::vector<Primitive> prims,
                                const ParameterDictionary &parameters);

    Bounds3f Bounds() const;
    pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) const;
    bool IntersectP(const Ray &ray, Float tMax) const;

  private:
    // BVHAggregate Private Methods
    BVHBuildNode *buildRecursive(ThreadLocal<Allocator> &threadAllocators,
                                 pstd::span<BVHPrimitive> bvhPrimitives,
                                 std::atomic<int> *totalNodes,
                                 std::atomic<int> *orderedPrimsOffset,
                                 std::vector<Primitive> &orderedPrims
                                );
    BVHBuildNode *buildHLBVH(Allocator alloc,
                             const std::vector<BVHPrimitive> &primitiveInfo,
                             std::atomic<int> *totalNodes,
                             std::vector<Primitive> &orderedPrims);
    BVHBuildNode *emitLBVH(BVHBuildNode *&buildNodes,
                           const std::vector<BVHPrimitive> &primitiveInfo,
                           MortonPrimitive *mortonPrims, int nPrimitives, int *totalNodes,
                           std::vector<Primitive> &orderedPrims,
                           std::atomic<int> *orderedPrimsOffset, int bitIndex);
    BVHBuildNode *buildUpperSAH(Allocator alloc,
                                std::vector<BVHBuildNode *> &treeletRoots, int start,
                                int end, std::atomic<int> *totalNodes) const;
    int flattenBVH(BVHBuildNode *node, int *offset);

    // BVHAggregate Private Members
    int maxPrimsInNode;
    std::vector<Primitive> primitives;
    SplitMethod splitMethod;
    LinearBVHNode *nodes = nullptr;
};

struct WBVHBuildNode;
struct LinearWBVHNode;

class WBVHAggregate {
  public:
    // WBVHAggregate Public Methods
    WBVHAggregate(std::vector<Primitive> p, int maxPrimsInNode = 1);
    static WBVHAggregate *Create(std::vector<Primitive> prims, const ParameterDictionary &parameters);
    Bounds3f Bounds() const;
    pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) const;
    bool IntersectP(const Ray &ray, Float tMax) const;

    constexpr static std::size_t WIDTH = 6;


  private:
    // WBVHAggregate Private Methods
    WBVHBuildNode* buildRecursive(ThreadLocal<Allocator> &threadAllocators,
                                  pstd::span<BVHPrimitive> bvhPrimitives,
                                  std::atomic<int> *totalNodes,
                                  std::atomic<int> *orderedPrimsOffset,
                                  std::vector<Primitive> &orderedPrims,
                                  int depth = 0);
    int flattenWBVH(WBVHBuildNode *node, int locate, int offset);

    int maxPrimsInNode;
    Bounds3f gbounds;

    std::atomic<int> empty_nodes = 0;
    std::atomic<int> total_nodes = 0;
    std::atomic<int> one_nodes = 0;

    std::vector<Primitive> primitives;
    LinearWBVHNode *nodes = nullptr;
};

struct MEBVHBuildNode;
struct LinearMEBVHNode;
struct LinearMEBVHNode_4;
struct LinearOptimizedMEBVHNode;



class MEBVHAggregate {
  public:
    // MEBVHAggregate Public Methods
    MEBVHAggregate(std::vector<Primitive> p, int maxPrimsInNode = 1);
    static MEBVHAggregate *Create(std::vector<Primitive> prims, const ParameterDictionary &parameters);
    Bounds3f Bounds() const;
    pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) const;
    bool IntersectP(const Ray &ray, Float tMax) const;
    constexpr static std::size_t WIDTH = 6;

  private:
    // MEBVHAggregate Private Methods
    MEBVHBuildNode *buildRecursive(ThreadLocal<Allocator> &threadAllocators,
                                  pstd::span<BVHPrimitive> bvhPrimitives,
                                  std::atomic<int> *totalNodes,
                                  std::atomic<int> *orderedPrimsOffset,
                                  std::vector<Primitive> &orderedPrims,
                                  int depth = 0);
    int flattenMEBVH(MEBVHBuildNode *node, int locate, int offset);

    int maxPrimsInNode;
    Bounds3f gbounds;
    std::vector<Primitive> primitives;

    // change the evaluation setting by comment and uncomment
    // using LinearNode = LinearOptimizedMEBVHNode;
    using LinearNode = LinearMEBVHNode;
    // using LinearNode = LinearMEBVHNode_4;

    #define METRIC_TRAVERSAL
    // #define METRIC_RAYBOX
    // #define METRIC_DEQUANT
    LinearNode *nodes = nullptr;
};


struct KdTreeNode;
struct BoundEdge;

// KdTreeAggregate Definition
class KdTreeAggregate {
  public:
    // KdTreeAggregate Public Methods
    KdTreeAggregate(std::vector<Primitive> p, int isectCost = 5, int traversalCost = 1,
                    Float emptyBonus = 0.5, int maxPrims = 1, int maxDepth = -1);
    static KdTreeAggregate *Create(std::vector<Primitive> prims,
                                   const ParameterDictionary &parameters);
    pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) const;

    Bounds3f Bounds() const { return bounds; }

    bool IntersectP(const Ray &ray, Float tMax) const;

  private:
    // KdTreeAggregate Private Methods
    void buildTree(int nodeNum, const Bounds3f &bounds,
                   const std::vector<Bounds3f> &primBounds,
                   pstd::span<const int> primNums, int depth,
                   std::vector<BoundEdge> edges[3], pstd::span<int> prims0,
                   pstd::span<int> prims1, int badRefines);

    // KdTreeAggregate Private Members
    int isectCost, traversalCost, maxPrims;
    Float emptyBonus;
    std::vector<Primitive> primitives;
    std::vector<int> primitiveIndices;
    KdTreeNode *nodes;
    int nAllocedNodes, nextFreeNode;
    Bounds3f bounds;
};

}  // namespace pbrt

#endif  // PBRT_CPU_AGGREGATES_H
