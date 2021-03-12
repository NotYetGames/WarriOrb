// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoSplineGraphStructs.h"
#include "SoSplinePoint.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplineGraphNode::FSoSplineGraphNode(const FSoSplinePoint& InSplinePoint, float InZValue) :
	SplinePoint(InSplinePoint),
	ZValue(InZValue)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLinearColor FSoSplineGraphEdge::GetEdgeColor() const
{
	switch (Type)
	{
		case ESoSplineGraphEdge::ESGE_Walk:
			return FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

		case ESoSplineGraphEdge::ESGE_Fall:
			return FLinearColor(0.0f, 0.6f, 0.7f, 1.0f);
	}

	return FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
}
