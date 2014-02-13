#include "utils.hpp"
#include "ezOptionParser.hpp"

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

const char* g_window_title = "Heatmap";

// Options
int g_kernel_size = 75;
float g_fade_time = 2.0;
float g_kernel_intensity = 0.1;
float g_base_intensity = 0.1;
float g_max_transparency = 0.6;
bool g_linear_kernel = false;
bool g_realtime_playback = true;
bool g_print_progress = false;
bool g_show_video = true;
bool g_out_video = false;
string g_out_file;
string g_out_four_cc = "MJPG";

// Required arguments
string g_in_video;
string g_data_file;

Mat g_heatmap;	
Mat g_kernel;
Mat g_ones;
Mat g_zeros;
Mat g_fade_mat;

vector<DataPoint> g_data;

// Following are in HSV format
Vec3b g_heat_color1 = Vec3b(0, 255, 255); // Red
Vec3b g_heat_color2 = Vec3b(170, 255, 255); // Blue

//============================================================================
// parse_arguments 
//============================================================================

std::string get_usage(ez::ezOptionParser& eop)
{
	string usage;
	eop.getUsage(usage, 80, ez::ezOptionParser::ALIGN);
	return usage;
}

bool parse_arguments(int argc, const char* argv[])
{
	using namespace ez;

	ezOptionParser opt;

	opt.overview = "Create a heatmap overlay on a video";
	opt.syntax = "heatmap [OPTIONS] video point_data";
	opt.example = "heatmap my_video.avi mouse.txt\n";

	ezOptionValidator* vUByte(new ezOptionValidator("u1", "gele", "0, 255"));
	ezOptionValidator* v01Float(new ezOptionValidator("f", "gele", "0.0, 1.0"));
	ezOptionValidator* vNonZeroFloat(new ezOptionValidator("f", "gt", "0.0"));
	ezOptionValidator* vNonZeroInt(new ezOptionValidator("u4", "gt", "0"));
	ezOptionValidator* vString(new ezOptionValidator("text"));

	opt.add("75", 0, 1, 0, "Kernel size in pixels.", "-ks", 
		"--kernel-size", vNonZeroInt);
	opt.add("2.0", 0, 1, 0, "Fade time in seconds. Supports floating values."
		"Must be greater than zero.", "-ft", 
		"--fade-time", vNonZeroFloat);
	opt.add("", 0, 0, 0, "Use a linear kernel instead of gaussian.", 
		"-l", "--linear");
	opt.add("0.1", 0, 1, 0, "Base intensity, higher values saturate faster. [0-1]", 
		"-in", "--intensity", v01Float);
	opt.add("170,255,255", 0, 3, ',', "Starting heat map color in HSV. [0-255]", 
		"-sc", "--start-color", vUByte);
	opt.add("0,255,255", 0, 3, ',', "Ending heat map color in HSV. [0-255]", 
		"-ec", "--end-color", vUByte);
	opt.add("0.6", 0, 1, 0, "Max alpha value for the overlay. [0-1]", 
		"-am", "--alpha-max", v01Float);
	opt.add("", 0, 0, 0, "Play the video back as fast as possible.",
		"-nw", "--no-wait");
	opt.add("", 0, 0, 0, "Print percent progress while playing. "
		"Updates in one percent increments.", "-pp", "--print-progress");
	opt.add("", 0, 0, 0, "Don't show the video window. "
		"Useful for creating an output video.", "-nv", "--no-video");
	opt.add("", 0, 1, 0, "Write heat map video to a file.",
		"-ov", "--out-video", vString);
	opt.add("", 0, 1, 0, "Specify the FOURCC code for the output video. "
		"Must be exactly four characters. Use quotes for whitespace.", 
		"-fc", "--four-cc", vString);

	opt.parse(argc, argv);

	if (opt.lastArgs.size() < 2)
	{
		cerr << "Missing required arguments\n";
		cout << get_usage(opt);
		return false;
	}

	vector<string> bad_options;
	if (!opt.gotRequired(bad_options))
	{
		for (size_t i = 0; i < bad_options.size(); ++i)
		{
			cerr << "Missing option " << bad_options[i] << "\n";
		}

		cout << get_usage(opt);
		return false;
	}

	if (!opt.gotExpected(bad_options))
	{
		for (size_t i = 0; i < bad_options.size(); ++i)
		{
			cerr << "Expected option " << bad_options[i] << "\n";
		}

		cout << get_usage(opt);
		return false;
	}

	vector<string> bad_args;
	if (!opt.gotValid(bad_options, bad_args))
	{
		for (size_t i = 0; i < bad_options.size(); ++i)
		{
			cerr << "Invalid option " << bad_options[i] << " " 
				<< bad_args[i] << "\n";
		}

		return false;
	}

	if (opt.isSet("-ov"))
	{
		opt.get("-ov")->getString(g_out_file);
		g_out_video = true;
	}

	if (opt.isSet("-fc"))
	{
		opt.get("-fc")->getString(g_out_four_cc);
		if (g_out_four_cc.length() != 4)
		{
			cerr << "FOURCC must be exactly four characters.\n";
			return false;
		}

		if (!g_out_video)
		{
			cerr << "Output video file must be specified if specifying a FOURCC.\n";
			return false;
		}
	}

	// Simple flags
	if (opt.isSet("-pp")) { g_print_progress = true; }
	if (opt.isSet("-nv")) { g_show_video = false; }
	if (opt.isSet("-nw")) { g_realtime_playback = false; }
	if (opt.isSet("-ks")) { opt.get("-ks")->getInt(g_kernel_size); }
	if (opt.isSet("-ft")) { opt.get("-ft")->getFloat(g_fade_time); }
	if (opt.isSet("-l"))  { g_linear_kernel = true; }
	if (opt.isSet("-am")) { opt.get("-am")->getFloat(g_max_transparency); }
	if (opt.isSet("-in")) { opt.get("-in")->getFloat(g_base_intensity); }

	if (opt.isSet("-sc"))
	{
		vector<int> multi_int;
		opt.get("-sc")->getInts(multi_int);
		g_heat_color2 = Vec3b(multi_int[0], multi_int[1], multi_int[2]);
	}

	if (opt.isSet("-ec"))
	{
		vector<int> multi_int;
		opt.get("-ec")->getInts(multi_int);
		g_heat_color1 = Vec3b(multi_int[0], multi_int[1], multi_int[2]);
	}

	g_in_video = *opt.lastArgs[0];
	g_data_file = *opt.lastArgs[1];

	return true;
}

//============================================================================
// heat_point
//============================================================================

void heat_point(int x, int y)
{
	// Make sure the coordinates are in bounds
	if (x < 0 || y < 0 || x >= g_heatmap.cols || y >= g_heatmap.rows)
	{
		return;
	}

	// Only update a small portion of the matrix
	const int g_kernel_half = g_kernel_size / 2;
	const int fixed_x = x - g_kernel_half;
	const int fixed_y = y - g_kernel_half;
	const int roi_l = max(fixed_x, 0);
	const int roi_t = max(fixed_y, 0);
	const int roi_w = min(fixed_x + g_kernel_size, g_heatmap.cols) - roi_l;
	const int roi_h = min(fixed_y + g_kernel_size, g_heatmap.rows) - roi_t;

	Mat roi(g_heatmap(Rect(roi_l, roi_t, roi_w, roi_h)));

	const int groi_l = roi_l - fixed_x;
	const int groi_t = roi_t - fixed_y;
	const int groi_w = roi_w;
	const int groi_h = roi_h;

	Mat roi_gauss(g_kernel(Rect(groi_l, groi_t, groi_w, groi_h)));
	roi += roi_gauss;
}

//============================================================================
//
//============================================================================

// For debugging, moving the mouse on the screen will add heat
// map values to that location.
void mouse_event(int event, int x, int y, int, void*)
{
	if (event == CV_EVENT_MOUSEMOVE)
	{
		heat_point(x, y);
	}
}

//============================================================================
// decrease_heatmap
//============================================================================

/* Fades the entire heatmap by g_fade_mat amount.
 */
void decrease_heatmap()
{
	// Fade some of the values in the matrix	
	g_heatmap -= g_fade_mat;
	g_heatmap = max(g_zeros, g_heatmap);
}

//============================================================================
// overlay_heatmap
//============================================================================

/* Draws the heatmap on top of a frame. The frame must be the same size as
 * the heatmap. 
 */
void overlay_heatmap(Mat frame)
{
	// Make sure all values are capped at one
	g_heatmap = min(g_ones, g_heatmap);

	Mat temp_map;
	blur(g_heatmap, temp_map, Size(15, 15));

	for (int r = 0; r < frame.rows; ++r)
	{
		Vec3b* f_ptr = frame.ptr<Vec3b>(r);
		float* h_ptr = temp_map.ptr<float>(r);
		for (int c = 0; c < frame.cols; ++c)
		{
			const float heat_mix = h_ptr[c];
			if (heat_mix > 0.0)
			{
				// in BGR
				const Vec3b i_color = f_ptr[c];

				const Vec3b heat_color = 
					hsv_to_bgr(interpolate_hsv(g_heat_color2, g_heat_color1, heat_mix));

				const float heat_mix2 = std::min(heat_mix, g_max_transparency);

				const Vec3b final_color = interpolate(i_color, heat_color, heat_mix2);
				
				f_ptr[c] = final_color;
			}
		}
	}
}

//============================================================================
// create_kernel
//============================================================================

/* Create the heatmap kernel. This is applied when heat_point() is called. 
 */
void create_kernel()
{
	if (g_linear_kernel)
	{
		// Linear kernel
		const float max_val = 1.0 * g_base_intensity;
		const float min_val = 0.0;
		const float interval = max_val - min_val;

		const int center = g_kernel_size / 2 + 1;
		const float radius = g_kernel_size / 2;

		g_kernel = Mat::zeros(g_kernel_size, g_kernel_size, CV_32F);
		for (int r = 0; r < g_kernel_size; ++r)
		{
			float* ptr = g_kernel.ptr<float>(r);
			for (int c = 0; c < g_kernel_size; ++c)
			{
				// Calculate the distance from the center	
				const float diff_x = static_cast<float>(abs(r - center));
				const float diff_y = static_cast<float>(abs(c - center));
				const float length = sqrt(diff_x*diff_x + diff_y*diff_y);
				if (length <= radius)
				{
					const float b = 1.0 - (length / radius);
					const float val = b*interval + min_val;
					ptr[c] = val;
				}
			}
		}
	}
	else
	{
		// Gaussian kernel
		Mat coeffs = getGaussianKernel(g_kernel_size, 0.0, CV_32F)*150*g_base_intensity;
		g_kernel = coeffs * coeffs.t();
	}
}

//============================================================================
// opencv_error_handler
//============================================================================
int opencv_error_handler(int, const char*, const char*, const char*, int, void*)
{
    return 0;
} 

//============================================================================
// main
//============================================================================

int main(int argc, const char* argv[])
{
	if (!parse_arguments(argc, argv))
	{
		return 1;
	}

	// Open the video file
	VideoCapture video(g_in_video);
	if (!video.isOpened())
	{
		cerr << "Couldn't open video file\n";
		return 2;
	}

	// Read the FPS
	double fps = video.get(CV_CAP_PROP_FPS);
	cout << "Video FPS: " << fps << "\n";

	// Sometimes OpenCV reports an odd FPS value
	if (fps > 60)
	{
		cerr << "Video FPS is too high or can't read correct FPS value\n";
		return 3;
	}

	VideoWriter out_video;
	if (g_out_video)
	{
		const int frame_w = static_cast<int>(video.get(CV_CAP_PROP_FRAME_WIDTH));
		const int frame_h = static_cast<int>(video.get(CV_CAP_PROP_FRAME_HEIGHT));
		Size S = Size(frame_w, frame_h);
		try
		{
			// Make slightly more readable error messages
			cvRedirectError(opencv_error_handler);

			const int four_cc = CV_FOURCC(
				g_out_four_cc[0], g_out_four_cc[1],
				g_out_four_cc[2], g_out_four_cc[3]);
			out_video.open(g_out_file, four_cc,
					video.get(CV_CAP_PROP_FPS), S, true);

			cvRedirectError(NULL);
		}
		catch (cv::Exception& ex)
		{
			cerr << "FOURCC code \"" + g_out_four_cc  + "\" not supported: " 
				<< ex.err << "\n";
			return 4;
		}
	}

	// Read in the data
	if (!parse_data_points(g_data_file, g_data))
	{
		cerr << "Couldn't read data file\n";
		return 5;
	}

	cout << "Number of data points: " << g_data.size() << "\n";

	// Setup the OpenCV window
	if (g_show_video)
	{
		namedWindow(g_window_title, 1);
		setMouseCallback(g_window_title, mouse_event, 0);
	}

	// Get initial video frame, so we can setup other matrices
	Mat frame;
	video >> frame;
	g_heatmap = Mat::zeros(frame.rows, frame.cols, CV_32FC1);
	g_ones = Mat::ones(frame.rows, frame.cols, CV_32F);
	g_zeros = Mat::zeros(frame.rows, frame.cols, CV_32F);

	g_fade_mat = Mat::ones(frame.rows, frame.cols, CV_32F);
	// Determine how much to fade the heatmap values by each frame
	g_fade_mat.setTo((1.0 / fps) / g_fade_time);
	// Create heatmap kernel
	create_kernel();

	// For the OpenCV waitKey loop
	int dt = static_cast<int>((1.0 / fps) * 1000);
	if (!g_realtime_playback && g_show_video)
	{
		// We still have to call waitKey() if they want to see
		// the video. This is because OpenCV does all the GUI
		// updating when waitKey() is called.
		dt = 1;
	}

	// For finding the next data point
	const double frame_time = 1.0 / fps;

	// The current data point
	unsigned int data_index = 0;

	// Global video time, used to sync the data
	double global_time = 0;

	int percentage = -1;
	while (frame.data != NULL && data_index < g_data.size())
	{
		// Plot as many data points for this frame as we can
		while (data_index < g_data.size() &&
				g_data[data_index].timestamp <= global_time)
		{
			heat_point(g_data[data_index].px, g_data[data_index].py);
			++data_index;
		}

		if (g_print_progress)
		{
			// How far through the video, could add a trackbar instead
			int cur_percent = (data_index / (float)g_data.size()) * 100;
			if (cur_percent > percentage)
			{
				cout << cur_percent << "%\n";
				percentage = cur_percent;
			}
		}

		// Draw the heatmap
		overlay_heatmap(frame);
		if (g_show_video)
		{
			imshow(g_window_title, frame);
		}
		decrease_heatmap();

		if (g_out_video)
		{
			out_video << frame;
		}

		video >> frame;
		global_time += frame_time;

		if (g_realtime_playback || g_show_video)
		{
			char key = waitKey(dt);
			if (key == 27 || key == 'q')
			{
				break;
			}
		}
	}

	return 0;
}

//============================================================================
//
//============================================================================
