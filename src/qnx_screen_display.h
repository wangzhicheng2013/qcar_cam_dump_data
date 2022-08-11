#ifndef QNX_SCREEN_DISPLAY
#define QNX_SCREEN_DISPLAY
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <screen/screen.h>
#include <ft2build.h>
#include <wchar.h>
#include FT_FREETYPE_H
#include <string>
#include <vector>
#include <map>
// The width and height of the display must be 1920 and 720, otherwise there will be problems with the display
static const int WIDTH = 1920;
static const int HEIGHT = 720;
typedef std::pair<int, int> coordinate_t;	// (x,y) pair
class qnx_screen_display {
public:
	qnx_screen_display();
	~qnx_screen_display();
	int init();
	void show_window_group_name() {
		printf("window group name = %s\n", window_group_name_);	 	// S202 shows up as __scrn-win-8-00000062-905ab4dde30e7aa311f7fa090fe09a8a
	}
	// pImage:Image data source pointer
	void display_image(const char *pImage);
	// str:text data source pointer
	// start_x:start x coordinate of text
	// start_y:start y coordinate of text
	// width:text render width of screen
	// height:text render height of screen
	// draw text and store text handle
	void display_text(int start_x, int start_y, const char *str);
	// clear handle by coordinate
	void clear_text(const coordinate_t &coordinate);
	// judge whether the text is clear by coordinate
	bool text_is_clear(const coordinate_t &coordinate);
	inline void set_font_path(const char *path) {
		if (path) {
			font_path_ = path;
		}
	}
	inline void set_format(int format) {
		if (format > 0) {
			format_ = format;
		}
	}
	inline void set_image_size(int width, int height) {
		image_size_[0] = width;
		image_size_[1] = height;
		image_cap_ = 2 * width * height;
	}
	inline void set_clip_size(int width, int height) {
		clip_size_[0] = width;
		clip_size_[1] = height;
	}
	inline void set_image_pos(int x, int y) {
		image_pos_[0] = x;
		image_pos_[1] = y;
	}
	inline int prepare_image() {
		int error = screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_BUFFER_SIZE, image_size_);
		if (error) {
			//printf("screen_set_window_property_iv for SCREEN_PROPERTY_BUFFER_SIZE failed, error:%s\n", strerror(errno));
			return error;
		}
		error = screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_POSITION, image_pos_);
		if (error) {
			//printf("screen_set_window_property_iv for SCREEN_PROPERTY_POSITION failed, error:%s\n", strerror(errno));
			return error;
		}
		// Set the window size after scaling
		error = screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_SIZE, clip_size_);
		if (error) {
			//printf("screen_set_window_property_iv for SCREEN_PROPERTY_SIZE failed, error:%s\n", strerror(errno));
			return error;
		}
		return 0;
	}
private:
	void draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y);
	// Call system font library to draw text
	int freetype_draw(const char *str);
	int init_context();
	int init_win();
	int init_freetype();
	int get_display_device();
	screen_window_t create_text();
	inline void clear_text(screen_window_t screen_text_win) {
		if (screen_text_win) {
			screen_destroy_window(screen_text_win);
		}
	}
private:
	static void draw_pix(int x, int y, char *ptr, int stride, int color);
private:
	screen_context_t screen_ctx_;
	screen_window_t  screen_win_;		// display image

	screen_buffer_t image_buf_[2];		// image cache
	screen_display_t *screen_disps_;	// display device
	char *image_buf_ptr_;				// image cache address
	int image_size_[2];					// image size
	int image_pos_[2];					// Image horizontal and vertical coordinates
	int clip_size_[2];					// size after trimming
	const char *font_path_;			    // qnx font library path
	int text_size_;					    // qnx car screen pixel size
	int format_;					    // image format
	int image_cap_;						// The number of bytes occupied by the image

	unsigned char image_[HEIGHT][WIDTH];
	char window_group_name_[64]; 
	int display_count_;

	FT_Library    library_;
	FT_Face       face_;

	std::map<coordinate_t, screen_window_t>coordinate_text_map_;
};

#endif 

