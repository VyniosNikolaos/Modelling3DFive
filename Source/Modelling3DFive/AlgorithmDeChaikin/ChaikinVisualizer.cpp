/*
 * ChaikinVisualizer.cpp
 *
 * Implementation of the Chaikin algorithm visualization actor.
 *
 * This file provides a real-time, interactive demonstration of Chaikin's corner-cutting
 * algorithm using Unreal Engine's debug drawing system. It's designed for educational
 * purposes and rapid prototyping of curve-based systems.
 *
 * The visualization approach:
 * - Uses DrawDebugLine for immediate-mode rendering (no mesh generation)
 * - Redraws every frame to maintain persistent visualization
 * - Supports both runtime and editor-time preview
 * - Color-codes original and smoothed curves for easy comparison
 */

#include "ChaikinVisualizer.h"
#include "Chaikin.h"
#include "DrawDebugHelpers.h"

/**
 * Constructor
 *
 * Sets up the actor with default visualization settings and a sample control polygon.
 * The default polygon is a closed square (200x200 units) that demonstrates the
 * smoothing effect clearly when iterations are applied.
 */
AChaikinVisualizer::AChaikinVisualizer()
{
	// Enable ticking so the visualization updates every frame
	// This is necessary because DrawDebugLine with negative lifetime only draws for one frame
	PrimaryActorTick.bCanEverTick = true;

	// Initialize with a simple closed square as a demonstration
	// This shape has sharp corners which makes the smoothing effect very visible
	// Points form a 200x200 unit square with the first point repeated to close the loop
	ControlPoints = {
		FVector2D(0, 0),      // Bottom-left corner
		FVector2D(200, 0),    // Bottom-right corner
		FVector2D(200, 200),  // Top-right corner
		FVector2D(0, 200),    // Top-left corner
		FVector2D(0, 0)       // Back to start (creates closed shape)
	};
}

/**
 * BeginPlay - Called when the game starts or when the actor is spawned
 *
 * Inherits default behavior from AActor.
 * The visualization logic is handled in Tick() for continuous updates.
 */
void AChaikinVisualizer::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * Tick - Called every frame during gameplay
 *
 * Continuously redraws the visualization to keep debug lines visible.
 * DrawDebugLine with negative lifetime (-1.0f) only persists for one frame,
 * so we must redraw every tick to maintain a continuous visualization.
 *
 * @param DeltaTime Time elapsed since the previous frame (unused in this implementation)
 */
void AChaikinVisualizer::Tick(float DeltaTime)
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
 * - Rapid iteration on curve shapes
 * - Understanding the effect of different iteration counts
 * - Fine-tuning visual parameters
 *
 * @param PropertyChangedEvent Contains information about which property was modified
 */
void AChaikinVisualizer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Update visualization immediately when any property changes in the editor
	DrawVisualization();
}

/**
 * PostEditMove - Called when the actor is moved, rotated, or scaled in the editor
 *
 * Ensures the visualization updates to reflect the new transform, maintaining
 * accurate visual feedback as the actor is manipulated in the viewport.
 *
 * @param bFinished True if the move operation is complete, false if still in progress
 */
void AChaikinVisualizer::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	// Update visualization when actor transform changes
	DrawVisualization();
}

/**
 * EditorTick - Called every frame in the editor viewport (even when not playing)
 *
 * This is the key function that enables visualization without pressing Play.
 * It continuously redraws the debug lines every frame in the editor, making the
 * curves visible at all times while working in the editor.
 *
 * Note: This only runs in the editor and has no performance impact on packaged games.
 *
 * @param DeltaSeconds Time elapsed since the last editor frame
 */
void AChaikinVisualizer::EditorTick(float DeltaSeconds)
{
	// Continuously redraw visualization in editor viewport
	// This keeps debug lines visible without needing to press Play
	DrawVisualization();
}
#endif

/**
 * DrawVisualization - Core rendering method for the Chaikin algorithm visualization
 *
 * This method performs three main tasks:
 * 1. Draws the original control polygon with visible control points
 * 2. Applies the Chaikin smoothing algorithm
 * 3. Draws the resulting smoothed curve
 *
 * The visualization uses different colors for original (red) and smoothed (green)
 * curves, making it easy to compare the input and output visually.
 *
 * Note: This uses debug drawing primitives which are:
 * - Fast and simple to use
 * - Suitable for prototyping and debugging
 * - Not suitable for production rendering (use splines or meshes instead)
 * - Only visible in editor and development builds by default
 */
void AChaikinVisualizer::DrawVisualization()
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

	// Get the actor's world position to transform local 2D points to world space
	// ControlPoints are defined in local 2D coordinates relative to this actor
	FVector ActorLocation = GetActorLocation();

	// ==================== STEP 1: Draw Original Control Polygon ====================
	// Draw line segments connecting consecutive control points
	// This shows the input shape before any smoothing is applied
	for (int32 i = 0; i < ControlPoints.Num() - 1; ++i)
	{
		// Convert 2D control point to 3D world position
		// X and Y come from the control point, Z is the configurable height offset
		FVector Start = ActorLocation + FVector(ControlPoints[i].X, ControlPoints[i].Y, ZHeight);
		FVector End = ActorLocation + FVector(ControlPoints[i + 1].X, ControlPoints[i + 1].Y, ZHeight);

		// Draw the line segment
		// Parameters:
		// - World: World context
		// - Start/End: Line endpoints in world space
		// - Color: OriginalCurveColor (default red)
		// - bPersistentLines: false (not persistent between level loads)
		// - LifeTime: -1.0f (draw for exactly one frame)
		// - DepthPriority: 0 (default rendering order)
		// - Thickness: Configurable line thickness
		DrawDebugLine(World, Start, End, OriginalCurveColor, false, -1.0f, 0, LineThickness);
	}

	// Draw spheres at each control point for visual reference
	// These markers make it easy to see where the original vertices are located
	for (const FVector2D& Point : ControlPoints)
	{
		// Convert 2D point to 3D world position
		FVector Position = ActorLocation + FVector(Point.X, Point.Y, ZHeight);

		// Draw a small sphere at the control point location
		// Parameters:
		// - World: World context
		// - Position: Sphere center in world space
		// - Radius: 5.0 units (small, visible marker)
		// - Segments: 8 (sphere tessellation - lower = faster but less smooth)
		// - Color: Same as original curve for consistency
		// - bPersistentLines: false
		// - LifeTime: -1.0f (one frame)
		// - DepthPriority: 0
		// - Thickness: 1.0 (thin outline)
		DrawDebugSphere(World, Position, 5.0f, 8, OriginalCurveColor, false, -1.0f, 0, 1.0f);
	}

	// ==================== STEP 2: Apply Chaikin Smoothing ====================
	// Use the FChaikinSmoothing utility class to process the control points
	TArray<FVector2D> SmoothedPoints;
	FChaikinSmoothing::Smooth(ControlPoints, SmoothedPoints, Iterations);

	// ==================== STEP 3: Draw Smoothed Curve ====================
	// Draw the result of the smoothing algorithm in a different color
	// This allows easy visual comparison with the original polygon
	for (int32 i = 0; i < SmoothedPoints.Num() - 1; ++i)
	{
		// Convert smoothed 2D points to 3D world positions
		FVector Start = ActorLocation + FVector(SmoothedPoints[i].X, SmoothedPoints[i].Y, ZHeight);
		FVector End = ActorLocation + FVector(SmoothedPoints[i + 1].X, SmoothedPoints[i + 1].Y, ZHeight);

		// Draw the smoothed curve segment
		// Uses SmoothedCurveColor (default green) to differentiate from original
		DrawDebugLine(World, Start, End, SmoothedCurveColor, false, -1.0f, 0, LineThickness);
	}

	// Note: We don't draw spheres on the smoothed curve points to avoid visual clutter
	// The smoothed curve typically has many more points than the original, so marking
	// all of them would make the visualization harder to read
}
