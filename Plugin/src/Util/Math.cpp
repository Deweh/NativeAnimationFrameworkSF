#include "Math.h"

namespace Util
{
	float NormalizeSpan(float begin, float end, float val) {
		return (val - begin) / (end - begin);
	}
}