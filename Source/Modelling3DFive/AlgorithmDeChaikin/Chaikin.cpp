/*
 * Chaikin.cpp
 *
 * Implementation of the Chaikin corner-cutting algorithm for curve smoothing.
 *
 * Reference: George Chaikin, "An algorithm for high-speed curve generation",
 *            Computer Graphics and Image Processing, 1974
 */

#include "Chaikin.h"

/**
 * Implementation of Chaikin's corner-cutting smoothing algorithm.
 *
 * The algorithm works through iterative subdivision:
 *
 * Visual Example (1 iteration):
 * Input:  P0----------P1----------P2
 * Output: P0--Q0--R0--Q1--R1------P2
 *
 * Where:
 * - Q0 = 0.75*P0 + 0.25*P1 (point at 1/4 along P0->P1)
 * - R0 = 0.25*P0 + 0.75*P1 (point at 3/4 along P0->P1)
 * - Q1 = 0.75*P1 + 0.25*P2 (point at 1/4 along P1->P2)
 * - R1 = 0.25*P1 + 0.75*P2 (point at 3/4 along P1->P2)
 * - P0 and P2 are preserved as endpoints
 *
 * Note that the original interior point P1 is "cut off" (removed), creating a smoother curve.
 */
void FChaikinSmoothing::Smooth(const TArray<FVector2D>& InPoints, TArray<FVector2D>& OutPoints, int32 Iterations)
{
	// Edge case handling: Return original points if smoothing is not applicable
	// - Less than 2 points: Cannot form a line segment to smooth
	// - Zero or negative iterations: No smoothing requested
	if (InPoints.Num() < 2 || Iterations <= 0)
	{
		OutPoints = InPoints;
		return;
	}

	// Working array that holds the current state of points
	// This is updated with each iteration of the smoothing process
	TArray<FVector2D> CurrentPoints = InPoints;

	// Perform the requested number of smoothing iterations
	// Each iteration progressively smooths the curve further
	for (int32 Iter = 0; Iter < Iterations; ++Iter)
	{
		// Array to hold the newly generated points for this iteration
		TArray<FVector2D> NewPoints;

		// Pre-allocate memory for efficiency
		// Each iteration approximately doubles the number of points:
		// - We keep the first endpoint (1 point)
		// - Each edge generates 2 new points (N-1 edges × 2 points)
		// - We keep the last endpoint (1 point)
		// Total: 1 + (N-1)*2 + 1 = 2N points (approximately)
		NewPoints.Reserve(CurrentPoints.Num() * 2);

		// Preserve the starting point of the curve
		// This ensures the smoothed curve begins at the same location as the original
		NewPoints.Add(CurrentPoints[0]);

		// Process each edge of the curve
		// For N points, there are N-1 edges to process
		for (int32 i = 0; i < CurrentPoints.Num() - 1; ++i)
		{
			// Get the two points defining the current edge
			const FVector2D& P0 = CurrentPoints[i];     // Start point of edge
			const FVector2D& P1 = CurrentPoints[i + 1]; // End point of edge

			// Calculate Q: The point at 1/4 (25%) along the edge from P0 to P1
			// Formula: Q = P0 + 0.25 * (P1 - P0) = 0.75*P0 + 0.25*P1
			// This point is closer to P0, "cutting" the corner near P0
			FVector2D Q = 0.75f * P0 + 0.25f * P1;

			// Calculate R: The point at 3/4 (75%) along the edge from P0 to P1
			// Formula: R = P0 + 0.75 * (P1 - P0) = 0.25*P0 + 0.75*P1
			// This point is closer to P1, "cutting" the corner near P1
			FVector2D R = 0.25f * P0 + 0.75f * P1;

			// Add both subdivision points to the new curve
			// The order Q then R maintains the curve direction
			NewPoints.Add(Q);
			NewPoints.Add(R);

			// Note: The original point P1 (except for the last point) is NOT added
			// This "cuts the corner" at P1, creating the smoothing effect
		}

		// Preserve the ending point of the curve
		// This ensures the smoothed curve ends at the same location as the original
		NewPoints.Add(CurrentPoints.Last());

		// Move the newly generated points to CurrentPoints for the next iteration
		// MoveTemp is used for performance (avoids copying the array)
		CurrentPoints = MoveTemp(NewPoints);
	}

	// Move the final result to the output array
	// After all iterations, CurrentPoints contains the fully smoothed curve
	OutPoints = MoveTemp(CurrentPoints);
}