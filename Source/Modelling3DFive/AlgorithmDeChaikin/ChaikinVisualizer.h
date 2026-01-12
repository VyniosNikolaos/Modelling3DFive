/*
 * ChaikinVisualizer.h
 *
 * An interactive Unreal Engine actor for visualizing Chaikin's corner-cutting algorithm.
 *
 * This actor provides real-time visualization of the Chaikin smoothing process by:
 * - Displaying the original control polygon in one color (default: red)
 * - Showing the smoothed result in another color (default: green)
 * - Marking control points with spheres for easy identification
 * - Updating in real-time as parameters are modified in the editor
 *
 * Usage:
 * 1. Place this actor in your level
 * 2. Edit the ControlPoints array to define your curve shape
 * 3. Adjust Iterations to control smoothness (0-5)
 * 4. Customize colors and visual settings as desired
 *
 * The visualization uses DrawDebugLine for rendering, which is perfect for:
 * - Quick prototyping and testing
 * - Educational demonstrations
 * - Algorithm development and debugging
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChaikinVisualizer.generated.h"

/**
 * AChaikinVisualizer
 *
 * An actor that visualizes Chaikin's corner-cutting curve smoothing algorithm in real-time.
 * Displays both the original control polygon and the smoothed result using debug draw primitives.
 *
 * This visualizer is particularly useful for:
 * - Understanding how Chaikin smoothing works
 * - Experimenting with different iteration counts
 * - Teaching subdivision algorithms
 * - Rapid prototyping of curve-based systems
 */
UCLASS()
class MODELLING3DFIVE_API AChaikinVisualizer : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * Constructor
	 * Initializes default values and sets up a basic control polygon for demonstration
	 */
	AChaikinVisualizer();

	/**
	 * Array of 2D control points that define the input curve.
	 *
	 * Each point represents a vertex in the control polygon. The curve will be drawn
	 * by connecting these points in sequence. Modifying this array in the editor will
	 * immediately update the visualization.
	 *
	 * Tips:
	 * - Minimum 2 points required for meaningful results
	 * - Points are in local 2D space (X, Y coordinates)
	 * - Use the Z-Height parameter to adjust vertical positioning
	 * - Add duplicate first/last points to create closed curves
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin")
	TArray<FVector2D> ControlPoints;

	/**
	 * Number of Chaikin smoothing iterations to apply.
	 *
	 * Each iteration:
	 * - Approximately doubles the number of points
	 * - Produces progressively smoother curves
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Iterations = 2;

	/**
	 * The Z-coordinate (height) at which to draw the curves.
	 *
	 * Since the algorithm operates on 2D points (XY plane), this parameter allows
	 * you to position the visualization at any vertical height in the 3D world.
	 * Useful for:
	 * - Avoiding Z-fighting with ground planes
	 * - Layering multiple visualizations at different heights
	 * - Aligning with other actors in your scene
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin")
	float ZHeight = 0.0f;

	/**
	 * Color used to draw the original control polygon and control point markers.
	 *
	 * Default: Red (easily distinguishable from the smoothed curve)
	 * This helps users clearly see the input shape before smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin")
	FColor OriginalCurveColor = FColor::Red;

	/**
	 * Color used to draw the smoothed curve result.
	 *
	 * Default: Green (contrasts well with the red original curve)
	 * This allows easy visual comparison between input and output.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin")
	FColor SmoothedCurveColor = FColor::Green;

	/**
	 * Thickness of the debug lines used to draw curves.
	 *
	 * Default: 2.0
	 * Larger values make the curves more visible but may look less precise.
	 * Smaller values produce thinner, more precise-looking lines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaikin")
	float LineThickness = 2.0f;

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
	 * This ensures the visualization updates when the actor is moved, rotated, or scaled
	 * in the editor viewport, maintaining accurate visual feedback.
	 */
	virtual void PostEditMove(bool bFinished) override;

	/**
	 * Called every frame in the editor (even when not playing).
	 *
	 * This enables persistent visualization in the editor viewport without pressing Play.
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
	 * Internal method that performs the actual visualization rendering.
	 *
	 * This method:
	 * 1. Draws the original control polygon with debug lines
	 * 2. Draws spheres at each control point for visibility
	 * 3. Applies Chaikin smoothing to the control points
	 * 4. Draws the resulting smoothed curve
	 *
	 * Called from both Tick() (during gameplay) and PostEditChangeProperty() (in editor).
	 */
	void DrawVisualization();
};
