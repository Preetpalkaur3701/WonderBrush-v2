#include "GaussFilter.h"

#include <stdio.h>

#include <Bitmap.h>


GaussFilter::GaussFilter()
{
}


GaussFilter::~GaussFilter()
{
}


void
GaussFilter::Filter(BBitmap* bitmap, double sigma)
{
	sigma /= 2.0;

	if (sigma < 0.5) {
		printf("GaussFilter::Filter() - radius too small (< 1.0)\n");
		return;
	}

	_Init(sigma);

	int32 width = bitmap->Bounds().IntegerWidth() + 1;
	int32 height = bitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = bitmap->BytesPerRow();
	uint8* bits = (uint8*)bitmap->Bits();

	if (bitmap->ColorSpace() == B_RGBA32
		|| bitmap->ColorSpace() == B_RGB32) {

		for (int32 i = 0; i < height; i++) {
			_FilterRow4(bits + i * bpr, width,
						bitmap->ColorSpace() == B_RGBA32);
		}
		for (int32 i = 0; i < width; i++) {
			_FilterColumn4(bits + i * 4, bpr, height,
						   bitmap->ColorSpace() == B_RGBA32);
		}

	} else if (bitmap->ColorSpace() == B_GRAY8) {

		for (int32 i = 0; i < height; i++) {
			_FilterRow1(bits + i * bpr, width);
		}
		for (int32 i = 0; i < width; i++) {
			_FilterColumn1(bits + i, bpr, height);
		}

	} else {
		printf("GaussFilter::Filter() - unsupported color space\n");
	}
}


// #pragma mark -


void
GaussFilter::_Init(double sigma)
{
	// calculate the weird numbers as instructed in that paper
	double q;
	if (sigma >= 2.5) {
		q = 0.98711 * sigma - 0.96330;
	} else {
		q = 3.97156 - 4.14554 * sqrt(1 - 0.26891 * sigma);
	}

	double q2 = q * q;
	double q3 = q2 * q;

	b0 = (calc_type)(1.57825 + 2.44413 * q + 1.4281 * q2 + 0.422205 * q3);
	b1 = (calc_type)(2.44413 * q + 2.85619 * q2 + 1.26661 * q3);
	b2 = (calc_type)(-(1.4281 * q2 + 1.26661 * q3));
	b3 = (calc_type)(0.422205 * q3);

	B = (calc_type)(1 - (b1 + b2 + b3) / b0);

	// NOTE: eliminate some extra multiplications
	b1 /= b0;
	b2 /= b0;
	b3 /= b0;
}


void
GaussFilter::_Filter(calc_type* buffer, int32 count)
{
	// forward
	calc_type* b = buffer + 3;
	for (int32 i = 3; i < count + 6; i++) {
		*b++ = B * b[0] + (b1 * b[-1] + b2 * b[-2] + b3 * b[-3]);
	}

	// backward
	b = buffer + count + 2;
	for (int32 i = count + 2; i >= 0; i--) {
		*b-- = B * b[0] + (b1 * b[1] + b2 * b[2] + b3 * b[3]);
	}
}

void
GaussFilter::_FilterRow4(uint8* buffer, int32 count, bool alpha)
{
	calc_type temp0[count + 6];
	calc_type temp1[count + 6];
	calc_type temp2[count + 6];
	calc_type temp3[count + 6];

	// copy buffer into temps

	uint8* b = buffer;
	int32 it = 3;
	if (alpha) {
		for (int32 i = 0; i < count; i++, it++) {
			temp0[it] = b[0];
			temp1[it] = b[1];
			temp2[it] = b[2];
			temp3[it] = b[3];
			b += 4;
		}
	} else {
		for (int32 i = 0; i < count; i++, it++) {
			temp0[it] = b[0];
			temp1[it] = b[1];
			temp2[it] = b[2];
			b += 4;
		}
	}
	// copy edge pixels
	temp0[0] = temp0[1] = temp0[2] = temp0[3];
	temp0[count + 3] = temp0[count + 4] = temp0[count + 5] = temp0[count + 2];

	temp1[0] = temp1[1] = temp1[2] = temp1[3];
	temp1[count + 3] = temp1[count + 4] = temp1[count + 5] = temp1[count + 2];

	temp2[0] = temp2[1] = temp2[2] = temp2[3];
	temp2[count + 3] = temp2[count + 4] = temp2[count + 5] = temp2[count + 2];

	// filter
	_Filter(temp0, count);
	_Filter(temp1, count);
	_Filter(temp2, count);

	if (alpha) {
		temp3[0] = temp3[1] = temp3[2] = temp3[3];
		temp3[count + 3] = temp3[count + 4] = temp3[count + 5] = temp3[count + 2];
		_Filter(temp3, count);
	}

	// copy temp back to buffer

	b = buffer;
	it = 3;
	if (alpha) {
		for (int32 i = 0; i < count; i++, it++) {
			b[0] = (uint8)temp0[it];
			b[1] = (uint8)temp1[it];
			b[2] = (uint8)temp2[it];
			b[3] = (uint8)temp3[it];
			b += 4;
		}
	} else {
		for (int32 i = 0; i < count; i++, it++) {
			b[0] = (uint8)temp0[it];
			b[1] = (uint8)temp1[it];
			b[2] = (uint8)temp2[it];
			b += 4;
		}
	}
}

void
GaussFilter::_FilterColumn4(uint8* buffer, uint32 skip,
							int32 count, bool alpha)
{
	calc_type temp0[count + 6];
	calc_type temp1[count + 6];
	calc_type temp2[count + 6];
	calc_type temp3[count + 6];

	// copy buffer into temps

	uint8* b = buffer;
	int32 it = 3;
	if (alpha) {
		for (int32 i = 0; i < count; i++, it++) {
			temp0[it] = b[0];
			temp1[it] = b[1];
			temp2[it] = b[2];
			temp3[it] = b[3];
			b += skip;
		}
	} else {
		for (int32 i = 0; i < count; i++, it++) {
			temp0[it] = b[0];
			temp1[it] = b[1];
			temp2[it] = b[2];
			b += skip;
		}
	}

	// copy edge pixels
	temp0[0] = temp0[1] = temp0[2] = temp0[3];
	temp0[count + 3] = temp0[count + 4] = temp0[count + 5] = temp0[count + 2];

	temp1[0] = temp1[1] = temp1[2] = temp1[3];
	temp1[count + 3] = temp1[count + 4] = temp1[count + 5] = temp1[count + 2];

	temp2[0] = temp2[1] = temp2[2] = temp2[3];
	temp2[count + 3] = temp2[count + 4] = temp2[count + 5] = temp2[count + 2];

	// filter
	_Filter(temp0, count);
	_Filter(temp1, count);
	_Filter(temp2, count);

	if (alpha) {
		temp3[0] = temp3[1] = temp3[2] = temp3[3];
		temp3[count + 3] = temp3[count + 4] = temp3[count + 5] = temp3[count + 2];
		_Filter(temp3, count);
	}

	// copy temp back to buffer

	b = buffer;
	it = 3;
	if (alpha) {
		for (int32 i = 0; i < count; i++, it++) {
			b[0] = (uint8)temp0[it];
			b[1] = (uint8)temp1[it];
			b[2] = (uint8)temp2[it];
			b[3] = (uint8)temp3[it];
			b += skip;
		}
	} else {
		for (int32 i = 0; i < count; i++, it++) {
			b[0] = (uint8)temp0[it];
			b[1] = (uint8)temp1[it];
			b[2] = (uint8)temp2[it];
			b += skip;
		}
	}
}


void
GaussFilter::_FilterRow1(uint8* buffer, int32 count)
{
	calc_type temp[count + 6];

	for (int32 i = 0; i < count; i++)
		temp[i + 3] = buffer[i];

	temp[0] = temp[1] = temp[2] = temp[3];
	temp[count + 3] = temp[count + 4] = temp[count + 5] = temp[count + 2];

	_Filter(temp, count);

	for (int32 i = 0; i < count; i++)
		buffer[i] = (uint8)temp[i + 3];
}

void
GaussFilter::_FilterColumn1(uint8* buffer, uint32 skip, int32 count)
{
	calc_type temp[count + 6];

	for (int32 i = 0; i < count; i++)
		temp[i + 3] = buffer[i * skip];

	temp[0] = temp[1] = temp[2] = temp[3];
	temp[count + 3] = temp[count + 4] = temp[count + 5] = temp[count + 2];

	_Filter(temp, count);

	for (int32 i = 0; i < count; i++)
		buffer[i * skip] = (uint8)temp[i + 3];
}


