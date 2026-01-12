/*
 * Chaikin3D.h
 *
 * Implementation of Chaikin's corner-cutting algorithm for 3D curve smoothing.
 *
 * The Chaikin algorithm is a simple subdivision scheme that smooths polygonal curves
 * by iteratively replacing each line segment with two new segments. This 3D version
 * operates on all three spatial dimensions (X, Y, Z), smoothing sharp angles in any
 * direction in 3D space.
 *
 * Algorithm Overview:
 * - For each edge between two points (P0, P1) in 3D space:
 *   - Calculate Q at 25% along the edge: Q = 0.75*P0 + 0.25*P1
 *   - Calculate R at 75% along the edge: R = 0.25*P0 + 0.75*P1
 *   - Replace the original edge with Q and R
 * - The number of points approximately doubles with each iteration
 * - Endpoints are preserved to maintain the curve's start and end positions
 *
 * Properties:
 * - Produces smooth, aesthetically pleasing 3D curves
 * - Converges to a quadratic B-spline in the limit
 * - Each iteration doubles the number of points (approximately)
 * - Simple and fast to compute
 * - Works identically in all three spatial dimensions
 * - Does not require solving systems of equations
 *
 * Use Cases:
 * - Smoothing 3D paths and trajectories
 * - Creating smooth camera movement paths
 * - Generating organic 3D curves from control points
 * - Smoothing skeletal animation paths
 * - Path planning for autonomous agents
 */

#pragma once
#include "CoreMinimal.h"

/**
 * FChaikinSmoothing3D
 *
 * Static utility class for applying Chaikin's corner-cutting algorithm to 3D point arrays.
 * This class provides a simple interface for smoothing 3D polygonal curves and is particularly
 * useful for:
 * - Smoothing user-defined 3D paths
 * - Creating aesthetic trajectories in 3D space
 * - Generating smooth approximations of polygonal 3D shapes
 * - Real-time 3D curve editing and manipulation
 * - Camera path smoothing
 */
class FChaikinSmoothing3D
{
public:
	/**
	 * Applies Chaikin's corner-cutting algorithm to smooth a 3D curve defined by 3D points.
	 *
	 * The algorithm works by iteratively subdividing line segments in 3D space. Each iteration:
	 * 1. Creates two new points for each original edge
	 * 2. These points are placed at 1/4 and 3/4 positions along each edge
	 * 3. The original interior points are removed, "cutting the corners"
	 * 4. Start and end points are preserved
	 *
	 * The algorithm treats all three dimensions (X, Y, Z) equally, smoothing the curve
	 * uniformly in 3D space regardless of the curve's orientation.
	 *
	 * @param InPoints    The input array of 3D points defining the original curve.
	 *                    Must contain at least 2 points for smoothing to occur.
	 *                    The points are expected to be in sequential order along the curve.
	 *                    Each point has X, Y, and Z coordinates in world or local space.
	 *
	 * @param OutPoints   The output array that will contain the smoothed curve points.
	 *                    This array is cleared and populated with new points.
	 *                    The number of points will be approximately (InPoints.Num() * 2^Iterations).
	 *                    If smoothing cannot be performed, OutPoints will be a copy of InPoints.
	 *
	 * @param Iterations  The number of smoothing iterations to perform.
	 *                    - 0 or negative: No smoothing (OutPoints = InPoints)
	 *                    - 1: Single subdivision (moderate smoothing)
	 *                    - 2-3: Good balance of smoothness and performance
	 *                    - 4+: Very smooth but many points (use with caution)
	 *                    Each iteration approximately doubles the point count.
	 *
	 * @note Time Complexity: O(n * 2^iterations) where n is the number of input points
	 * @note Space Complexity: O(n * 2^iterations) for storing the result
	 * @note The algorithm preserves the general 3D shape while removing sharp corners
	 * @note Endpoints are always preserved to maintain curve connectivity
	 * @note All three spatial dimensions are smoothed equally
	 * @note The smoothing does not depend on the curve's orientation in space
	 */
	static void Smooth(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints, int32 Iterations);
};
