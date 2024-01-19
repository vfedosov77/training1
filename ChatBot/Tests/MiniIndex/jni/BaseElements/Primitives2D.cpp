#include "Primitives2D.h"

const Vector2D Vector2D::ms_notValidVector(g_nIntMin, g_nIntMin);
const Vector2DF Vector2DF::ms_notValidVector(g_nIntMin, g_nIntMin);

const Edge Edge::ms_emptyEdge(Vector2D::ms_notValidVector, Vector2D::ms_notValidVector);

float Edge::distance2(const Edge &_e2) const {
	const float SMALL_NUM = 0.000001;
	Vector2D u = getVector();
	Vector2D v = _e2.getVector();
	Vector2D w = v1 - _e2.v1;
	float a = u.length2(); // always >= 0
	float b = u.dotProduct(v);
	float c = v.length2(); // always >= 0
	float d = u.dotProduct(w);
	float e = v.dotProduct(w);
	float D = a*c - b*b; // always >= 0
	float sc, sN, sD = D; // sc = sN / sD, default sD = D >= 0
	float tc, tN, tD = D; // tc = tN / tD, default tD = D >= 0

	// compute the line parameters of the two closest points
	if (D < SMALL_NUM)
	{
		// the lines are almost parallel
		sN = 0.0; // force using point P0 on segment S1
		sD = 1.0; // to prevent possible division by 0.0 later
		tN = e;
		tD = c;
	}
	else
	{
		// get the closest points on the infinite lines
		sN = (b*e - c*d);
		tN = (a*e - b*d);
		if (sN < 0.0)
		{
			// sc < 0 => the s=0 edge is visible
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if (sN > sD)
		{
			// sc > 1  => the s=1 edge is visible
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if (tN < 0.0)
	{
		// tc < 0 => the t=0 edge is visible
		tN = 0.0;
		// recompute sc for this edge
		if (-d < 0.0)
			sN = 0.0;
		else if (-d > a)
			sN = sD;
		else
		{
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD)
	{
		// tc > 1  => the t=1 edge is visible
		tN = tD;
		// recompute sc for this edge
		if ((-d + b) < 0.0)
			sN = 0;
		else if ((-d + b) > a)
			sN = sD;
		else
		{
			sN = (-d + b);
			sD = a;
		}
	}
	// finally do the division to get sc and tc
	sc = (std::abs(sN) < SMALL_NUM ? 0.0 : sN/sD);
	tc = (std::abs(tN) < SMALL_NUM ? 0.0 : tN/tD);

	// get the difference of the two closest points
	Vector2D dP = w + Vector2D((u.x*sc) - (v.x*tc), (u.y*sc) - (v.y*tc)); // =  S1(sc) - S2(tc)
	return dP.length2(); // return the closest distance
}
