
// Representation of a 32-bit float
typedef union {
	float f;
	struct {
		unsigned int mantisa : 23;
		unsigned int exponent : 8;
		unsigned int sign : 1;
	} parts;

} float_cast;

extern "C" {
float  __attribute__((always_inline)) compareFloatValues(float val1, float val2) {

	//int v1 = *reinterpret_cast<int*>(&val1);
	//int v2 = *reinterpret_cast<int*>(&val2);

	/*
	if (!(v1 ^ v2))
		return val1;
	else {
		// Take the absolute and return the min
		// Why even take the absolute? Most activation functions don't allow
		// passing large negative values.
		
		uint32_t temp = v1 >> 31;
		v1 ^= temp;
		v1 += temp & 1;
		temp = v2 >> 31;
		v2 ^= temp;
		v2 += temp & 1;
		*/

	
	float_cast f1 = {.f = val1};
	float_cast f2 = {.f = val2};
	float_cast retval = {.f = 0.0};
	retval.parts.mantisa = f1.parts.mantisa & f2.parts.mantisa;
	retval.parts.exponent = f1.parts.exponent & f2.parts.exponent;
	retval.parts.sign = f1.parts.sign & f2.parts.sign;
	return retval.f;
	
//	return (val1 < val2) ? val1 : val2;
}
}
