// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSplineGraph.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/SplineMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "SplineLogic/SoMarker.h"
#include "SplineLogic/SoSplineGraphStructs.h"

static constexpr float MIN_HEIGHT = -1000000.0f;
static constexpr float MAX_HEIGHT = 1000000.0f;

DEFINE_LOG_CATEGORY(LogSoSplineGraph);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoSplineGraph::ASoSplineGraph()
{
	PrimaryActorTick.bCanEverTick = false;
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("SoBillboard"));
	RootComponent = Billboard;
}


#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASoSplineGraph, bVisualize)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASoSplineGraph, NodeMeshScale)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASoSplineGraph, NodeTemplate))
		UpdateVisualisation();
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASoSplineGraph, bDebugNoOptimalization))
		Rebuild();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::Rebuild()
{
	// 0. clear
	Nodes.Empty();
	Edges.Empty();
	SplineEdgeList.Empty();

	// 1. build
	for (TActorIterator<ASoMarker> It(GetWorld(), GeneratorClass); It; ++It)
		ExtendGraph(*It);

	// 2. post processes

	// 2.1	remove node if it is between two other (no other connection with any other node)
	//		and it is too close (was created cause of height difference)

	if (!bDebugNoOptimalization)
		for (int32 i = Nodes.Num() - 1; i >= 0; --i)
		{
			// first checks only the registered nodes
			// second also the directional nodes pointing to this target (node does not have them in its list)
			if (Nodes[i].EdgeIndices.Num() == 2 && CalcEdgeTargetCount(i) == 2)
			{
				const int32 FirstIndex = Nodes[i].EdgeIndices[0];
				const int32 SecondIndex = Nodes[i].EdgeIndices[1];

				FSoSplineGraphEdge& FirstEdge = Edges[FirstIndex];
				FSoSplineGraphEdge& SecondEdge = Edges[SecondIndex];

				if (FirstEdge.Type == ESoSplineGraphEdge::ESGE_Walk &&
					SecondEdge.Type == ESoSplineGraphEdge::ESGE_Walk)
				{
					if ((FirstEdge.Length + SecondEdge.Length) < MaxEdgeDistance &&
						FMath::Min(FirstEdge.Length, SecondEdge.Length) < TooCloseEdgeLength)
					{
						// create a new edge to connect the 2 neighbors
						const int32 StartIndex = (FirstEdge.StartNodeIndex == i) ? FirstEdge.EndNodeIndex
																				: FirstEdge.StartNodeIndex;
						const int32 EndIndex = (SecondEdge.StartNodeIndex == i) ? SecondEdge.EndNodeIndex
																				: SecondEdge.StartNodeIndex;
						const float Length = FirstEdge.Length + SecondEdge.Length;
						const float MaxHeight = FMath::Min(FirstEdge.MaxHeight, SecondEdge.MaxHeight);

						Edges.Add({ StartIndex, EndIndex, Length, MaxHeight, ESoSplineGraphEdge::ESGE_Walk });
						Nodes[StartIndex].EdgeIndices.Add(Edges.Num() - 1);
						Nodes[EndIndex].EdgeIndices.Add(Edges.Num() - 1);

						RemoveNode(i);
					}
				}
			}
		}

	// TODO: find portals, add graph nodes and edges so the AI can use them too ???


	// 3. rebuild spline edge list:
	for (int32 i = 0; i < Edges.Num(); ++i)
	{
		// check one end of the edge, assign edge to associated spline
		ASoSpline* FirstSpline = Nodes[Edges[i].StartNodeIndex].SplinePoint.GetSpline();
		FSoIntArray* ArrayPtr = SplineEdgeList.Find(FirstSpline);
		if (ArrayPtr == nullptr)
			ArrayPtr = &SplineEdgeList.Add(FirstSpline, {});
		ArrayPtr->Elements.Add(i);

		// check other end, add if it is not the same
		ASoSpline* SecondSpline = Nodes[Edges[i].EndNodeIndex].SplinePoint.GetSpline();
		if (FirstSpline != SecondSpline)
		{
			ArrayPtr = SplineEdgeList.Find(SecondSpline);
			if (ArrayPtr == nullptr)
				ArrayPtr = &SplineEdgeList.Add(SecondSpline, {});
			ArrayPtr->Elements.Add(i);
		}
	}

	// 4. refresh visualization
	UpdateVisualisation();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::ExtendGraph(ASoMarker* Generator)
{
	const FSoSplinePoint& MarkerSplineLocation = Generator->GetSplineLocation();
	if (Generator == nullptr || !MarkerSplineLocation.IsValid())
		return;

	const float MarkerZ = Generator->GetActorLocation().Z;

	FSoSplineGraphPoint ActualPoint = ConstructPoint(MarkerSplineLocation, MarkerZ);

	if (!IsValid(ActualPoint) || IsPointInGraph(MarkerSplineLocation, ActualPoint.FloorZ))
	{
		UE_LOG(LogSoSplineGraph, Warning, TEXT("Marker %s did not add anything to the graph!"), *Generator->GetName())
		return;
	}

	FSoSplineGraphPoint NextPoint;

	// find first point in negative direction
	while (Step(ActualPoint, -1, NextPoint))
		ActualPoint = NextPoint;

	// add first point
	Nodes.Add({ ActualPoint.SplineLoc, ActualPoint.FloorZ });

	const int32 NodeIndex = Nodes.Num() - 1;
	// explore to both directions
	ExtendGraph(NodeIndex, 1);
	ExtendGraph(NodeIndex, -1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::ExtendGraph(int32 NodeIndex, int32 Direction)
{
	check(NodeIndex >= 0 && NodeIndex < Nodes.Num());

	FSoSplineGraphPoint ActualPoint = ConstructPoint(Nodes[NodeIndex]);
	FSoSplineGraphPoint NextPoint;

	int32 StepCounter = 0;
	bool bAtLeastOneNotInGraph = false;

	float MinHeight = MAX_HEIGHT;
	float MaxHeight = MinEdgeHeight;
	// loop with small steps until each step is valid and each step could be on the same edge
	while (Step(ActualPoint, Direction, NextPoint) && StepCounter * StepSize < MaxEdgeDistance)
	{
		MaxHeight = FMath::Max(NextPoint.Height, MaxHeight);
		const float NewMinHeight = FMath::Min(NextPoint.Height, MinHeight);

		// too much height dif -> new step can't be in the same edge with the previous ones
		if (fabs(NewMinHeight - MaxHeight) > MaxHeightDifference && NewMinHeight < MaxHeightToCheck)
			break;

		ActualPoint = NextPoint;
		MinHeight = NewMinHeight;

		StepCounter++;

		// if we end up in a visited place, we would like to know if there was any unexplored involved or not
		if (!bAtLeastOneNotInGraph)
			bAtLeastOneNotInGraph = !IsPointInGraph(ActualPoint.SplineLoc, ActualPoint.FloorZ + MinEdgeHeight / 2);
	}

	// check, maybe handle CASE 1:
	// some kind of edge, maybe special action can lead to other nodes
	if (StepCounter == 0)
	{
		// special action number 1: walk off a cliff
		TryToAddJumpDownEdge(ActualPoint, NodeIndex, Direction, MinHeight);
		return;
	}

	// check, maybe handle CASE 2:
	int32 ClosestNodeIndex;
	if (IsPointInGraph(ActualPoint.SplineLoc, ActualPoint.FloorZ + MinEdgeHeight / 2.0f, &ClosestNodeIndex))
	{
		// even if it leads back to the original graph, chances are that we have to add an edge here
		if (bAtLeastOneNotInGraph)
		{
			const float EdgeLength = fabs(Nodes[ClosestNodeIndex].SplinePoint - Nodes[NodeIndex].SplinePoint);
			Edges.Add({ NodeIndex, ClosestNodeIndex, EdgeLength, MinHeight, ESoSplineGraphEdge::ESGE_Walk });

			Nodes[NodeIndex].EdgeIndices.Push(Edges.Num() - 1);
			Nodes[ClosestNodeIndex].EdgeIndices.Push(Edges.Num() - 1);
		}

		return;
	}

	// handle CASE 3:
	// nothing special: add another node along direction
	Nodes.Add({ ActualPoint.SplineLoc, ActualPoint.FloorZ });

	const float Length = fabs(ActualPoint.SplineLoc - Nodes[NodeIndex].SplinePoint);
	Edges.Add({ NodeIndex, Nodes.Num() - 1, Length, MinHeight, ESoSplineGraphEdge::ESGE_Walk });

	Nodes[NodeIndex].EdgeIndices.Push(Edges.Num() - 1);
	Nodes[Nodes.Num() - 1].EdgeIndices.Push(Edges.Num() - 1);

	ExtendGraph(Nodes.Num() - 1, Direction);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::TryToAddJumpDownEdge(const FSoSplineGraphPoint& ActualPoint, int32 NodeIndex, int32 Direction, float MinHeight)
{
	// check walk down
	FSoSplinePoint WDSPoint = ActualPoint.SplineLoc;
	bool bSplineOver = false;
	float Rest;
	WDSPoint.AddToDistance(WalkDownDistance * Direction, bSplineOver, Rest);
	if (!bSplineOver && IsVisible(ActualPoint.ToVector(MinEdgeHeight / 2.0f),
									WDSPoint.ToVector(ActualPoint.FloorZ + MinEdgeHeight / 2.0f)))
	{
		const FSoSplineGraphPoint Point = ConstructPoint(WDSPoint, ActualPoint.FloorZ);
		if (IsValid(Point))
		{
			int32 EdgeIndex = -1;
			if (IsPointInGraph(WDSPoint, Point.FloorZ + MinEdgeHeight / 2.0f, nullptr, &EdgeIndex))
			{
				// sigh, already explored place
				// if a node is like really really close it can be bound with the upper one
				// if that's not the case the edge has to be split
				check(EdgeIndex >= 0 && EdgeIndex < Edges.Num());
				const FVector PointLocation = Point.ToVector();
				const FSoSplineGraphNode& FirstNode = Nodes[Edges[EdgeIndex].StartNodeIndex];
				const FSoSplineGraphNode& SecondNode = Nodes[Edges[EdgeIndex].EndNodeIndex];

				if ((FirstNode.ToVector() - PointLocation).SizeSquared() < 20)
				{
					Edges.Add({ NodeIndex,
								Edges[EdgeIndex].StartNodeIndex,
								FMath::Abs(ActualPoint.SplineLoc - FirstNode.SplinePoint),
								MinHeight,
								ESoSplineGraphEdge::ESGE_Fall });

					Nodes[NodeIndex].EdgeIndices.Add(Edges.Num() - 1);
					return;
				}
				if ((SecondNode.ToVector() - PointLocation).SizeSquared() < 20)
				{
					Edges.Add({ NodeIndex,
								Edges[EdgeIndex].EndNodeIndex,
								FMath::Abs(ActualPoint.SplineLoc - SecondNode.SplinePoint),
								MinHeight,
								ESoSplineGraphEdge::ESGE_Fall });

					Nodes[NodeIndex].EdgeIndices.Add(Edges.Num() - 1);
					return;
				}

				// edge has to be split :/
				// step 1: add node
				Nodes.Add({ WDSPoint, Point.FloorZ });
				// step 2: add the fall edge to the new node
				Edges.Add({ NodeIndex,
							Nodes.Num() - 1,
							FMath::Abs(ActualPoint.SplineLoc - Nodes[NodeIndex].SplinePoint),
							MinHeight,
							ESoSplineGraphEdge::ESGE_Fall });
				Nodes[NodeIndex].EdgeIndices.Push(Edges.Num() - 1);
				// step 3: add a new edge between the new node and the EndNode of the edge is about to be split
				Edges.Add({ Nodes.Num() - 1,
							Edges[EdgeIndex].EndNodeIndex,
							FMath::Abs(ActualPoint.SplineLoc - SecondNode.SplinePoint),
							Edges[EdgeIndex].MaxHeight,
							ESoSplineGraphEdge::ESGE_Walk });
				Nodes[Nodes.Num() - 1].EdgeIndices.Push(Edges.Num() - 1);
				Nodes[Edges[EdgeIndex].EndNodeIndex].EdgeIndices.Push(Edges.Num() - 1);
				// step 4: add a new edge between the StartNode of the old edge and the new node
				Edges.Add({ Edges[EdgeIndex].StartNodeIndex,
							Nodes.Num() - 1,
							FMath::Abs(FirstNode.SplinePoint - ActualPoint.SplineLoc),
							Edges[EdgeIndex].MaxHeight,
							ESoSplineGraphEdge::ESGE_Walk });
				Nodes[Nodes.Num() - 1].EdgeIndices.Push(Edges.Num() - 1);
				Nodes[Edges[EdgeIndex].StartNodeIndex].EdgeIndices.Push(Edges.Num() - 1);
				// step 4: remove old edge
				RemoveEdge(EdgeIndex);
			}
			else
			{
				// new, not yet explored land! let's put a node here and check what's up in both directions
				Nodes.Add({ WDSPoint, Point.FloorZ });

				Edges.Add({ NodeIndex,
							Nodes.Num() - 1,
							FMath::Abs(ActualPoint.SplineLoc - Nodes[NodeIndex].SplinePoint),
							MinHeight,
							ESoSplineGraphEdge::ESGE_Fall });
				Nodes[NodeIndex].EdgeIndices.Push(Edges.Num() - 1);

				const int32 NewNodeIndex = Nodes.Num() - 1;
				ExtendGraph(NewNodeIndex, 1);
				ExtendGraph(NewNodeIndex, -1);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::RemoveNode(int32 NodeIndex)
{
	// 1. remove all edges connecting with this node
	for (int32 i = Edges.Num() - 1; i >= 0; --i)
		if (Edges[i].StartNodeIndex == NodeIndex || Edges[i].EndNodeIndex == NodeIndex)
			RemoveEdge(i);


	// update node indices
	for (FSoSplineGraphEdge& Edge : Edges)
	{
		if (Edge.StartNodeIndex > NodeIndex)
			Edge.StartNodeIndex -= 1;

		if (Edge.EndNodeIndex > NodeIndex)
			Edge.EndNodeIndex -= 1;
	}

	Nodes.RemoveAt(NodeIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::RemoveEdge(int32 EdgeIndex)
{
	for (FSoSplineGraphNode& Node : Nodes)
	{
		Node.EdgeIndices.Remove(EdgeIndex);

		// update edge indices
		for (int32 i = 0; i < Node.EdgeIndices.Num(); ++i)
			if (Node.EdgeIndices[i] > EdgeIndex)
				Node.EdgeIndices[i] -= 1;
	}

	Edges.RemoveAt(EdgeIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSplineGraph::IsPointInGraph(const FSoSplinePoint& Point, float ZValue, int32* ClosestNodeIndex, int32* EdgeIndexPtr)
{
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex)
	{
		const FSoSplineGraphNode& Node = Nodes[NodeIndex];
		const ASoSpline* NodeSpline = Node.SplinePoint.GetSpline();

		if (NodeSpline == Point.GetSpline())
		{
			for (const int32& EdgeIndex : Node.EdgeIndices)
				if (IsValid(EdgeIndex))
				{
					const FSoSplineGraphEdge& Edge = Edges[EdgeIndex];

					if (Edge.Type != ESoSplineGraphEdge::ESGE_Walk)
						continue;

					const FSoSplineGraphNode& StartNode = Nodes[Edge.StartNodeIndex];
					const FSoSplineGraphNode& EndNode = Nodes[Edge.EndNodeIndex];
					const FSoSplinePoint& StartNodeSplineLoc = StartNode.SplinePoint;
					const FSoSplinePoint& EndNodeSplineLoc = EndNode.SplinePoint;

					// check along spline
					const float StartToEnd = fabs(EndNodeSplineLoc - StartNodeSplineLoc);
					const float StartToPoint = fabs(Point - StartNodeSplineLoc);
					const float EndToPoint = fabs(Point - EndNodeSplineLoc);

					if (StartToEnd >= StartToPoint && StartToEnd >= EndToPoint)
					{
						// check z
						const float ZOffset = MinEdgeHeight / 2.0f;
						const FVector StartPos = StartNodeSplineLoc.GetWorldLocation(StartNode.ZValue + ZOffset);
						const FVector EndPos = EndNodeSplineLoc.GetWorldLocation(EndNode.ZValue + ZOffset);
						const FVector PointPos = Point.GetWorldLocation(ZValue + ZOffset);

						if (IsVisible(StartPos, PointPos) && IsVisible(PointPos, EndPos))
						{
							if (ClosestNodeIndex != nullptr)
							{
								const float StartDist2 = (PointPos - StartPos).SizeSquared();
								const float EndDist2 = (PointPos - EndPos).SizeSquared();
								*ClosestNodeIndex = StartDist2 < EndDist2 ? Edge.StartNodeIndex : Edge.EndNodeIndex;
							}
							if (EdgeIndexPtr != nullptr)
								*EdgeIndexPtr = EdgeIndex;
							return true;
						}
					}
				}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSplineGraph::IsVisible(const FVector A, const FVector B) const
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	const FCollisionQueryParams QueryParams;
	return (!GetWorld()->LineTraceTestByObjectType(A, B, ObjectQueryParams, QueryParams));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoSplineGraph::GetFloorZUnderPoint(const FSoSplinePoint& Point, float ZValue)
{
	const FVector SplineLocation = Point;

	const FVector Start = FVector(SplineLocation.X, SplineLocation.Y, ZValue);
	const FVector End = FVector(Start.X, Start.Y, MIN_HEIGHT);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams QuaryParams;
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectQueryParams, QuaryParams);

	if (bVisualizeBuild)
		DrawDebugLine(GetWorld(),
					  Start,
					  FVector(End.X, End.Y, bHit ? Hit.ImpactPoint.Z : End.Z),
					  bHit ? FColor(0, 255, 0) : FColor(255, 0, 0),
					  false,
					  VisualizeBuildDuration,
					  0,
					  VisualizeLineTickness);


	return bHit ? Hit.ImpactPoint.Z : MIN_HEIGHT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoSplineGraph::GetCeilingZAbovePoint(const FSoSplinePoint& Point, float ZValue)
{
	const FVector Start = Point.GetWorldLocation(ZValue);
	const FVector End = FVector(Start.X, Start.Y, MAX_HEIGHT);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams QuaryParams;
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectQueryParams, QuaryParams);

	if (bVisualizeBuild)
		DrawDebugLine(GetWorld(),
					  Start,
					  FVector(End.X, End.Y, bHit ? Hit.ImpactPoint.Z : End.Z),
					  bHit ? FColor(0, 0, 255) : FColor(255, 0, 0),
					  false,
					  VisualizeBuildDuration,
					  0,
					  VisualizeLineTickness);

	return bHit ? Hit.ImpactPoint.Z : MAX_HEIGHT;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSplineGraph::IsValid(FSoSplineGraphEdge Edge)
{
	return (Edge.StartNodeIndex >= 0 && Edge.StartNodeIndex < Nodes.Num() &&
			Edge.EndNodeIndex >= 0 && Edge.EndNodeIndex < Nodes.Num());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSplineGraph::IsValid(int32 Edge)
{
	if (Edge < 0 || Edge >= Edges.Num())
		return false;

	return IsValid(Edges[Edge]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 ASoSplineGraph::CalcEdgeTargetCount(int32 NodeIndex)
{
	int32 Count = 0;
	for (const FSoSplineGraphEdge& Edge : Edges)
		if (Edge.EndNodeIndex == NodeIndex || (Edge.IsUndirected() && Edge.StartNodeIndex ==  NodeIndex))
			Count += 1;

	return Count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplineGraphPoint ASoSplineGraph::ConstructPoint(const FSoSplinePoint& SplineLoc, float ZValue)
{
	FSoSplineGraphPoint Point;
	Point.FloorZ = GetFloorZUnderPoint(SplineLoc, ZValue);
	Point.Height = GetCeilingZAbovePoint(SplineLoc, ZValue) - Point.FloorZ;
	Point.SplineLoc = SplineLoc;
	return Point;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplineGraphPoint ASoSplineGraph::ConstructPoint(const FSoSplineGraphNode& Node)
{
	return ConstructPoint(Node.SplinePoint, Node.ZValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSplineGraph::Step(const FSoSplineGraphPoint& StartPoint, int32 StepMultiplier, FSoSplineGraphPoint& EndPoint)
{
	EndPoint.SplineLoc = StartPoint.SplineLoc;

	FSoSplinePoint NewSL = StartPoint.SplineLoc;
	NewSL.SetReferenceZ(StartPoint.FloorZ);
	bool bOutSplineEnd = false;
	float Rest = 0.0f;
	NewSL.AddToDistance(StepSize * StepMultiplier, bOutSplineEnd, Rest);

	const float RefZ = StartPoint.FloorZ + MinEdgeHeight - KINDA_SMALL_NUMBER;
	EndPoint = ConstructPoint(NewSL, RefZ);

	if (!IsValid(EndPoint))
		return false;

	if (bOutSplineEnd && fabs(Rest) > fabs(StepSize * StepMultiplier) - KINDA_SMALL_NUMBER)
		return false;

	// check angle
	const float Angle = FMath::RadiansToDegrees(FMath::Atan(fabs(StartPoint.FloorZ - EndPoint.FloorZ) / StepSize));
	if (fabs(Angle) > MaxStepAngle)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSplineGraph::IsValid(const FSoSplineGraphPoint& Point)
{
	if (Point.FloorZ < MIN_HEIGHT + KINDA_SMALL_NUMBER)
		return false;

	if (Point.Height < MinEdgeHeight)
		return false;

	return Point.SplineLoc.IsValid(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSplineGraph::UpdateVisualisation()
{
	// 0: clean up
	for (UStaticMeshComponent* MeshComp : Meshes)
		if (MeshComp)
			MeshComp->DestroyComponent();
	Meshes.Empty();
	for (UArrowComponent* Arrow : ArrowComponents)
		if (Arrow)
			Arrow->DestroyComponent();
	ArrowComponents.Empty();

	// 1: check if enabled
	if (!bVisualize)
		return;

	// 2: add node meshes:
	for (const FSoSplineGraphNode& Node : Nodes)
	{
		const FVector Location = Node.SplinePoint.ToVector(Node.ZValue);

		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
		MeshComp->RegisterComponent();
		MeshComp->SetStaticMesh(NodeTemplate);
		MeshComp->SetWorldLocation(Location);
		MeshComp->SetWorldScale3D(NodeMeshScale);
		MeshComp->SetWorldRotation(Node.SplinePoint.GetDirection().Rotation());

		Meshes.Add(MeshComp);
	}

	// 3: add edge meshes:
	for (const FSoSplineGraphEdge& Edge : Edges)
	{
		const FVector StartPoint = Nodes[Edge.StartNodeIndex].SplinePoint.ToVector(Nodes[Edge.StartNodeIndex].ZValue);
		const FVector EndPoint = Nodes[Edge.EndNodeIndex].SplinePoint.ToVector(Nodes[Edge.EndNodeIndex].ZValue);
		// side offset based on direction
		const float Length = FMath::Min((EndPoint - StartPoint).Size(), 160.0f);

		switch (Edge.Type)
		{
			case ESoSplineGraphEdge::ESGE_Walk:
			{
				const FVector UpOffset = FVector(0.0f, 0.0f, 15.0f);

				UArrowComponent* Arrow = NewObject<UArrowComponent>(this, UArrowComponent::StaticClass());
				Arrow->RegisterComponent();
				ArrowComponents.Add(Arrow);
				Arrow->SetWorldScale3D(FVector(Length / 80.0f, 1.0f, 1.0f));
				Arrow->SetWorldLocation((StartPoint + EndPoint) / 2.0f + UpOffset);
				Arrow->SetWorldRotation((EndPoint - StartPoint).Rotation());
				Arrow->SetArrowColor(Edge.GetEdgeColor());

				Arrow = NewObject<UArrowComponent>(this, UArrowComponent::StaticClass());
				Arrow->RegisterComponent();
				ArrowComponents.Add(Arrow);
				Arrow->SetWorldScale3D(FVector(Length / 80.0f, 1.0f, 1.0f));
				Arrow->SetWorldLocation((StartPoint + EndPoint) / 2.0f + UpOffset);
				Arrow->SetWorldRotation((StartPoint - EndPoint).Rotation());
				Arrow->SetArrowColor(Edge.GetEdgeColor());
			}
			break;

			case ESoSplineGraphEdge::ESGE_Fall:
			{
				UArrowComponent* Arrow = NewObject<UArrowComponent>(this, UArrowComponent::StaticClass());
				Arrow->RegisterComponent();
				ArrowComponents.Add(Arrow);
				Arrow->SetWorldScale3D(FVector(Length / 80.0f, 1.0f, 1.0f));
				Arrow->SetWorldLocation(StartPoint);
				Arrow->SetWorldRotation((EndPoint - StartPoint).Rotation());
				Arrow->SetArrowColor(Edge.GetEdgeColor());

				Arrow = NewObject<UArrowComponent>(this, UArrowComponent::StaticClass());
				Arrow->RegisterComponent();
				ArrowComponents.Add(Arrow);
				Arrow->SetWorldScale3D(FVector(Length / 80.0f, 1.0f, 1.0f));
				Arrow->SetWorldLocation(EndPoint - (EndPoint - StartPoint).GetSafeNormal() * (Length + 60));
				Arrow->SetWorldRotation((EndPoint - StartPoint).Rotation());
				Arrow->SetArrowColor(Edge.GetEdgeColor());
			}
			break;
		}
	}
}
