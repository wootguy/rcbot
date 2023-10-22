#pragma once
#include <vector.h>

inline bool operator<=(const Vector &v1, const Vector& v2) {
	return ( (v1.x <= v2.x) && (v1.y <= v2.y) && (v1.z <= v2.z) ); 
}
inline bool operator>=(const Vector& v1, const Vector& v2) {
	return ( (v1.x >= v2.x) && (v1.y >= v2.y) && (v1.z >= v2.z) );
}