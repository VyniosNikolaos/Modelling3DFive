/*
 * ChaikinVisualizer3D.cpp
 *
 * Implementation of the 3D Chaikin algorithm visualization actor.
 *
 * This file provides a real-time, interactive demonstration of Chaikin's corner-cutting
 * algorithm in full 3D space using Unreal Engine's debug drawing system. It's designed
 * for educational purposes and rapid prototyping of 3D curve-based systems.
 *
 * The visualization approach:
 * - Uses DrawDebugLine for immediate-mode rendering (no mesh generation)
 * - Redraws every frame to maintain persistent visualization
 * - Supports both runtime and editor-time preview
 * - Color-codes original and smoothed curves for easy comparison
 * - Works with full 3D coordinates (X, Y, Z)
 */

#include "ChaikinVisualizer3D.h"
#include "Chaikin3D.h"
#include "DrawDebugHelpers.h"

/**
 * Constructor
 *
 * Sets up the actor with default visualization settings and a sample 3D control polygon.
 * The default polygon is a 3D helix-like shape that demonstrates the smoothing effect
 * in all three dimensions clearly when iterations are applied.
 */
AChaikinVisualizer3D::AChaikinVisualizer3D()
{
	// Enable ticking so the visualization updates every frame
	// This is necessary because DrawDebugLine with negative lifetime only draws for one frame
	PrimaryActorTick.bCanEverTick = true;

	// Initialize with a simple 3D path as a demonstration
	// This shape has points at varying heights and positions to show true 3D smoothing
	// Creates a zigzag pattern that rises and falls in Z
	ControlPoints = {
		FVector(0, 0, 0),         // Start at ground level
		FVector(100, 0, 100),     // Rise diagonally
		FVector(200, 0, 0),       // Drop back down
		FVector(300, 0, 100),     // Rise again
		FVector(400, 0, 0)        // End at ground level
	};
}

/**
 * BeginPlay - Called when the game starts or when the actor is spawned
 *
 * Inherits default behavior from AActor.
 * The visualization logic is handled in Tick() for continuous updates.
 */
void AChaikinVisualizer3D::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * Tick - Called every frame during gameplay
 *
 * Continuously redraws the 3D visualization to keep debug lines visible.
 * DrawDebugLine with negative lifetime (-1.0f) only persists for one frame,
 * so we must redraw every tick to maintain a continuous visualization.
 *
 * @param DeltaTime Time elapsed since the previous frame (unused in this implementation)
 */
void AChaikinVisualizer3D::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Redraw the visualization every frame
	DrawVisualization();
}

#if WITH_EDITOR
/**
 * PostEditChangeProperty - Called when a property is changed in the Unreal Editor
 *
 * This enables real-time preview functionality: whenever you modify any exposed
 * property (ControlPoints, Iterations, colors, etc.) in the Details panel,
 * the visualization immediately updates without requiring Play mode.
 *
 * This is invaluable for:
 * - Rapid iteration on 3D curve shapes
 * - Understanding the effect of different iteration counts in 3D
 * - Fine-tuning visual parameters
 * - Designing smooth 3D paths interactively
 *
 * @param PropertyChangedEvent Contains information about which property was modified
 */
void AChaikinVisualizer3D::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Update visualization immediately when any property changes in the editor
	DrawVisualization();
}

/**
 * PostEditMove - Called when the actor is moved, rotated, or scaled in the editor
 *
 * Ensures the 3D visualization updates to reflect the new transform, maintaining
 * accurate visual feedback as the actor is manipulated in the viewport.
 *
 * @param bFinished True if the move operation is complete, false if still in progress
 */
void AChaikinVisualizer3D::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	// Update 3D visualization when actor transform changes
	DrawVisualization();
}

/**
 * EditorTick - Called every frame in the editor viewport (even when not playing)
 *
 * This is the key function that enables 3D visualization without pressing Play.
 * It continuously redraws the debug lines every frame in the editor, making the
 * 3D curves visible at all times while working in the editor.
 *
 * Note: This only runs in the editor and has no performance impact on packaged games.
 *
 * @param DeltaSeconds Time elapsed since the last editor frame
 */
void AChaikinVisualizer3D::EditorTick(float DeltaSeconds)
{
	// Continuously redraw 3D visualization in editor viewport
	// This keeps debug lines visible without needing to press Play
	DrawVisualization();
}
#endif

/**
 * DrawVisualization - Core rendering method for the 3D Chaikin algorithm visualization
 *
 * This method performs three main tasks:
 * 1. Draws the original 3D control polygon with visible control points
 * 2. Applies the Chaikin smoothing algorithm in 3D
 * 3. Draws the resulting smoothed 3D curve
 *
 * The visualization uses different colors for original (red) and smoothed (green)
 * curves, making it easy to compare the input and output visually in 3D space.
 *
 * Note: This uses debug drawing primitives which are:
 * - Fast and simple to use
 * - Suitable for prototyping and debugging
 * - Not suitable for production rendering (use splines or meshes instead)
 * - Only visible in editor and development builds by default
 */
void AChaikinVisualizer3D::DrawVisualization()
{
	// Safety check: Need at least 2 points to form a line segment
	// With fewer points, there's nothing meaningful to visualize
	if (ControlPoints.Num() < 2)
	{
		return;
	}

	// Get the world context for drawing debug primitives
	// If World is null, we're likely in a context where drawing isn't available
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Get the actor's world position to transform local 3D points to world space
	// ControlPoints are defined in local 3D coordinates relative to this actor
	FVector ActorLocation = GetActorLocation();

	// ==================== STEP 1: Draw Original 3D Control Polygon ====================
	// Draw line segments connecting consecutive control points in 3D space
	// This shows the input shape before any smoothing is applied
	for (int32 i = 0; i < ControlPoints.Num() - 1; ++i)
	{
		// Convert local 3D control point to world 3D position
		// All three coordinates (X, Y, Z) are used directly
		FVector Start = ActorLocation + ControlPoints[i];
		FVector End = ActorLocation + ControlPoints[i + 1];

		// Draw the 3D line segment
		// Parameters:
		// - World: World context
		// - Start/End: Line endpoints in 3D world space
		// - Color: OriginalCurveColor (default red)
		// - bPersistentLines: false (not persistent between level loads)
		// - LifeTime: -1.0f (draw for exactly one frame)
		// - DepthPriority: 0 (default rendering order)
		// - Thickness: Configurable line thickness
		DrawDebugLine(World, Start, End, OriginalCurveColor, false, -1.0f, 0, LineThickness);
	}

	// Draw spheres at each 3D control point for visual reference
	// These markers make it easy to see where the original vertices are located in 3D space
	for (const FVector& Point : ControlPoints)
	{
		// Convert local 3D point to world 3D position
		FVector Position = ActorLocation + Point;

		// Draw a sphere at the 3D control point location
		// Parameters:
		// - World: World context
		// - Position: Sphere center in 3D world space
		// - Radius: Configurable (default 10.0 units for better visibility in 3D)
		// - Segments: 8 (sphere tessellation - lower = faster but less smooth)
		// - Color: Same as original curve for consistency
		// - bPersistentLines: false
		// - LifeTime: -1.0f (one frame)
		// - DepthPriority: 0
		// - Thickness: 1.0 (thin outline)
		DrawDebugSphere(World, Position, ControlPointRadius, 8, OriginalCurveColor, false, -1.0f, 0, 1.0f);
	}

	// ==================== STEP 2: Apply 3D Chaikin Smoothing ====================
	// Use the FChaikinSmoothing3D utility class to process the 3D control points
	// The algorithm operates on all three dimensions (X, Y, Z) equally
	TArray<FVector> SmoothedPoints;
	FChaikinSmoothing3D::Smooth(ControlPoints, SmoothedPoints, Iterations);

	// ==================== STEP 3: Draw Smoothed 3D Curve ====================
	// Draw the result of the 3D smoothing algorithm in a different color
	// This allows easy visual comparison with the original polygon in 3D space
	for (int32 i = 0; i < SmoothedPoints.Num() - 1; ++i)
	{
		// Convert smoothed local 3D points to world 3D positions
		FVector Start = ActorLocation + SmoothedPoints[i];
		FVector End = ActorLocation + SmoothedPoints[i + 1];

		// Draw the smoothed 3D curve segment
		// Uses SmoothedCurveColor (default green) to differentiate from original
		DrawDebugLine(World, Start, End, SmoothedCurveColor, false, -1.0f, 0, LineThickness);
	}

	// Note: We don't draw spheres on the smoothed curve points to avoid visual clutter
	// The smoothed curve typically has many more points than the original, so marking
	// all of them would make the visualization harder to read, especially in 3D space
}
