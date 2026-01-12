/*
 * Chaikin.h
 *
 * Implementation of Chaikin's corner-cutting algorithm for curve smoothing.
 *
 * The Chaikin algorithm is a simple subdivision scheme that smooths polygonal curves
 * by iteratively replacing each line segment with two new segments. This creates
 * progressively smoother curves with each iteration.
 *
 * Algorithm Overview:
 * - For each edge between two points (P0, P1):
 *   - Calculate Q at 25% along the edge: Q = 0.75*P0 + 0.25*P1
 *   - Calculate R at 75% along the edge: R = 0.25*P0 + 0.75*P1
 *   - Replace the original edge with Q and R
 * - The number of points approximately doubles with each iteration
 * - Endpoints are preserved to maintain the curve's start and end positions
 *
 * Properties:
 * - Produces smooth, aesthetically pleasing curves
 * - Converges to a quadratic B-spline in the limit
 * - Each iteration doubles the number of points (approximately)
 * - Simple and fast to compute
 * - Does not require solving systems of equations
 */

#pragma once
#include "CoreMinimal.h"

/**
 * FChaikinSmoothing
 *
 * Static utility class for applying Chaikin's corner-cutting algorithm to 2D point arrays.
 * This class provides a simple interface for smoothing polygonal curves and is particularly
 * useful for:
 * - Smoothing user-drawn curves
 * - Creating aesthetic paths and trajectories
 * - Generating smooth approximations of polygonal shapes
 * - Real-time curve editing and manipulation
 */
class FChaikinSmoothing
{
public:
	/**
	 * Applies Chaikin's corner-cutting algorithm to smooth a curve defined by 2D points.
	 *
	 * The algorithm works by iteratively subdividing line segments. Each iteration:
	 * 1. Creates two new points for each original edge
	 * 2. These points are placed at 1/4 and 3/4 positions along each edge
	 * 3. The original interior points are removed, "cutting the corners"
	 * 4. Start and end points are preserved
	 *
	 * @param InPoints    The input array of 2D points defining the original curve.
	 *                    Must contain at least 2 points for smoothing to occur.
	 *                    The points are expected to be in sequential order along the curve.
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
	 * @note The algorithm preserves the general shape while removing sharp corners
	 * @note Endpoints are always preserved to maintain curve connectivity
	 */
	static void Smooth(const TArray<FVector2D>& InPoints, TArray<FVector2D>& OutPoints, int32 Iterations);
};