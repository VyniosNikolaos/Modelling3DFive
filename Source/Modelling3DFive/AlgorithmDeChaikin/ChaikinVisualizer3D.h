/*
 * ChaikinVisualizer3D.h
 *
 * An interactive Unreal Engine actor for visualizing Chaikin's corner-cutting algorithm in 3D.
 *
 * This actor provides real-time visualization of the Chaikin smoothing process in full 3D space by:
 * - Displaying the original control polygon in one color (default: red)
 * - Showing the smoothed result in another color (default: green)
 * - Marking control points with spheres for easy identification
 * - Updating in real-time as parameters are modified in the editor
 * - Supporting full 3D control points (X, Y, Z coordinates)
 *
 * Usage:
 * 1. Place this actor in your level
 * 2. Edit the ControlPoints array to define your 3D curve shape
 * 3. Adjust Iterations to control smoothness (0-5)
 * 4. Customize colors and visual settings as desired
 *
 * The visualization uses DrawDebugLine for rendering, which is perfect for:
 * - Quick prototyping and testing of 3D paths
 * - Educational demonstrations of curve smoothing in 3D
 * - Algorithm development and debugging
 * - Camera path visualization
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChaikinVisualizer3D.generated.h"

/**
 * AChaikinVisualizer3D
 *
 * An actor that visualizes Chaikin's corner-cutting curve smoothing algorithm in real-time
 * using full 3D coordinates. Displays both the original control polygon and the smoothed
 * result using debug draw primitives.
 *
 * This visualizer is particularly useful for:
 * - Understanding how Chaikin smoothing works in 3D space
 * - Experimenting with different iteration counts on 3D curves
 * - Teaching subdivision algorithms in 3D
 * - Rapid prototyping of 3D curve-based systems
 * - Designing smooth camera paths
 * - Creating trajectories for moving objects
 */
UCLASS()
class MODELLING3DFIVE_API AChaikinVisualizer3D : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * Constructor
	 * Initializes default values and sets up a basic 3D control polygon for demonstration
	 */
	AChaikinVisualizer3D();

	/**
	 * Array of 3D control points that define the input curve.
	 *
	 * Each point represents a vertex in the 3D control polygon. The curve will be drawn
	 * by connecting these points in sequence in 3D space. Modifying this array in the
	 * editor will immediately update the visualization.
	 *
	 * Tips:
	 * - Minimum 2 points required for meaningful results
	 * - Points are in local 3D space (X, Y, Z coordinates)
	 * - All three dimensions are smoothed equally by the algorithm
	 * - Add duplicate first/last points to create closed curves
	 * - Try varying Z coordinates to see true 3D smoothing
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin 3D")
	TArray<FVector> ControlPoints;

	/**
	 * Number of Chaikin smoothing iterations to apply.
	 *
	 * Each iteration:
	 * - Approximately doubles the number of points
	 * - Produces progressively smoother curves in 3D space
	 * - Increases computation time exponentially
	 *
	 * Recommended values:
	 * - 0: No smoothing (shows original polygon)
	 * - 1: Light smoothing, still angular
	 * - 2: Moderate smoothing (default, good balance)
	 * - 3-5: Very smooth curves
	 * - 6+: Extremely smooth (point of diminishing returns, too many points, use with caution)
	 *
	 * Clamped between 0 and 10 to prevent excessive point generation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin 3D", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Iterations = 2;

	/**
	 * Color used to draw the original control polygon and control point markers.
	 *
	 * Default: Red (easily distinguishable from the smoothed curve)
	 * This helps users clearly see the input shape before smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin 3D")
	FColor OriginalCurveColor = FColor::Red;

	/**
	 * Color used to draw the smoothed curve result.
	 *
	 * Default: Green (contrasts well with the red original curve)
	 * This allows easy visual comparison between input and output.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin 3D")
	FColor SmoothedCurveColor = FColor::Green;

	/**
	 * Thickness of the debug lines used to draw curves.
	 *
	 * Default: 2.0
	 * Larger values make the curves more visible but may look less precise.
	 * Smaller values produce thinner, more precise-looking lines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin 3D")
	float LineThickness = 2.0f;

	/**
	 * Radius of the spheres drawn at control points.
	 *
	 * Default: 10.0
	 * Larger values make control points more visible.
	 * Smaller values produce less visual clutter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin 3D")
	float ControlPointRadius = 10.0f;

protected:
	/**
	 * Called when the game starts or when spawned.
	 * Inherited from AActor.
	 */
	virtual void BeginPlay() override;

public:
	/**
	 * Called every frame.
	 *
	 * Updates the visualization continuously during gameplay, ensuring that
	 * debug lines remain visible (DrawDebugLine with negative lifetime draws for one frame).
	 *
	 * @param DeltaTime Time elapsed since last frame
	 */
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	/**
	 * Called when a property is modified in the editor.
	 *
	 * This enables real-time preview: as you change control points, iterations,
	 * or visual settings in the Details panel, the visualization updates immediately
	 * without needing to play the game.
	 *
	 * @param PropertyChangedEvent Information about which property was changed
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * Called when the actor is moved or modified in the editor.
	 *
	 * This ensures the 3D visualization updates when the actor is moved, rotated, or scaled
	 * in the editor viewport, maintaining accurate visual feedback.
	 */
	virtual void PostEditMove(bool bFinished) override;

	/**
	 * Called every frame in the editor (even when not playing).
	 *
	 * This enables persistent 3D visualization in the editor viewport without pressing Play.
	 * The visualization will continuously update as you work in the editor.
	 *
	 * @param DeltaSeconds Time elapsed since last editor tick
	 */
	virtual void EditorTick(float DeltaSeconds);

	/**
	 * Whether this actor should be ticked in the editor viewport.
	 * Returns true to enable EditorTick() functionality.
	 */
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

private:
	/**
	 * Internal method that performs the actual visualization rendering in 3D.
	 *
	 * This method:
	 * 1. Draws the original 3D control polygon with debug lines
	 * 2. Draws spheres at each 3D control point for visibility
	 * 3. Applies Chaikin smoothing to the 3D control points
	 * 4. Draws the resulting smoothed 3D curve
	 *
	 * Called from both Tick() (during gameplay) and PostEditChangeProperty() (in editor).
	 */
	void DrawVisualization();
};
