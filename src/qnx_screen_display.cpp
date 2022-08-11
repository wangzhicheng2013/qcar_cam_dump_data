#include "qnx_screen_display.h"
qnx_screen_display::qnx_screen_display() {
	memset(&screen_ctx_, 0, sizeof(screen_ctx_));
	memset(&image_, 0, sizeof(image_));
	memset(&window_group_name_, 0, sizeof(window_group_name_));
	memset(&image_buf_, 0, sizeof(image_buf_));
	memset(&image_pos_, 0, sizeof(image_pos_));
	image_size_[0] = 1920;
	image_size_[1] = 1080;
	image_cap_ = 2 * image_size_[0] * image_size_[1];
	image_buf_ptr_ = NULL;
	// No scaling by default
	clip_size_[0] = 1920;
	clip_size_[1] = 1080;

	font_path_ = "/usr/bin/avmsetting/FZLTH.ttf";
	text_size_ = 42;
	format_ = SCREEN_FORMAT_UYVY;
	screen_disps_ = NULL;
	display_count_ = 0;
}
void qnx_screen_display::draw_pix(int x, int y, char *ptr, int stride, int color) {
	ptr += (stride * y);
	ptr[x * 4] = ((0xff000000 & color) >> 24);
	ptr[x * 4 + 1] = ((0x00ff0000 & color) >> 16);
	ptr[x * 4 + 2] = ((0x0000ff00 & color) >> 8);
	ptr[x * 4 + 3] = (0x000000ff & color);
}

void qnx_screen_display::draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y) {
  	FT_Int  i, j, p, q;
  	FT_Int  x_max = x + bitmap->width;
  	FT_Int  y_max = y + bitmap->rows;
  	for (i = x, p = 0;i < x_max;i++, p++) {
		for (j = y, q = 0; j < y_max; j++, q++) {
			if (i < 0 || j < 0 || i >= WIDTH || j >= HEIGHT) {
				continue;
			}
			image_[j][i] |= bitmap->buffer[q * bitmap->width + p];
		}
	}
}
int qnx_screen_display::freetype_draw(const char * str) { 
		
  	FT_GlyphSlot  slot;
  	FT_Matrix     matrix;                
  	FT_Vector     pen;                  
  	FT_Error      error;
 
  	double        angle;
  	int           target_height;
  	int           i, num_chars;
                                               
  	num_chars     = strlen(str);
  	angle         = (0.0 / 360) * 3.14159 * 2;     
  	target_height = HEIGHT;
  	FT_Set_Pixel_Sizes(face_, text_size_, text_size_);
  	slot = face_->glyph;
  	matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
  	matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
  	matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
  	matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

   	pen.x = 0 * 64;
  	pen.y = (target_height - (text_size_ * 7) / 10) * 64;
 	for (i = 0;i < num_chars;i++) {	  
		FT_Set_Transform(face_, &matrix, &pen);
		error = FT_Load_Char(face_, (wchar_t)str[i], FT_LOAD_RENDER);
		if (error) {
			continue;             
		}
		draw_bitmap(&slot->bitmap,
				slot->bitmap_left,
				target_height - slot->bitmap_top);

		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
  	}
  	return error;
}
int qnx_screen_display::init_context() {
	int error = screen_create_context(&screen_ctx_, SCREEN_APPLICATION_CONTEXT);
	if (error) {
		printf("screen_create_context failed, error:%s\n", strerror(errno));		// The error code is stored in errno
		return error;
	}
	return 0;
}
int qnx_screen_display::get_display_device() {
	int error = screen_get_context_property_iv(screen_ctx_, SCREEN_PROPERTY_DISPLAY_COUNT, &display_count_);	// Using the number of displays returned by the query, allocate enough memory to retrieve an array of pointers to screen_display_t
	printf("display_count = %d\n", display_count_);
	if (error) {			
		printf("screen_get_context_property_iv for SCREEN_PROPERTY_DISPLAY_COUNT failed, error:%s\n", strerror(errno));
		return error;
	}
	if (display_count_ < 2) {		// The count of the S202 device is 2
		printf("SCREEN_PROPERTY_DISPLAY_COUNT is less than 2\n");
		return -1;
	}
	screen_disps_ = (screen_display_t *)calloc(display_count_, sizeof(screen_display_t));
	if (!screen_disps_) {
		printf("calloc for screen_disps failed\n");
		return -2;
	}
	error = screen_get_context_property_pv(screen_ctx_, SCREEN_PROPERTY_DISPLAYS, (void **)screen_disps_);
	if (error) {
		free(screen_disps_);
		screen_disps_ = NULL;
		printf("screen_get_context_property_pv for SCREEN_PROPERTY_DISPLAYS failed, error:%s\n", strerror(errno));
		return error;
	}
	return 0;
}
int qnx_screen_display::init_win() {
	// Create your render targets.
	int error = screen_create_window(&screen_win_, screen_ctx_);
	if (error) {
		printf("screen_create_window failed, error:%s\n", strerror(errno));
		return error;
	}
	error = screen_get_window_property_cv(screen_win_, SCREEN_PROPERTY_ID, sizeof(window_group_name_), window_group_name_);
	if (error) {
		printf("screen_get_window_property_cv for SCREEN_PROPERTY_ID failed, error:%s\n", strerror(errno));
		return error;
	}
	show_window_group_name();
	int vis = 1;
	int dims[2] = {0, 0};	// image size
	error = get_display_device();
	if (error) {
		return error;
	}
	screen_display_t screen_disp = screen_disps_[1];		// Must be 1, otherwise it cannot be drawn
	error = screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_SIZE, dims);
	if (error) {
		printf("screen_get_context_property_pv for SCREEN_PROPERTY_SIZE failed, error:%s\n", strerror(errno));
		return error;
	}
	printf("start screen size w = %d, h = %d\n", dims[0], dims[1]);
	error = screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_VISIBLE, &vis);
	if (error) {
		printf("screen_set_window_property_iv for SCREEN_PROPERTY_VISIBLE failed, error:%s\n", strerror(errno));
		return error;
	}
	error = screen_set_window_property_pv(screen_win_, SCREEN_PROPERTY_DISPLAY, (void**)&screen_disps_[1]);
	if (error) {
		printf("screen_set_window_property_iv for SCREEN_PROPERTY_DISPLAY failed, error:%s\n", strerror(errno));
		return error;
	}
	int usage = SCREEN_USAGE_WRITE;
	error =  screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_USAGE, &usage);
	if (error) {
		printf("screen_set_window_property_iv for SCREEN_PROPERTY_USAGE failed, error:%s\n", strerror(errno));
		return error;
	}
	int zorder = 15;
	error = screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_ZORDER, &zorder);
	if (error) {
		printf("screen_set_window_property_iv for SCREEN_PROPERTY_ZORDER failed, error:%s\n", strerror(errno));
		return error;
	}
	// Create screen window buffers
	error = screen_create_window_buffers(screen_win_, 1);
	if (error) {
		printf("screen_create_window_buffers failed, error:%s\n", strerror(errno));
		return error;
	}
	error = screen_set_window_property_iv(screen_win_, format_, &format_);
	if (error) {
		printf("screen_set_window_property_iv for format failed, error:%s\n", strerror(errno));
		return error;
	}
	// Set the original window size
	error = screen_set_window_property_iv(screen_win_, SCREEN_PROPERTY_SOURCE_SIZE, dims);
	if (error) {
		printf("screen_set_window_property_iv for SCREEN_PROPERTY_SOURCE_SIZE failed, error:%s\n", strerror(errno));
		return error;
	}
	error = screen_get_window_property_pv(screen_win_, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)image_buf_);
	if (error) {
		printf("screen_set_window_property_iv for SCREEN_PROPERTY_RENDER_BUFFERS failed, error:%s\n", strerror(errno));
		return error;
	}
	error = screen_get_buffer_property_pv(image_buf_[0], SCREEN_PROPERTY_POINTER, (void **)&image_buf_ptr_);		// Associative buffer and data pointer
	if (error) {
		printf("screen_get_buffer_property_pv for SCREEN_PROPERTY_POINTER failed, error:%s\n", strerror(errno));
		return error;
	}
	return 0;
}
int qnx_screen_display::init_freetype() {
	int error = FT_Init_FreeType(&library_); 
	if (error) {
		return error;
	}
  	error = FT_New_Face(library_, font_path_, 0, &face_); 
	if (error) {
		FT_Done_Face(face_);
		FT_Done_FreeType(library_);
		return error;
	}
	return 0;
}
int qnx_screen_display::init() {
	int error = init_context();
	if (error) {
		return error;
	}
	error = init_win();
	if (error) {
		return error;
	}
	if (init_freetype()) {
		return error;
	}
	return 0;
}
void qnx_screen_display::display_image(const char *pImage) {
	if (!pImage) {
		return;
	}
	memcpy(image_buf_ptr_, pImage, image_cap_);
	screen_post_window(screen_win_, image_buf_[0], 0, NULL, 0);
}
screen_window_t qnx_screen_display::create_text() {
	int vis = 1;
	screen_window_t screen_text_win = NULL;
	screen_create_window(&screen_text_win, screen_ctx_);
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_VISIBLE, &vis);
	screen_set_window_property_pv(screen_text_win, SCREEN_PROPERTY_DISPLAY, (void**)&screen_disps_[1]);
	int flag = 1;
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_STATIC, &flag);
	int format = SCREEN_FORMAT_RGBA8888;
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_FORMAT, &format);
	int usage = SCREEN_USAGE_WRITE;
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_USAGE, &usage);
	int transparency = SCREEN_TRANSPARENCY_SOURCE_OVER;
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_TRANSPARENCY, &transparency);
	return screen_text_win;
}
void qnx_screen_display::display_text(int start_x, int start_y, const char *str) {
	if (!str) {
		return;
	}
	int size[2] = { 0 };
	std::pair<int, int>coordinate(start_x, start_y);
	std::map<coordinate_t, screen_window_t>::iterator it = coordinate_text_map_.find(coordinate);
	screen_window_t screen_text_win = NULL;
	if (coordinate_text_map_.end() == it) {		// draw new text
		screen_text_win = create_text();
		if (!screen_text_win) {
			return;
		}
		coordinate_text_map_[coordinate] = screen_text_win;
	}
	else {
		if (NULL == it->second) {
			it->second = create_text();
		}
		screen_text_win = it->second;
		if (!screen_text_win) {
			return;
		}
	}
	int pos[2] = { start_x, start_y };
	int rect[4]= { start_x, start_y, WIDTH, HEIGHT };
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_POSITION, pos); 
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_BUFFER_SIZE, rect + 2);

	screen_buffer_t screen_buf;
	screen_create_window_buffers(screen_text_win, 1);
	screen_get_window_property_pv(screen_text_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf);

	int zorder = 15;
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_ZORDER, &zorder);

	char *ptr = NULL;
	screen_get_buffer_property_pv(screen_buf, SCREEN_PROPERTY_POINTER, (void **)&ptr);
	int i = 0, j = 0;

	int stride = 0;
	screen_get_buffer_property_iv(screen_buf, SCREEN_PROPERTY_STRIDE, &stride);

	memset(image_, 0, sizeof(image_));
	freetype_draw(str);
	
	for (i = 0;i < HEIGHT; i++) {
		for (j = 0;j < WIDTH;j++) {
			if (image_[i][j]) {
				draw_pix(j, i,ptr, stride, 0xa0a0a0ff);
			}
			else {
				draw_pix(j, i,ptr, stride, 0xffffff00);
			}
			screen_get_buffer_property_pv(screen_buf, SCREEN_PROPERTY_POINTER, (void **)&ptr);
		}
	}
	screen_post_window(screen_text_win, screen_buf, 1, rect, 0);
	screen_get_window_property_iv(screen_text_win, SCREEN_PROPERTY_BUFFER_SIZE, size);
	screen_set_window_property_iv(screen_text_win, SCREEN_PROPERTY_SIZE, size);
	screen_flush_context(screen_ctx_, SCREEN_WAIT_IDLE);
}
void qnx_screen_display::clear_text(const coordinate_t &coordinate) {
	std::map<coordinate_t, screen_window_t>::iterator it = coordinate_text_map_.find(coordinate);
	if (coordinate_text_map_.end() != it) {
		clear_text(it->second);		// clear the text in the screen
		it->second = NULL;
	}
}
bool qnx_screen_display::text_is_clear(const coordinate_t &coordinate) {
	std::map<coordinate_t, screen_window_t>::iterator it = coordinate_text_map_.find(coordinate);
	return (coordinate_text_map_.end() != it && (NULL == it->second));
}
qnx_screen_display::~qnx_screen_display() {
	screen_destroy_window(screen_win_);
	screen_destroy_context(screen_ctx_);
	FT_Done_Face(face_);
  	FT_Done_FreeType(library_);
	if (screen_disps_) {
		free(screen_disps_);
		screen_disps_ = NULL;
	}
	// destory unused text
	std::map<coordinate_t, screen_window_t>::iterator it = coordinate_text_map_.begin();
	for (;it != coordinate_text_map_.end();++it) {
		screen_window_t screen_text_win = it->second;
		if (screen_text_win) {
			clear_text(screen_text_win);
		}
	}
}