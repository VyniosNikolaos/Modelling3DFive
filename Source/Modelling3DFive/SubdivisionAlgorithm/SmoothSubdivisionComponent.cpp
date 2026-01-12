// Fill out your copyright notice in the Description page of Project Settings.

#include "SmoothSubdivisionComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMesh/MeshTangents.h"
#include "MeshDescription.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMeshToMeshDescription.h"
#include "StaticMeshAttributes.h"

USmoothSubdivisionComponent::USmoothSubdivisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USmoothSubdivisionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache original mesh on begin play
    if (TargetMeshComponent)
    {
        OriginalMesh = GetMeshFromComponent();

        // Cache original static mesh if applicable
        OriginalStaticMesh = TargetMeshComponent->GetStaticMesh();

        // Cache materials from static mesh
        if (OriginalStaticMesh)
        {
            OriginalMaterials.Empty();
            for (int32 i = 0; i < OriginalStaticMesh->GetStaticMaterials().Num(); i++)
            {
                OriginalMaterials.Add(OriginalStaticMesh->GetMaterial(i));
            }
        }

        // Cache material overrides from component
        OriginalMaterialOverrides.Empty();
        for (int32 i = 0; i < TargetMeshComponent->GetNumMaterials(); i++)
        {
            OriginalMaterialOverrides.Add(TargetMeshComponent->GetMaterial(i));
        }
    }
}

bool USmoothSubdivisionComponent::ApplySubdivision()
{
    if (!TargetMeshComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("SmoothSubdivisionComponent: TargetMeshComponent is not set!"));
        return false;
    }

    // Get current mesh
    TSharedPtr<UE::Geometry::FDynamicMesh3> SourceMesh = GetMeshFromComponent();
    if (!SourceMesh.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("SmoothSubdivisionComponent: Failed to get mesh from component!"));
        return false;
    }

    // Apply subdivision
    UE::Geometry::FDynamicMesh3 ResultMesh = *SourceMesh;
    
    if (!SubdivideMesh(ResultMesh, SubdivisionLevel, SubdivisionScheme))
    {
        UE_LOG(LogTemp, Warning, TEXT("SmoothSubdivisionComponent: Subdivision failed!"));
        return false;
    }

    // Apply back to component
    return ApplyMeshToComponent(ResultMesh);
}

bool USmoothSubdivisionComponent::RestoreOriginalMesh()
{
    if (!TargetMeshComponent)
    {
        return false;
    }

    if (OriginalStaticMesh)
    {
        TargetMeshComponent->SetStaticMesh(OriginalStaticMesh);
        return true;
    }
    else if (OriginalMesh.IsValid())
    {
        return ApplyMeshToComponent(*OriginalMesh);
    }

    return false;
}

TSharedPtr<UE::Geometry::FDynamicMesh3> USmoothSubdivisionComponent::GetMeshFromComponent()
{
    if (!TargetMeshComponent)
    {
        return nullptr;
    }

    // Handle StaticMeshComponent
    if (TargetMeshComponent)
    {
        UStaticMesh* StaticMesh = TargetMeshComponent->GetStaticMesh();
        if (!StaticMesh)
        {
            return nullptr;
        }

        // Convert StaticMesh to DynamicMesh
        TSharedPtr<UE::Geometry::FDynamicMesh3> DynamicMesh = MakeShared<UE::Geometry::FDynamicMesh3>();
        DynamicMesh->EnableAttributes();

        FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0);
        if (!MeshDescription || MeshDescription->IsEmpty())
        {
            return nullptr;
        }

        // Create a copy for editing
        FMeshDescription SourceMeshDescription = *MeshDescription;

        // Convert to DynamicMesh for subdivision, preserving all attributes (hopefully)
        FMeshDescriptionToDynamicMesh Converter;
        Converter.bEnableOutputGroups = true;
        Converter.bVIDsFromNonManifoldMeshDescriptionAttr = false;
        Converter.bCalculateMaps = false;
        Converter.Convert(&SourceMeshDescription, *DynamicMesh);

        return DynamicMesh;
    }

    return nullptr;
}

bool USmoothSubdivisionComponent::ApplyMeshToComponent(const UE::Geometry::FDynamicMesh3& Mesh)
{
    if (!TargetMeshComponent)
    {
        return false;
    }

    // Handle StaticMeshComponent
    if (TargetMeshComponent)
    {
        // Convert DynamicMesh back to MeshDescription, preserving UVs and materials
        FMeshDescription MeshDescription;
        FStaticMeshAttributes Attributes(MeshDescription);
        Attributes.Register();

        FDynamicMeshToMeshDescription Converter;
        Converter.Convert(&Mesh, MeshDescription);
        
        // Create new static mesh
        UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(this);
        if (!NewStaticMesh)
        {
            return false;
        }

        // Add source model for LOD 0
        FStaticMeshSourceModel& SourceModel = NewStaticMesh->AddSourceModel();
        SourceModel.BuildSettings.bRecomputeNormals = bRecomputeNormals;
        SourceModel.BuildSettings.bRecomputeTangents = bRecomputeNormals;
        SourceModel.BuildSettings.bGenerateLightmapUVs = true;
        SourceModel.BuildSettings.bRemoveDegenerates = true;

        // Copy material slots from original mesh
        if (OriginalStaticMesh)
        {
            const TArray<FStaticMaterial>& OriginalStaticMaterials = OriginalStaticMesh->GetStaticMaterials();
            for (int32 i = 0; i < OriginalStaticMaterials.Num(); i++)
            {
                NewStaticMesh->GetStaticMaterials().Add(OriginalStaticMaterials[i]);
            }
        }

        // Set the mesh description
        FMeshDescription* MeshDescriptionPtr = NewStaticMesh->CreateMeshDescription(0);
        if (MeshDescriptionPtr)
        {
            *MeshDescriptionPtr = MoveTemp(MeshDescription);
            NewStaticMesh->CommitMeshDescription(0);
        }

        // Build the mesh
        NewStaticMesh->Build(false);
        NewStaticMesh->PostEditChange();

        // Apply to component
        TargetMeshComponent->SetStaticMesh(NewStaticMesh);

        // Restore material overrides on the component
        for (int32 i = 0; i < OriginalMaterialOverrides.Num(); i++)
        {
            if (OriginalMaterialOverrides[i])
            {
                TargetMeshComponent->SetMaterial(i, OriginalMaterialOverrides[i]);
            }
        }

        return true;
    }

    return false;
}

bool USmoothSubdivisionComponent::SubdivideMesh(UE::Geometry::FDynamicMesh3& Mesh, int32 Level, ESmoothSubdivisionScheme Scheme)
{
    if (!Mesh.IsCompact())
    {
        Mesh.CompactInPlace();
    }

    // Compute polygroups if needed for Catmull-Clark or Bilinear
    if (Scheme == ESmoothSubdivisionScheme::CatmullClark || Scheme == ESmoothSubdivisionScheme::Bilinear)
    {
        ComputePolyGroups(Mesh);
        
        // Check if we have valid polygroups
        if (!Mesh.HasTriangleGroups())
        {
            UE_LOG(LogTemp, Warning, TEXT("No polygroups found. Falling back to Loop subdivision."));
            Scheme = ESmoothSubdivisionScheme::Loop;
        }
    }

    // Apply subdivision iterations
    for (int32 i = 0; i < Level; i++)
    {
        switch (Scheme)
        {
        case ESmoothSubdivisionScheme::CatmullClark:
            ApplyCatmullClark(Mesh);
            break;
            
        case ESmoothSubdivisionScheme::Bilinear:
            ApplyBilinear(Mesh);
            break;
            
        case ESmoothSubdivisionScheme::Loop:
            ApplyLoop(Mesh);
            break;
        }

        UE_LOG(LogTemp, Log, TEXT("Subdivision iteration %d completed. New triangle count: %d"), 
               i + 1, Mesh.TriangleCount());
    }

    // Recompute normals if requested
    if (bRecomputeNormals)
    {
        UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
    }

    return true;
}

void USmoothSubdivisionComponent::ComputePolyGroups(UE::Geometry::FDynamicMesh3& Mesh)
{
    if (!Mesh.HasAttributes())
    {
        Mesh.EnableAttributes();
    }

    // If no polygroups exist, create them based on connected components
    if (!Mesh.HasTriangleGroups())
    {
        Mesh.EnableTriangleGroups();
    }

    // Use triangle normals and connectivity to create polygroups
    // This is a simplified approach. Ideally, we want more sophisticated quad detection
    TMap<int32, int32> TriToGroup;
    int32 CurrentGroup = 0;

    // Iterate through triangles and group coplanar adjacent triangles
    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        if (TriToGroup.Contains(TriID))
        {
            continue;
        }

        // Start new group
        TriToGroup.Add(TriID, CurrentGroup);
        
        // Flood fill to find connected coplanar triangles
        TArray<int32> Stack;
        Stack.Add(TriID);
        
        FVector3d BaseNormal = Mesh.GetTriNormal(TriID);
        
        while (Stack.Num() > 0)
        {
            int32 CurrentTri = Stack.Pop();
            TriToGroup.Add(CurrentTri, CurrentGroup);
            
            // Check adjacent triangles
            UE::Geometry::FIndex3i TriEdges = Mesh.GetTriEdges(CurrentTri);
            for (int j = 0; j < 3; j++)
            {
                UE::Geometry::FIndex2i EdgeTris = Mesh.GetEdgeT(TriEdges[j]);
                int32 AdjacentTri = (EdgeTris.A == CurrentTri) ? EdgeTris.B : EdgeTris.A;
                
                if (AdjacentTri != -1 && !TriToGroup.Contains(AdjacentTri))
                {
                    // Check if roughly coplanar
                    FVector3d AdjNormal = Mesh.GetTriNormal(AdjacentTri);
                    double Dot = BaseNormal.Dot(AdjNormal);
                    
                    if (Dot > 0.95) // ~18 degrees threshold
                    {
                        Stack.Add(AdjacentTri);
                    }
                }
            }
        }
        
        CurrentGroup++;
    }

    // Assign polygroups to mesh
    for (const TPair<int32, int32>& Pair : TriToGroup)
    {
        Mesh.SetTriangleGroup(Pair.Key, Pair.Value);
    }
}

void USmoothSubdivisionComponent::ApplyCatmullClark(UE::Geometry::FDynamicMesh3& Mesh)
{
    // Catmull-Clark subdivision with full topology changes
    // Treats each triangle as a face for subdivision purposes

    // Store original mesh data
    TArray<UE::Geometry::FIndex3i> OriginalTriangles;
    TMap<int32, int32> OriginalGroups;

    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        OriginalTriangles.Add(Mesh.GetTriangle(TriID));
        if (Mesh.HasTriangleGroups())
        {
            OriginalGroups.Add(TriID, Mesh.GetTriangleGroup(TriID));
        }
    }

    // Maps for new vertices
    TMap<int32, int32> FaceToFacePoint; // Triangle ID -> Face point vertex ID
    TMap<TPair<int32, int32>, int32> EdgeToEdgePoint; // Edge vertices -> Edge point vertex ID
    TMap<int32, FVector3d> NewVertexPositions; // Original vertex ID -> New position

    // 1. Create face points (centroid of each triangle)
    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        UE::Geometry::FIndex3i Tri = Mesh.GetTriangle(TriID);
        FVector3d FacePoint = (Mesh.GetVertex(Tri.A) + Mesh.GetVertex(Tri.B) + Mesh.GetVertex(Tri.C)) / 3.0;

        int32 FacePointID = Mesh.AppendVertex(FacePoint);
        FaceToFacePoint.Add(TriID, FacePointID);
    }

    // 2. Create edge points
    for (int32 EdgeID : Mesh.EdgeIndicesItr())
    {
        UE::Geometry::FIndex2i EdgeV = Mesh.GetEdgeV(EdgeID);
        TPair<int32, int32> EdgeKey(FMath::Min(EdgeV.A, EdgeV.B), FMath::Max(EdgeV.A, EdgeV.B));

        if (EdgeToEdgePoint.Contains(EdgeKey))
        {
            continue;
        }

        FVector3d EdgeMidpoint = 0.5 * (Mesh.GetVertex(EdgeV.A) + Mesh.GetVertex(EdgeV.B));
        UE::Geometry::FIndex2i EdgeTris = Mesh.GetEdgeT(EdgeID);

        FVector3d EdgePoint;
        if (EdgeTris.B != -1) // Interior edge
        {
            // Edge point = average of edge midpoint and adjacent face points
            int32 FacePoint0 = FaceToFacePoint[EdgeTris.A];
            int32 FacePoint1 = FaceToFacePoint[EdgeTris.B];

            EdgePoint = (EdgeMidpoint + Mesh.GetVertex(FacePoint0) + Mesh.GetVertex(FacePoint1)) / 3.0;
        }
        else // Boundary edge
        {
            EdgePoint = EdgeMidpoint;
        }

        int32 EdgePointID = Mesh.AppendVertex(EdgePoint);
        EdgeToEdgePoint.Add(EdgeKey, EdgePointID);
    }

    // 3. Compute new positions for original vertices
    for (int32 VertexID : Mesh.VertexIndicesItr())
    {
        // Skip if this is a newly created vertex
        if (FaceToFacePoint.FindKey(VertexID) || EdgeToEdgePoint.FindKey(VertexID))
        {
            continue;
        }

        TArray<int32> AdjacentTris;
        Mesh.GetVtxTriangles(VertexID, AdjacentTris);

        TArray<int32> AdjacentEdges;
        for (int32 EdgeID : Mesh.VtxEdgesItr(VertexID))
        {
            AdjacentEdges.Add(EdgeID);
        }

        bool bIsBoundary = false;
        for (int32 EdgeID : AdjacentEdges)
        {
            if (Mesh.GetEdgeT(EdgeID).B == -1)
            {
                bIsBoundary = true;
                break;
            }
        }

        if (bIsBoundary && bPreserveBoundaries)
        {
            NewVertexPositions.Add(VertexID, Mesh.GetVertex(VertexID));
        }
        else
        {
            // Catmull-Clark vertex rule: (Q + 2R + (n-3)S) / n
            // Q = average of face points, R = average of edge midpoints, S = original vertex, n = valence

            // Average of adjacent face points
            FVector3d Q(0, 0, 0);
            for (int32 TriID : AdjacentTris)
            {
                if (FaceToFacePoint.Contains(TriID))
                {
                    Q += Mesh.GetVertex(FaceToFacePoint[TriID]);
                }
            }
            Q /= FMath::Max(1, AdjacentTris.Num());

            // Average of edge midpoints
            FVector3d R(0, 0, 0);
            for (int32 EdgeID : AdjacentEdges)
            {
                UE::Geometry::FIndex2i EdgeV = Mesh.GetEdgeV(EdgeID);
                R += 0.5 * (Mesh.GetVertex(EdgeV.A) + Mesh.GetVertex(EdgeV.B));
            }
            R /= FMath::Max(1, AdjacentEdges.Num());

            FVector3d S = Mesh.GetVertex(VertexID);
            int32 n = AdjacentEdges.Num();

            FVector3d NewPos = (Q + 2.0 * R + (n - 3) * S) / n;
            NewVertexPositions.Add(VertexID, NewPos);
        }
    }

    // 4. Delete old triangles
    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        Mesh.RemoveTriangle(TriID, false);
    }

    // 5. Update original vertex positions
    for (const TPair<int32, FVector3d>& Pair : NewVertexPositions)
    {
        if (Mesh.IsVertex(Pair.Key))
        {
            Mesh.SetVertex(Pair.Key, Pair.Value);
        }
    }

    // 6. Create new triangles
    // For each original triangle, create 3 quads (represented as 6 triangles)
    int32 TriIndex = 0;
    for (const UE::Geometry::FIndex3i& OrigTri : OriginalTriangles)
    {
        int32 V0 = OrigTri.A;
        int32 V1 = OrigTri.B;
        int32 V2 = OrigTri.C;

        // Get face point
        int32 FP = FaceToFacePoint[TriIndex];

        // Get edge points
        TPair<int32, int32> Edge01(FMath::Min(V0, V1), FMath::Max(V0, V1));
        TPair<int32, int32> Edge12(FMath::Min(V1, V2), FMath::Max(V1, V2));
        TPair<int32, int32> Edge20(FMath::Min(V2, V0), FMath::Max(V2, V0));

        int32 E01 = EdgeToEdgePoint[Edge01];
        int32 E12 = EdgeToEdgePoint[Edge12];
        int32 E20 = EdgeToEdgePoint[Edge20];

        int32 GroupID = OriginalGroups.Contains(TriIndex) ? OriginalGroups[TriIndex] : 0;

        // Create 3 quads (each split into 2 triangles)
        // Quad 1: V0, E01, FP, E20
        Mesh.AppendTriangle(V0, E01, FP, GroupID);
        Mesh.AppendTriangle(V0, FP, E20, GroupID);

        // Quad 2: V1, E12, FP, E01
        Mesh.AppendTriangle(V1, E12, FP, GroupID);
        Mesh.AppendTriangle(V1, FP, E01, GroupID);

        // Quad 3: V2, E20, FP, E12
        Mesh.AppendTriangle(V2, E20, FP, GroupID);
        Mesh.AppendTriangle(V2, FP, E12, GroupID);

        TriIndex++;
    }
}

void USmoothSubdivisionComponent::ApplyBilinear(UE::Geometry::FDynamicMesh3& Mesh)
{
    // Bilinear subdivision - simple linear interpolation
    // Similar to Loop but without weighted averaging

    // Store original mesh data
    TArray<UE::Geometry::FIndex3i> OriginalTriangles;
    TMap<int32, int32> OriginalGroups;

    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        OriginalTriangles.Add(Mesh.GetTriangle(TriID));
        if (Mesh.HasTriangleGroups())
        {
            OriginalGroups.Add(TriID, Mesh.GetTriangleGroup(TriID));
        }
    }

    // Map from edge to new vertex ID
    TMap<TPair<int32, int32>, int32> EdgeToNewVertex;

    // 1. Create new edge vertices (simple midpoints, no smoothing)
    for (int32 EdgeID : Mesh.EdgeIndicesItr())
    {
        UE::Geometry::FIndex2i EdgeV = Mesh.GetEdgeV(EdgeID);
        TPair<int32, int32> EdgeKey(FMath::Min(EdgeV.A, EdgeV.B), FMath::Max(EdgeV.A, EdgeV.B));

        if (EdgeToNewVertex.Contains(EdgeKey))
        {
            continue;
        }

        // Simple midpoint - no weighting
        FVector3d NewPos = 0.5 * (Mesh.GetVertex(EdgeV.A) + Mesh.GetVertex(EdgeV.B));

        int32 NewVertexID = Mesh.AppendVertex(NewPos);
        EdgeToNewVertex.Add(EdgeKey, NewVertexID);
    }

    // 2. Delete old triangles
    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        Mesh.RemoveTriangle(TriID, false);
    }

    // 3. Create 4 new triangles for each original triangle
    // Original vertices stay in place (no smoothing)
    int32 TriIndex = 0;
    for (const UE::Geometry::FIndex3i& OrigTri : OriginalTriangles)
    {
        int32 V0 = OrigTri.A;
        int32 V1 = OrigTri.B;
        int32 V2 = OrigTri.C;

        // Get edge midpoint vertices
        TPair<int32, int32> Edge01(FMath::Min(V0, V1), FMath::Max(V0, V1));
        TPair<int32, int32> Edge12(FMath::Min(V1, V2), FMath::Max(V1, V2));
        TPair<int32, int32> Edge20(FMath::Min(V2, V0), FMath::Max(V2, V0));

        int32 M01 = EdgeToNewVertex[Edge01];
        int32 M12 = EdgeToNewVertex[Edge12];
        int32 M20 = EdgeToNewVertex[Edge20];

        int32 GroupID = OriginalGroups.Contains(TriIndex) ? OriginalGroups[TriIndex] : 0;

        // Create 4 new triangles
        Mesh.AppendTriangle(V0, M01, M20, GroupID);
        Mesh.AppendTriangle(M01, V1, M12, GroupID);
        Mesh.AppendTriangle(M20, M12, V2, GroupID);
        Mesh.AppendTriangle(M01, M12, M20, GroupID);

        TriIndex++;
    }
}

void USmoothSubdivisionComponent::ApplyLoop(UE::Geometry::FDynamicMesh3& Mesh)
{
    // Loop subdivision for triangle meshes with full topology changes

    // Store original mesh data
    TArray<UE::Geometry::FIndex3i> OriginalTriangles;
    TMap<int32, FVector3d> OriginalVertices;
    TMap<int32, int32> OriginalGroups;

    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        OriginalTriangles.Add(Mesh.GetTriangle(TriID));
        if (Mesh.HasTriangleGroups())
        {
            OriginalGroups.Add(TriID, Mesh.GetTriangleGroup(TriID));
        }
    }

    for (int32 VertexID : Mesh.VertexIndicesItr())
    {
        OriginalVertices.Add(VertexID, Mesh.GetVertex(VertexID));
    }

    // Map from edge (ordered pair) to new vertex ID
    TMap<TPair<int32, int32>, int32> EdgeToNewVertex;
    TMap<int32, FVector3d> NewVertexPositions;

    // 1. Compute new positions for original vertices
    for (int32 VertexID : Mesh.VertexIndicesItr())
    {
        TArray<int32> AdjacentEdges;
        for (int32 EdgeID : Mesh.VtxEdgesItr(VertexID))
        {
            AdjacentEdges.Add(EdgeID);
        }

        bool bIsBoundary = false;
        for (int32 EdgeID : AdjacentEdges)
        {
            if (Mesh.GetEdgeT(EdgeID).B == -1)
            {
                bIsBoundary = true;
                break;
            }
        }

        if (bIsBoundary && bPreserveBoundaries)
        {
            NewVertexPositions.Add(VertexID, Mesh.GetVertex(VertexID));
        }
        else if (bIsBoundary)
        {
            // Boundary vertex rule: 1/8 * (neighbors) + 3/4 * vertex
            FVector3d V0 = Mesh.GetVertex(VertexID);
            FVector3d BoundaryNeighborSum(0, 0, 0);
            int32 BoundaryNeighborCount = 0;

            for (int32 EdgeID : AdjacentEdges)
            {
                if (Mesh.GetEdgeT(EdgeID).B == -1)
                {
                    UE::Geometry::FIndex2i EdgeV = Mesh.GetEdgeV(EdgeID);
                    int32 OtherVertex = (EdgeV.A == VertexID) ? EdgeV.B : EdgeV.A;
                    BoundaryNeighborSum += Mesh.GetVertex(OtherVertex);
                    BoundaryNeighborCount++;
                }
            }

            if (BoundaryNeighborCount > 0)
            {
                NewVertexPositions.Add(VertexID, 0.75 * V0 + 0.125 * BoundaryNeighborSum);
            }
            else
            {
                NewVertexPositions.Add(VertexID, V0);
            }
        }
        else
        {
            // Interior vertex rule: (1 - n*beta) * V + beta * sum(neighbors)
            int32 n = AdjacentEdges.Num();
            double beta = (n == 3) ? 3.0 / 16.0 : 3.0 / (8.0 * n);

            FVector3d NeighborSum(0, 0, 0);
            for (int32 EdgeID : AdjacentEdges)
            {
                UE::Geometry::FIndex2i EdgeV = Mesh.GetEdgeV(EdgeID);
                int32 AdjacentVertex = (EdgeV.A == VertexID) ? EdgeV.B : EdgeV.A;
                NeighborSum += Mesh.GetVertex(AdjacentVertex);
            }

            FVector3d NewPos = (1.0 - n * beta) * Mesh.GetVertex(VertexID) + beta * NeighborSum;
            NewVertexPositions.Add(VertexID, NewPos);
        }
    }

    // 2. Create new edge vertices and compute their positions
    for (int32 EdgeID : Mesh.EdgeIndicesItr())
    {
        UE::Geometry::FIndex2i EdgeV = Mesh.GetEdgeV(EdgeID);
        TPair<int32, int32> EdgeKey(FMath::Min(EdgeV.A, EdgeV.B), FMath::Max(EdgeV.A, EdgeV.B));

        if (EdgeToNewVertex.Contains(EdgeKey))
        {
            continue;
        }

        FVector3d NewPos;
        UE::Geometry::FIndex2i EdgeTris = Mesh.GetEdgeT(EdgeID);

        if (EdgeTris.B != -1) // Interior edge
        {
            // Edge point formula: 3/8 * (v0 + v1) + 1/8 * (opposite0 + opposite1)
            UE::Geometry::FIndex3i TriA = Mesh.GetTriangle(EdgeTris.A);
            UE::Geometry::FIndex3i TriB = Mesh.GetTriangle(EdgeTris.B);

            // Find the third vertex in each triangle (not part of the edge)
            int32 Opposite0 = (TriA.A != EdgeV.A && TriA.A != EdgeV.B) ? TriA.A : (TriA.B != EdgeV.A && TriA.B != EdgeV.B) ? TriA.B : TriA.C;
            int32 Opposite1 = (TriB.A != EdgeV.A && TriB.A != EdgeV.B) ? TriB.A : (TriB.B != EdgeV.A && TriB.B != EdgeV.B) ? TriB.B : TriB.C;

            NewPos = 0.375 * (Mesh.GetVertex(EdgeV.A) + Mesh.GetVertex(EdgeV.B)) +
                     0.125 * (Mesh.GetVertex(Opposite0) + Mesh.GetVertex(Opposite1));
        }
        else // Boundary edge
        {
            // Midpoint for boundary
            NewPos = 0.5 * (Mesh.GetVertex(EdgeV.A) + Mesh.GetVertex(EdgeV.B));
        }

        int32 NewVertexID = Mesh.AppendVertex(NewPos);
        EdgeToNewVertex.Add(EdgeKey, NewVertexID);
    }

    // 3. Delete old triangles
    for (int32 TriID : Mesh.TriangleIndicesItr())
    {
        Mesh.RemoveTriangle(TriID, false);
    }

    // 4. Update original vertex positions
    for (const TPair<int32, FVector3d>& Pair : NewVertexPositions)
    {
        if (Mesh.IsVertex(Pair.Key))
        {
            Mesh.SetVertex(Pair.Key, Pair.Value);
        }
    }

    // 5. Create 4 new triangles for each original triangle
    int32 TriIndex = 0;
    for (const UE::Geometry::FIndex3i& OrigTri : OriginalTriangles)
    {
        int32 V0 = OrigTri.A;
        int32 V1 = OrigTri.B;
        int32 V2 = OrigTri.C;

        // Get edge midpoint vertices
        TPair<int32, int32> Edge01(FMath::Min(V0, V1), FMath::Max(V0, V1));
        TPair<int32, int32> Edge12(FMath::Min(V1, V2), FMath::Max(V1, V2));
        TPair<int32, int32> Edge20(FMath::Min(V2, V0), FMath::Max(V2, V0));

        int32 M01 = EdgeToNewVertex[Edge01];
        int32 M12 = EdgeToNewVertex[Edge12];
        int32 M20 = EdgeToNewVertex[Edge20];

        int32 GroupID = OriginalGroups.Contains(TriIndex) ? OriginalGroups[TriIndex] : 0;

        // Create 4 new triangles
        int32 NewTri0 = Mesh.AppendTriangle(V0, M01, M20, GroupID);
        int32 NewTri1 = Mesh.AppendTriangle(M01, V1, M12, GroupID);
        int32 NewTri2 = Mesh.AppendTriangle(M20, M12, V2, GroupID);
        int32 NewTri3 = Mesh.AppendTriangle(M01, M12, M20, GroupID);

        TriIndex++;
    }
}