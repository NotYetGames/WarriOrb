// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Components/BillboardComponent.h"
#include "Logging/LogMacros.h"
#include "SoSplineGraphStructs.h"
#include "GameFramework/Actor.h"
#include "SoSpline.h"
#include "SoSplineGraph.generated.h"

struct FSoSplinePoint;
class ASoMarker;
class UArrowComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSoSplineGraph, Log, All);


UCLASS()
class SORB_API ASoSplineGraph : public AActor
{
	GENERATED_BODY()

public:

	/** please tell me that I don't have to write here what a constructor is */
	ASoSplineGraph();

#if WITH_EDITOR
	/** used to rebuild visuals if any associated variable changes */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** clears the graph, gets all generator markers and rebuilds it starting from those */
	void Rebuild();

protected:
	// Runtime functionality:





	// Graph construction functionality:

	/* Explores all not yet explored reachable area starting from the input marker, and adds it to the graph*/
	void ExtendGraph(ASoMarker* Generator);

	/* Extends the graph if any not yet visited reachable area found from Nodes[NodeIndex] based on input direction */
	void ExtendGraph(int32 NodeIndex, int32 Direction);

	/* If the node is at an edge (edge like the edge of a table) maybe a JumpDown edge can be added here */
	void TryToAddJumpDownEdge(const FSoSplineGraphPoint& ActualPoint, int32 NodeIndex, int32 Direction, float MinHeight);

	/** removes node and all associated edges, fixes both node and end indices accordingly */
	void RemoveNode(int32 NodeIndex);

	/** removes edge, updates each edge index accordingly */
	void RemoveEdge(int32 EdgeIndex);

	/** performs a line check against static objects between A and B, returns true if no hit occurred */
	bool IsVisible(const FVector A, const FVector B) const;

	/** first hit impact point downwards from world location defined via {Point, ZValue} */
	float GetFloorZUnderPoint(const FSoSplinePoint& Point, float ZValue);
	/** first hit impact point upwards from world location defined via {Point, ZValue} */
	float GetCeilingZAbovePoint(const FSoSplinePoint& Point, float ZValue);

	/**
	 *  Checks if given input location is on the graph, optionally gives back the closest edge/node
	 *  IMPORTANT: construct functionality, should not be used runtime
	 *  For runtime use: <TODO: function name>
	 *
	 * @PARAM  Point: spline point of the location to check
	 * @PARAM  ZValue: z coordinate of the location to check
	 * @PARAM  ClosestNodeIndex: optional, calculated if it is on spline
	 * @PARAM  EdgeIndexPtr: optional, filled if it is on spline
	 * @RETURN weather the point was in the graph or not
	 */
	bool IsPointInGraph(const FSoSplinePoint& Point, float ZValue, int32* ClosestNodeIndex = nullptr, int32* EdgeIndexPtr = nullptr);

	/** checks if the edge has 2 valid node index */
	bool IsValid(FSoSplineGraphEdge Edge);

	/** checks if the edge index is a valid edge index pointing to an edge with 2 valid node index */
	bool IsValid(int32 Edge);

	/** calculates the amount of edges having the node as targets */
	int32 CalcEdgeTargetCount(int32 NodeIndex);


	FSoSplineGraphPoint ConstructPoint(const FSoSplinePoint& SplineLoc, float ZValue);
	FSoSplineGraphPoint ConstructPoint(const FSoSplineGraphNode& Node);

	/* one step from StartPoint in the direction defined by StepMultiplier, if successful EndPoint is filled */

	/**
	 *  Makes one step along the spline, step size is based on StepSize
	 *  A successful step has to meet these requirements:
	 *		- EndPoint is a valid point (has floor, height is large enough)
	 *		- it actually has at least some horizontal movement (not sucked at the dead-end of a spline)
	 *		- Height offset between StartPoint and EndPoint is smaller as a threshold (defined via MaxStepAngle)
	 *
	 * @PARAM  StartPoint: source location of the step
	 * @PARAM  StepMultiplier: should be 1 or -1, defines the direction the step is heading
	 * @PARAM  EndPoint: Output, filled if the step is successful
	 * @RETURN weather the step was a success or not
	 */
	bool Step(const FSoSplineGraphPoint& StartPoint, int32 StepMultiplier, FSoSplineGraphPoint& EndPoint);

	/* a valid point has floor under it, and the distance between floor and ceiling is greater than MinEdgeHeight */
	bool IsValid(const FSoSplineGraphPoint& Point);

	/* rebuilds the visual meshes and stuffs based on the setup defined by the member variabled in the Visualization category */
	void UpdateVisualisation();

protected:

	UPROPERTY(EditAnywhere)
	bool bDebugNoOptimalization;

	/** Horizontal step size along spline for the ray-casts */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float StepSize = 5;

	/** Max angle in degrees an edge can contain at any given point */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float MaxStepAngle = 45;

	/** Max height difference inside an edge */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float MaxHeightDifference = 30;

	/** spline location with height under MidEdgeHeight is not considered as walkable surface */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float MinEdgeHeight = 30;

	/** height change above MaxHeightToCheck will not lead to new edges */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float MaxHeightToCheck = 500;

	/** horizontal length of a walk down edge */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float WalkDownDistance = 60;

	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float MaxEdgeDistance = 1000.0f;


	/** edges too close to each other will be cleaned unless they are there for a good reason (e.g. jump/fall edge), branch */
	UPROPERTY(EditAnywhere, Category = GraphConstants)
	float TooCloseEdgeLength = 300.0f;


	UPROPERTY(VisibleAnywhere, Category = GraphData)
	TArray<FSoSplineGraphNode> Nodes;

	UPROPERTY(VisibleAnywhere, Category = GraphData)
	TArray<FSoSplineGraphEdge> Edges;

	/*
	 *  list of the index of each connected edge for each spline, RemoveEdge() and RemoveNode() does not update this
	 *  it is generated as the last step of the constuct process, should only be used runtime
	 */
	UPROPERTY(VisibleAnywhere, Category = GraphData)
	TMap<ASoSpline*, FSoIntArray> SplineEdgeList;

	/** the child class of markers used as start locations */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ASoMarker> GeneratorClass;


	/* It makes the actor clickable, and also serves as a rootcomponent (without it one generated node visualizer mesh would become root) */
	UPROPERTY(EditAnywhere, Category = Visualization)
	UBillboardComponent* Billboard;

	UPROPERTY(EditAnywhere, Category = Visualization)
	bool bVisualize;

	/* each raycast is displayed as a debug line with different colors, disabled by default for good reason */
	UPROPERTY(EditAnywhere, Category = Visualization)
	bool bVisualizeBuild;

	UPROPERTY(EditAnywhere, Category = Visualization)
	float VisualizeBuildDuration;

	UPROPERTY(EditAnywhere, Category = Visualization)
	float VisualizeLineTickness = 3.0f;

	/* mesh used to visualize nodes */
	UPROPERTY(EditAnywhere, Category = Visualization)
	UStaticMesh* NodeTemplate;

	UPROPERTY(EditAnywhere, Category = Visualization)
	FVector NodeMeshScale = FVector(1.0f, 1.0f, 1.0f);

	/* the mesh components used to visualize nodes */
	UPROPERTY(VisibleAnywhere, Category = Visualization)
	TArray<UStaticMeshComponent*> Meshes;

	/* the array components used to visualize edges */
	UPROPERTY(VisibleAnywhere, Category = Visualization)
	TArray<UArrowComponent*> ArrowComponents;
};



// TODO: runtime function to get graph location
// TODO: AI should use the graph
// TODO: jump nodes ?!
