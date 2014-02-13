#include "utils.hpp"

#include <fstream>
#include <sstream>

using namespace cv;

//============================================================================
// interpolate_hsv
//============================================================================

Vec3b interpolate_hsv(const Vec3b color1, const Vec3b color2, const float value)
{
	if (value <= 0.0) return color1;
	if (value >= 1.0) return color2;

	uchar h = saturate_cast<uchar>(round((1.0 - value)*color1.val[0] + value*color2.val[0])) % 256;
	uchar s = round((1.0 - value)*color1.val[1] + value*color2.val[1]);
	uchar v = round((1.0 - value)*color1.val[2] + value*color2.val[2]);

	return Vec3b(h, s, v);
}

//============================================================================
// interpolate
//============================================================================

Vec3b interpolate(const Vec3b color1, const Vec3b color2, const float value)
{
	uchar b = saturate_cast<uchar>(round((1.0 - value)*color1.val[0] + value*color2.val[0]));
	uchar g = saturate_cast<uchar>(round((1.0 - value)*color1.val[1] + value*color2.val[1]));
	uchar r = saturate_cast<uchar>(round((1.0 - value)*color1.val[2] + value*color2.val[2]));

	return Vec3b(b, g, r);
}

//============================================================================
// hsv_to_bgr
//============================================================================

Vec3b hsv_to_bgr(const Vec3b& hsv)
{
	const float h = ((hsv.val[0] / 255.0) * 360.0) / 60.0;
	const float s = hsv.val[1] / 255.0;
	const float v = hsv.val[2] / 255.0;

	const int i = static_cast<int>(h);
	const float ff = h - i;
	const float p = v * (1.0 - s);
	const float q = v * (1.0 - (s * ff));
	const float t = v * (1.0 - (s * (1.0 - ff)));

	float r = 0, g = 0, b = 0;
	switch (i)
	{
		case 0:
			r = v; g = t; b = p;
			break;
		case 1:
			r = q; g = v; b = p;
			break;
		case 2:
			r = p; g = v; b = t;
			break;
		case 3:
			r = p; g = q; b = v;
			break;
		case 4:
			r = t; g = p; b = v;
			break;
		case 5:
		default:
			r = v; g = p; b = q;
			break;
	}

	const int r_ = saturate_cast<uchar>(r * 255.0f);
	const int g_ = saturate_cast<uchar>(g * 255.0f);
	const int b_ = saturate_cast<uchar>(b * 255.0f);

	return Vec3b(b_, g_, r_);
}

//============================================================================
// parse_data_points
//============================================================================

bool parse_data_points(const std::string& file, std::vector<DataPoint>& out)
{
	using std::ifstream;

	ifstream istrm(file.c_str());
	if (!istrm.is_open())
	{
		return false;
	}

	// Assumes three values per line for the whole file
	float x, y;
	DataPoint dp;
	while (istrm.good() && !istrm.eof())
	{
		istrm >> dp.timestamp;
		istrm >> x;
		istrm >> y;

		dp.px = static_cast<unsigned int>(round(x));
		dp.py = static_cast<unsigned int>(round(y));

		out.push_back(dp);
	}

	istrm.close();

	return !istrm.bad() && istrm.eof();
}

//============================================================================
//
//============================================================================
