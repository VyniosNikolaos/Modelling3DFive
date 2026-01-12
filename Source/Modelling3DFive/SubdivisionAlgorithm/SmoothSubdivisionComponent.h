// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "SmoothSubdivisionComponent.generated.h"

/**
 * Enumeration of available subdivision surface algorithms
 *
 * Each algorithm has different characteristics and is suited for different types of meshes:
 *
 * - Catmull-Clark: Best for hard-surface modeling with quad-dominant topology
 *   Creates smooth C² continuous surfaces, treats each triangle as a face
 *   More controlled smoothing, good for architectural/mechanical shapes
 *
 * - Bilinear: Simple linear interpolation without smoothing
 *   Just adds geometry without changing the shape
 *   Useful for increasing polygon count while maintaining sharp features
 *
 * - Loop: Best for organic shapes with triangle meshes
 *   Creates very smooth C² continuous surfaces using weighted averaging
 *   More aggressive smoothing, ideal for characters and organic forms
 */
UENUM(BlueprintType)
enum class ESmoothSubdivisionScheme : uint8
{
    CatmullClark    UMETA(DisplayName = "Catmull-Clark"),
    Bilinear        UMETA(DisplayName = "Bilinear"),
    Loop            UMETA(DisplayName = "Loop")
};

/**
 * USmoothSubdivisionComponent
 *
 * A component that applies subdivision surface algorithms to Static Mesh Components at runtime.
 * Subdivision increases the polygon count of a mesh while smoothing its surface, creating
 * higher quality geometry from low-poly base meshes.
 *
 * Key Features:
 * - Three subdivision algorithms: Catmull-Clark, Bilinear, and Loop
 * - Configurable subdivision levels (1-5 iterations)
 * - Preserves UV coordinates, materials, and textures
 * - Optional boundary preservation to keep mesh edges sharp
 * - Ability to restore original mesh
 *
 * Usage:
 * 1. Add this component to an Actor with a Static Mesh Component
 * 2. Set TargetMeshComponent to reference the mesh you want to subdivide
 * 3. Configure subdivision parameters (level, scheme, etc.)
 * 4. Call ApplySubdivision() to apply the effect (e.g., on BeginPlay or button press)
 * 5. Call RestoreOriginalMesh() to undo the subdivision
 *
 * Technical Details:
 * - Internally uses UE's DynamicMesh3 for geometry processing
 * - Creates new Static Mesh assets at runtime (not saved to disk)
 * - Original mesh data is cached on BeginPlay for restoration
 * - Material slots and component overrides are preserved
 *
 * Performance Notes:
 * - Each subdivision level approximately quadruples triangle count
 * - Level 1: ~4x triangles, Level 2: ~16x, Level 3: ~64x, etc.
 * - Higher levels can significantly impact performance
 * - Subdivision is CPU-intensive and happens on the game thread
 */
UCLASS(meta=(BlueprintSpawnableComponent), Category="Mesh Subdivision")
class MODELLING3DFIVE_API USmoothSubdivisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USmoothSubdivisionComponent();

    // ========================================
    // SUBDIVISION PARAMETERS
    // ========================================

    /**
     * Number of subdivision iterations to apply (1-5)
     *
     * Each level roughly quadruples the triangle count:
     * - Level 1: ~4x triangles
     * - Level 2: ~16x triangles
     * - Level 3: ~64x triangles
     * - Level 4: ~256x triangles
     * - Level 5: ~1024x triangles
     *
     * Higher levels create smoother results but dramatically increase vertex count
     * and can impact rendering performance. Start with 1-2 for most cases.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subdivision", meta=(ClampMin=1, ClampMax=5, UIMin=1, UIMax=5))
    int32 SubdivisionLevel = 2;

    /**
     * The subdivision algorithm to use
     *
     * Catmull-Clark: Best for hard surfaces, creates quad-like topology
     * Bilinear: Simple subdivision without smoothing, preserves shape
     * Loop: Best for organic shapes, creates very smooth surfaces
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subdivision")
    ESmoothSubdivisionScheme SubdivisionScheme = ESmoothSubdivisionScheme::CatmullClark;

    /**
     * Whether to preserve boundary edges during subdivision
     *
     * When true: Vertices on mesh boundaries (open edges) are not smoothed,
     *           keeping the silhouette sharp and preventing shrinkage
     * When false: All vertices are smoothed, which may cause boundary edges
     *            to pull inward and the mesh to shrink slightly
     *
     * Generally should be true to maintain the original mesh shape and size.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subdivision")
    bool bPreserveBoundaries = true;

    /**
     * Whether to recalculate vertex normals after subdivision
     *
     * When true: Computes smooth vertex normals for better lighting
     * When false: Keeps whatever normals result from subdivision
     *
     * Usually should be true for smooth-looking results.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subdivision")
    bool bRecomputeNormals = true;

    /**
     * The Static Mesh Component to apply subdivision to
     *
     * This is the mesh that will be subdivided when ApplySubdivision() is called.
     * The component must have a valid Static Mesh assigned to it.
     *
     * Note: The original mesh is cached on BeginPlay, so this should be set
     *       before the game starts if using BeginPlay for caching.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subdivision")
    UStaticMeshComponent* TargetMeshComponent;

    // ========================================
    // PUBLIC API
    // ========================================

    /**
     * Apply subdivision to the target mesh component
     *
     * This performs the following operations:
     * 1. Extracts the mesh geometry from TargetMeshComponent
     * 2. Converts it to a DynamicMesh3 for processing
     * 3. Applies the selected subdivision algorithm N times (SubdivisionLevel)
     * 4. Converts back to a Static Mesh
     * 5. Restores materials and UV coordinates
     * 6. Applies the new mesh to the component
     *
     * @return true if subdivision succeeded, false if it failed
     *
     * Common failure reasons:
     * - TargetMeshComponent is null or invalid
     * - No Static Mesh assigned to the component
     * - Mesh has no valid geometry
     * - Out of memory (mesh too large after subdivision)
     */
    UFUNCTION(BlueprintCallable, Category="Subdivision")
    bool ApplySubdivision();

    /**
     * Restore the original mesh before subdivision
     *
     * This reverts the mesh back to its state before ApplySubdivision() was called.
     * The original mesh is cached in BeginPlay(), so this component must have been
     * active when the level started.
     *
     * @return true if restoration succeeded, false if no original mesh was cached
     *
     * Note: This only works if the original mesh was cached during BeginPlay.
     *       If ApplySubdivision was called before BeginPlay, restoration may fail.
     */
    UFUNCTION(BlueprintCallable, Category="Subdivision")
    bool RestoreOriginalMesh();

protected:
    /**
     * Called when the game starts or when the component is spawned
     *
     * Caches the original mesh data including:
     * - Mesh geometry (vertices, triangles, UVs)
     * - Static Mesh reference
     * - Material slots from the Static Mesh
     * - Material overrides from the component
     *
     * This cached data is used for RestoreOriginalMesh() and for preserving
     * materials when applying subdivision.
     */
    virtual void BeginPlay() override;

private:
    // ========================================
    // CACHED ORIGINAL MESH DATA
    // ========================================

    /**
     * Cached copy of the original mesh geometry in DynamicMesh3 format
     * Used for potential restoration or reference during subdivision
     */
    TSharedPtr<UE::Geometry::FDynamicMesh3> OriginalMesh;

    /**
     * Reference to the original Static Mesh asset before subdivision
     * Used to restore the mesh with RestoreOriginalMesh()
     */
    UPROPERTY()
    UStaticMesh* OriginalStaticMesh = nullptr;

    /**
     * Material slots from the original Static Mesh asset
     * These are copied to the new subdivided mesh to preserve appearance
     */
    UPROPERTY()
    TArray<UMaterialInterface*> OriginalMaterials;

    /**
     * Material overrides from the Static Mesh Component
     * These are reapplied after subdivision to preserve any per-instance materials
     */
    UPROPERTY()
    TArray<UMaterialInterface*> OriginalMaterialOverrides;

    // ========================================
    // SUBDIVISION ALGORITHM IMPLEMENTATIONS
    // ========================================

    /**
     * Main subdivision function that applies the algorithm iteratively
     *
     * @param Mesh - The DynamicMesh3 to subdivide (modified in place)
     * @param Level - Number of subdivision iterations to perform
     * @param Scheme - Which subdivision algorithm to use
     * @return true if subdivision succeeded, false otherwise
     *
     * This function calls the appropriate subdivision method (ApplyCatmullClark,
     * ApplyBilinear, or ApplyLoop) multiple times based on the Level parameter.
     */
    bool SubdivideMesh(UE::Geometry::FDynamicMesh3& Mesh, int32 Level, ESmoothSubdivisionScheme Scheme);

    /**
     * Catmull-Clark Subdivision Algorithm
     *
     * Process for each triangle:
     * 1. Create face point at triangle centroid
     * 2. Create edge points (avg of edge midpoint + adjacent face points)
     * 3. Update original vertices using formula: (Q + 2R + (n-3)S) / n
     *    where Q = avg face points, R = avg edge midpoints, S = original vertex, n = valence
     * 4. Connect points to create 3 quads per triangle (represented as 6 triangles)
     *
     * Properties:
     * - Creates C² smooth surfaces (except at extraordinary points which are C¹)
     * - Best for hard-surface modeling and quad-dominant topology
     * - More controlled smoothing than Loop
     * - Each triangle becomes 6 triangles (3 quads split into triangles)
     *
     * @param Mesh - The mesh to subdivide (modified in place)
     */
    void ApplyCatmullClark(UE::Geometry::FDynamicMesh3& Mesh);

    /**
     * Bilinear Subdivision Algorithm
     *
     * Process for each triangle:
     * 1. Create edge midpoints (simple 50/50 average)
     * 2. Connect midpoints to original vertices
     * 3. Create 4 new triangles without moving original vertices
     *
     * Properties:
     * - No smoothing - just adds geometry
     * - Original vertices stay in place
     * - Preserves sharp features and mesh shape exactly
     * - Each triangle becomes 4 triangles
     * - Useful for increasing polygon count for tessellation/displacement
     *
     * @param Mesh - The mesh to subdivide (modified in place)
     */
    void ApplyBilinear(UE::Geometry::FDynamicMesh3& Mesh);

    /**
     * Loop Subdivision Algorithm (Charles Loop, 1987)
     *
     * Process for each triangle:
     * 1. Compute new edge points using weighted formula:
     *    3/8 * (v0 + v1) + 1/8 * (opposite0 + opposite1)
     * 2. Update original vertices using beta weights:
     *    Interior: (1 - n*β)*V + β*sum(neighbors) where β = 3/16 for n=3, or 3/(8n)
     *    Boundary: 3/4*V + 1/8*sum(boundary neighbors)
     * 3. Connect points to create 4 new triangles per original triangle
     *
     * Properties:
     * - Creates C² smooth surfaces everywhere
     * - Best for organic shapes and character modeling
     * - More aggressive smoothing than Catmull-Clark
     * - Specifically designed for triangle meshes
     * - Each triangle becomes 4 triangles
     *
     * @param Mesh - The mesh to subdivide (modified in place)
     */
    void ApplyLoop(UE::Geometry::FDynamicMesh3& Mesh);

    // ========================================
    // HELPER FUNCTIONS
    // ========================================

    /**
     * Extract mesh geometry from the target Static Mesh Component
     *
     * This converts the Static Mesh to a DynamicMesh3 format for processing.
     * Also preserves UV coordinates and material IDs during conversion.
     *
     * @return Shared pointer to the extracted DynamicMesh3, or nullptr on failure
     */
    TSharedPtr<UE::Geometry::FDynamicMesh3> GetMeshFromComponent();

    /**
     * Apply a DynamicMesh3 back to the target component as a new Static Mesh
     *
     * This performs the following:
     * 1. Converts DynamicMesh3 back to MeshDescription
     * 2. Creates a new Static Mesh asset (runtime only, not saved)
     * 3. Copies material slots from original mesh
     * 4. Builds render data with proper normals and tangents
     * 5. Applies the new mesh to the component
     * 6. Restores material overrides
     *
     * @param Mesh - The DynamicMesh3 to apply to the component
     * @return true if successful, false otherwise
     */
    bool ApplyMeshToComponent(const UE::Geometry::FDynamicMesh3& Mesh);

    /**
     * Compute triangle groups (polygroups) for Catmull-Clark subdivision
     *
     * Groups coplanar adjacent triangles together to simulate quads.
     * Uses surface normals to detect which triangles should be grouped.
     *
     * Algorithm:
     * - Flood-fill from each triangle
     * - Add adjacent triangles if normals are similar (dot product > 0.95, ~18 degrees)
     * - Each group represents a "virtual quad" for Catmull-Clark
     *
     * @param Mesh - The mesh to compute groups for (modified in place)
     */
    void ComputePolyGroups(UE::Geometry::FDynamicMesh3& Mesh);
};
