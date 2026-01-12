/*
 * Chaikin3D.cpp
 *
 * Implementation of the Chaikin corner-cutting algorithm for 3D curve smoothing.
 *
 * This implementation extends the classic 2D Chaikin algorithm to work in three
 * dimensions. The algorithm operates identically to the 2D version, but applies
 * the corner-cutting subdivision to all three spatial coordinates (X, Y, Z).
 *
 * Reference: George Chaikin, "An algorithm for high-speed curve generation",
 *            Computer Graphics and Image Processing, 1974
 */

#include "Chaikin3D.h"

/**
 * Implementation of Chaikin's corner-cutting smoothing algorithm for 3D curves.
 *
 * The algorithm works through iterative subdivision in 3D space:
 *
 * Visual Example (1 iteration, shown in 1D for simplicity, but applies to X, Y, Z):
 * Input:  P0----------P1----------P2
 * Output: P0--Q0--R0--Q1--R1------P2
 *
 * Where (in 3D):
 * - Q0 = 0.75*P0 + 0.25*P1 (point at 1/4 along P0->P1, in all dimensions)
 * - R0 = 0.25*P0 + 0.75*P1 (point at 3/4 along P0->P1, in all dimensions)
 * - Q1 = 0.75*P1 + 0.25*P2 (point at 1/4 along P1->P2, in all dimensions)
 * - R1 = 0.25*P1 + 0.75*P2 (point at 3/4 along P1->P2, in all dimensions)
 * - P0 and P2 are preserved as endpoints
 *
 * Note that the original interior point P1 is "cut off" (removed), creating a smoother curve in 3D space.
 */
void FChaikinSmoothing3D::Smooth(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints, int32 Iterations)
{
	// Edge case handling: Return original points if smoothing is not applicable
	// - Less than 2 points: Cannot form a line segment to smooth
	// - Zero or negative iterations: No smoothing requested
	if (InPoints.Num() < 2 || Iterations <= 0)
	{
		OutPoints = InPoints;
		return;
	}

	// Working array that holds the current state of 3D points
	// This is updated with each iteration of the smoothing process
	TArray<FVector> CurrentPoints = InPoints;

	// Perform the requested number of smoothing iterations
	// Each iteration progressively smooths the 3D curve further
	for (int32 Iter = 0; Iter < Iterations; ++Iter)
	{
		// Array to hold the newly generated 3D points for this iteration
		TArray<FVector> NewPoints;

		// Pre-allocate memory for efficiency
		// Each iteration approximately doubles the number of points:
		// - We keep the first endpoint (1 point)
		// - Each edge generates 2 new points (N-1 edges × 2 points)
		// - We keep the last endpoint (1 point)
		// Total: 1 + (N-1)*2 + 1 = 2N points (approximately)
		NewPoints.Reserve(CurrentPoints.Num() * 2);

		// Preserve the starting point of the 3D curve
		// This ensures the smoothed curve begins at the same location as the original
		NewPoints.Add(CurrentPoints[0]);

		// Process each edge of the 3D curve
		// For N points, there are N-1 edges to process
		for (int32 i = 0; i < CurrentPoints.Num() - 1; ++i)
		{
			// Get the two 3D points defining the current edge
			const FVector& P0 = CurrentPoints[i];     // Start point of edge (X, Y, Z)
			const FVector& P1 = CurrentPoints[i + 1]; // End point of edge (X, Y, Z)

			// Calculate Q: The point at 1/4 (25%) along the edge from P0 to P1
			// Formula: Q = P0 + 0.25 * (P1 - P0) = 0.75*P0 + 0.25*P1
			// This point is closer to P0, "cutting" the corner near P0
			// The calculation applies to all three dimensions (X, Y, Z) simultaneously
			FVector Q = 0.75f * P0 + 0.25f * P1;

			// Calculate R: The point at 3/4 (75%) along the edge from P0 to P1
			// Formula: R = P0 + 0.75 * (P1 - P0) = 0.25*P0 + 0.75*P1
			// This point is closer to P1, "cutting" the corner near P1
			// The calculation applies to all three dimensions (X, Y, Z) simultaneously
			FVector R = 0.25f * P0 + 0.75f * P1;

			// Add both subdivision points to the new 3D curve
			// The order Q then R maintains the curve direction in 3D space
			NewPoints.Add(Q);
			NewPoints.Add(R);

			// Note: The original point P1 (except for the last point) is NOT added
			// This "cuts the corner" at P1, creating the smoothing effect in 3D space
		}

		// Preserve the ending point of the 3D curve
		// This ensures the smoothed curve ends at the same location as the original
		NewPoints.Add(CurrentPoints.Last());

		// Move the newly generated 3D points to CurrentPoints for the next iteration
		// MoveTemp is used for performance (avoids copying the array)
		CurrentPoints = MoveTemp(NewPoints);
	}

	// Move the final result to the output array
	// After all iterations, CurrentPoints contains the fully smoothed 3D curve
	OutPoints = MoveTemp(CurrentPoints);
}
