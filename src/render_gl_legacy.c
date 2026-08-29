
// Shared MiniGL library on AmigaOS
#if defined(AMIGA_SHARED_MINIGL)
	#include <proto/minigl.h>

// macOS
#elif defined(__APPLE__) && defined(__MACH__)
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>

	void glCreateTextures(GLuint ignored, GLsizei n, GLuint *name) {
		glGenTextures(1, name);
	}
// MorphOS
#elif defined(__MORPHOS__)
	// libSDL-mgl includes this header packed to 2 bytes; matching that here is
	// what makes GLcontext_t field offsets (w3dContext) agree with the context
	// the library actually allocated.
	#pragma pack(push, 2)
	#include "mgl/gl.h"
	#pragma pack(pop)
	
   // #define CUSTOM_OPENGL_IMPL 1
	
// Linux
#elif defined(__unix__)
	#include <GL/glew.h>

// Dreamcast
#elif defined(_arch_dreamcast)
	#include "gl.h"
	#include "glext.h"
	#include "glkos.h"

	#define CUSTOM_OPENGL_IMPL 1

// WINDOWS
#else
	#include <windows.h>

	#define GL3_PROTOTYPES 1
	#include <glew.h>
	#pragma comment(lib, "glew32.lib")

	#include <gl/GL.h>
	#pragma comment(lib, "opengl32.lib")
#endif


#include "libs/stb_image_write.h"

#include "render.h"
#include "mem.h"
#include "utils.h"

#undef RENDER_USE_MIPMAPS
#define RENDER_USE_MIPMAPS 0

#define NEAR_PLANE 256.0
#define FAR_PLANE 262144.0

#if defined(__MORPHOS__)
	/* MiniGL's default transformed-vertex buffer has 256 entries.  Leave
	 * enough headroom for worst-case clipping, which can expand one triangle
	 * to a polygon with up to nine vertices. */
	#define RENDER_TRIS_BUFFER_CAPACITY 16
#else
	#define RENDER_TRIS_BUFFER_CAPACITY 2048
#endif
#define TEXTURES_MAX 2048

typedef struct {
	vec2i_t size;
	vec2_t scale;
	GLuint texId;
} render_texture_t;

uint16_t RENDER_NO_TEXTURE;

void render_resize(vec2i_t size);

static tris_t __attribute__((aligned(32))) tris_buffer[RENDER_TRIS_BUFFER_CAPACITY];
static uint32_t tris_len = 0;
static float screen_2d_z = -1;
static bool render_view_is_2d = false;

static vec2i_t screen_size;

static render_blend_mode_t blend_mode = RENDER_BLEND_NORMAL;

#if 0//__GCC_VERSION__ >= 4
static mat4_t projection_mat_2d = mat4_identity();
static mat4_t projection_mat_3d = mat4_identity();
static mat4_t sprite_mat = mat4_identity();
static mat4_t view_mat = mat4_identity();
static mat4_t model_mat = mat4_identity();
#else
static mat4_t projection_mat_2d; //gcc4
static mat4_t projection_mat_3d;
static mat4_t sprite_mat;
static mat4_t view_mat;
static mat4_t model_mat;
#endif

static render_texture_t textures[TEXTURES_MAX];
static uint32_t textures_len = 0;
static uint16_t texture_index_prev = (uint16_t)0;

static void render_flush();
uint32_t upper_power_of_two(uint32_t v);
void render_textures_dump(const char *path);
void render_texture_dump(unsigned int textureNum);
void create_white_texture();

void render_init(vec2i_t size) {
	#if defined(__APPLE__) && defined(__MACH__)
		// OSX
		// (nothing to do here)
		// Dreamcast
	#elif defined(_arch_dreamcast)
		// (nothing to do here)
		int i=GL_DIRECT_BUFFER_KOS;

		GLdcConfig config;
    glKosInitConfig(&config);
    config.autosort_enabled = GL_TRUE;
    config.fsaa_enabled = GL_FALSE;
    /*@Note: These should be adjusted at some point */
    config.initial_op_capacity = 1024;
    config.initial_pt_capacity = 1024;
    config.initial_tr_capacity = 1024;
    config.initial_immediate_capacity = 0;
    glKosInitEx(&config);
	#elif defined(__MORPHOS__)
		// ??
	#else
		// Windows, Linux
		//glewExperimental = GL_TRUE;
		//glewInit();
	#endif

	// Defaults

	render_resize(size);
	render_set_view(vec3(0, 0, 0), vec3(0, 0, 0));
	render_set_model_mat(&mat4_identity());

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glEnable(GL_ALPHA_TEST);
  
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LESS);
	glDepthRange(0.0, 1.0);
	glClearDepth(1.0);
	mglSetZOffset(0.0f);
	glDisable(MGL_Z_OFFSET);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc (GL_GREATER, 0.0f);

	create_white_texture();
}

void create_white_texture(void) {
	// Dreamcast
#if defined(_arch_dreamcast) || defined(__MORPHOS__)
	// Create white texture
	rgba_t white_pixels[64];
	for(uint32_t i = 0;i < 64;i++)
	{
		white_pixels[i] = rgba(128,128,128,255);
	}
	RENDER_NO_TEXTURE = render_texture_create(8, 8, white_pixels);
#else
	// Create white texture
	rgba_t white_pixels[4] = {
		rgba(128,128,128,255), rgba(128,128,128,255),
		rgba(128,128,128,255), rgba(128,128,128,255)
	};
	RENDER_NO_TEXTURE = render_texture_create(2, 2, white_pixels);
#endif
}

void render_cleanup(void) {
	GLuint texId;

	/*
	 * This is final shutdown, not the in-game texture stack reset.  The old
	 * call used textures_len as the reset point, which deliberately deleted
	 * nothing and left all texture objects alive until context destruction.
	 * Some shared MiniGL backends retain that stale object state across a
	 * close/reopen cycle, corrupting geometry on the next program run.
	 */
	render_flush();
	glFinish();
	for (uint32_t i = 0; i < textures_len; i++) {
		texId = textures[i].texId;
		if (texId != 0)
			glDeleteTextures(1, &texId);
		textures[i].texId = 0;
	}
	textures_len = 0;
	texture_index_prev = (uint16_t)0;

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glFinish();
}

static void render_setup_2d_projection_mat(void) {

	float near = -1;
	float far = 1;
	float left = 0;
	float right = screen_size.x;
	float bottom = screen_size.y;
	float top = 0;
	float lr = 1 / (left - right);
	float bt = 1 / (bottom - top);
	float nf = 1 / (near - far);
  	projection_mat_2d = mat4(
		-2 * lr,  0,  0,  0,
		0,  -2 * bt,  0,  0,
		0,        0,  2 * nf,    0,
		(left + right) * lr, (top + bottom) * bt, (far + near) * nf, 1
	);
}

static void render_setup_3d_projection_mat() {
	// wipeout has a horizontal fov of 90deg, but we want the fov to be fixed
	// for the vertical axis, so that widescreen displays just have a wider
	// view. For the original 4/3 aspect ratio this equates to a vertial fov
	// of 73.75deg.
	float aspect = (float)screen_size.x / (float)screen_size.y;
	float fov = (73.75 / 180.0) * 3.14159265358;
	float f = 1.0 / tan(fov / 2);
	float nf = 1.0 / (NEAR_PLANE - FAR_PLANE);
	projection_mat_3d = mat4(
		f / aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (FAR_PLANE + NEAR_PLANE) * nf, -1,
		0, 0, 2 * FAR_PLANE * NEAR_PLANE * nf, 0
	);
}

void render_resize(vec2i_t size) {
	
	glViewport(0, 0, size.x, size.y);
	screen_size = size;

	render_setup_2d_projection_mat();
	render_setup_3d_projection_mat();
}

vec2i_t render_size() {
	return screen_size;
}

void render_set_resolution(render_resolution_t res) {}
void render_set_post_effect(render_post_effect_t post) {}
void render_set_screen_size(vec2i_t size) {}

void render_frame_prepare() {
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glDepthFunc(GL_LESS);
	glDepthRange(0.0, 1.0);
	glClearDepth(1.0);
	mglSetZOffset(0.0f);
	glDisable(MGL_Z_OFFSET);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[RENDER_NO_TEXTURE].texId);
}

void render_frame_end() {
	render_flush();
	// Dreamcast
	#if defined(_arch_dreamcast)
		/* Lets us yield to other threads*/
	
		glKosSwapBuffers();
	#endif
	screen_2d_z = -1;
}

void render_flush() {
	if (tris_len == 0) {
		return;
	}

	// Send all tris
	render_texture_t *t = &textures[texture_index_prev];
	glBindTexture(GL_TEXTURE_2D, t->texId);

#if defined(__MORPHOS__)
	/*
	 * Submit 2D quads as triangle strips on MiniGL.  Sending the two halves as
	 * independent GL_TRIANGLES makes the second half of a fullscreen image
	 * disappear on this backend.  render_push_2d_tile() always appends the two
	 * triangles of a quad next to each other, so reconstruct its four corners.
	 */
	if (render_view_is_2d) {
		uint32_t i = 0;
		for (; i + 1 < tris_len; i += 2) {
			vertex_t quad[4] __attribute__((aligned(32)));

			/* top-left, bottom-left, top-right, bottom-right */
			quad[0] = tris_buffer[i].vertices[2];
			quad[1] = tris_buffer[i].vertices[0];
			quad[2] = tris_buffer[i].vertices[1];
			quad[3] = tris_buffer[i + 1].vertices[0];

			glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), &quad[0].pos);
			glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), &quad[0].uv);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), &quad[0].color);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		/* Preserve support for a possible unpaired custom 2D triangle. */
		if (i < tris_len) {
			glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), &tris_buffer[i].vertices[0].pos);
			glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), &tris_buffer[i].vertices[0].uv);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), &tris_buffer[i].vertices[0].color);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}
	else
#endif
	{
		glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), &tris_buffer[0].vertices[0].pos);
		glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), &tris_buffer[0].vertices[0].uv);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), &tris_buffer[0].vertices[0].color);
		glDrawArrays(GL_TRIANGLES, 0, tris_len * 3);
	}

	tris_len = 0;
}


void render_set_view(vec3_t pos, vec3_t angles) {
	render_flush();
	render_view_is_2d = false;
	render_set_depth_write(true);
	render_set_depth_test(true);
	render_set_cull_backface(true);

	view_mat = mat4_identity();
	mat4_set_translation(&view_mat, vec3(0, 0, 0));
	mat4_set_roll_pitch_yaw(&view_mat, vec3(angles.x, -angles.y + M_PI, angles.z + M_PI));
	mat4_translate(&view_mat, vec3_inv(pos));
	mat4_set_yaw_pitch_roll(&sprite_mat, vec3(-angles.x, angles.y - M_PI, 0));

	render_set_model_mat(&mat4_identity());

    glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection_mat_3d.m);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view_mat.m);
	//glUniform2f(u_fade, RENDER_FADEOUT_NEAR, RENDER_FADEOUT_FAR);
}

void render_set_view_2d() {
	render_flush();
	render_view_is_2d = true;
	render_set_depth_test(false);
	render_set_depth_write(false);
	render_set_cull_backface(false);

	render_set_model_mat(&mat4_identity());

  	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection_mat_2d.m);
  	glMatrixMode(GL_MODELVIEW);

#ifdef __MORPHOS__
	mat4_t mat = mat4_identity();
	glLoadMatrixf(mat.m);
#else
	glLoadMatrixf(mat4_identity().m);
#endif
}

void render_set_model_mat(mat4_t *m) {
	memcpy(&model_mat, m, sizeof(mat4_t));
}

void render_push_matrix() {
	glPushMatrix();
	glMultMatrixf(model_mat.m);
}

void render_pop_matrix() {
	render_flush();
	glPopMatrix();
}

void render_set_depth_write(bool enabled) {
	render_flush();
	glDepthMask(enabled);
}

void render_set_depth_test(bool enabled) {
	render_flush();
	if (enabled) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}
}

void render_set_depth_offset(float offset) {
	render_flush();
	//offset = 0; //mgl
	if (offset == 0) {
		mglSetZOffset(offset);
		glDisable(MGL_Z_OFFSET);
		glDisable(GL_POLYGON_OFFSET_FILL);
		return;
	}
	mglSetZOffset(offset);
	glEnable(MGL_Z_OFFSET);
	glEnable(GL_POLYGON_OFFSET_FILL);
	// glPolygonOffset() is only present in the debug MiniGL build; mglSetZOffset()
	// above is the native equivalent and already applied the offset.
}

void render_set_screen_position(vec2_t pos) {
	render_flush();

	/*
	 * Match the shader renderer's `gl_Position.xy += screen * w`.  Adding a
	 * multiple of the projection matrix's W row performs the same translation
	 * directly in clip space and therefore does not depend on model distance.
	 */
	mat4_t shifted_projection = projection_mat_3d;
	for (int column = 0; column < 4; column++) {
		int row = column * 4;
		shifted_projection.m[row + 0] += pos.x * shifted_projection.m[row + 3];
		shifted_projection.m[row + 1] -= pos.y * shifted_projection.m[row + 3];
	}

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(shifted_projection.m);
	glMatrixMode(GL_MODELVIEW);
}

void render_set_blend_enabled(bool enabled) {
	render_flush();
	if (enabled) {
		glEnable(GL_BLEND);
	}
	else {
		glDisable(GL_BLEND);
	}
}

void render_set_blend_mode(render_blend_mode_t new_mode) {
	if (new_mode == blend_mode) {
		return;
	}
	render_flush();

	blend_mode = new_mode;
	if (blend_mode == RENDER_BLEND_NORMAL) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (blend_mode == RENDER_BLEND_LIGHTER) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
}

void render_set_cull_backface(bool enabled) {
	render_flush();
	if (enabled) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}
}

vec3_t render_transform(vec3_t pos) {
	return vec3_transform(vec3_transform(pos, &view_mat), &projection_mat_3d);
}

void render_push_tris(tris_t tris, uint16_t texture_index) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	if (tris_len >= RENDER_TRIS_BUFFER_CAPACITY) {
		render_flush();
	}
	if(texture_index != texture_index_prev){
		render_flush();
	}
	texture_index_prev = texture_index;

	render_texture_t *t = &textures[texture_index];

	for (int i = 0; i < 3; i++) {
		
		// resize back to (0,1) uv space
		tris.vertices[i].uv.x = (tris.vertices[i].uv.x / t->size.x) * t->scale.x;
		tris.vertices[i].uv.y = (tris.vertices[i].uv.y / t->size.y) * t->scale.y;
		if(tris.vertices[i].color.a == 0){
			continue;
		}
		/*
		// move colors back to (0,255)
		uint8_t R = tris.vertices[i].color.r;
		uint8_t G = tris.vertices[i].color.g;
		uint8_t B = tris.vertices[i].color.b;
		if(R == 128){
			R = 255;
		} else {
			R *=2;
		}
		if(G == 128){
			G = 255;
		} else {
			G *=2;
		}
		if(B == 128){
			B = 255;
		} else {
			B *=2;
		}
		tris.vertices[i].color.r = R;
		tris.vertices[i].color.g = G;
		tris.vertices[i].color.b = B;
		*/
		
		tris.vertices[i].color.r = clamp(tris.vertices[i].color.r * 2, 0, 255);
		tris.vertices[i].color.g = clamp(tris.vertices[i].color.g * 2, 0, 255);;
		tris.vertices[i].color.b = clamp(tris.vertices[i].color.b * 2, 0, 255);;
		
	}
	tris_buffer[tris_len++] = tris;
}

void render_push_sprite(vec3_t pos, vec2i_t size, rgba_t color, uint16_t texture_index) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	screen_2d_z += 0.001f;
	vec3_t p1 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5, -size.y * 0.5, screen_2d_z), &sprite_mat));
	vec3_t p2 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5, -size.y * 0.5, screen_2d_z), &sprite_mat));
	vec3_t p3 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5,  size.y * 0.5, screen_2d_z), &sprite_mat));
	vec3_t p4 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5,  size.y * 0.5, screen_2d_z), &sprite_mat));

	render_texture_t *t = &textures[texture_index];
	render_push_tris((tris_t){
		.vertices = {
			{
				.pos = p1,
				.uv = {0, 0},
				.color = color
			},
			{
				.pos = p2,
				.uv = {0 + t->size.x ,0},
				.color = color
			},
			{
				.pos = p3,
				.uv = {0, 0 + t->size.y},
				.color = color
			},
		}
	}, texture_index);
	render_push_tris((tris_t){
		.vertices = {
			{
				.pos = p3,
				.uv = {0, 0 + t->size.y},
				.color = color
			},
			{
				.pos = p2,
				.uv = {0 + t->size.x, 0},
				.color = color
			},
			{
				.pos = p4,
				.uv = {0 + t->size.x, 0 + t->size.y},
				.color = color
			},
		}
	}, texture_index);
}

void render_push_2d(vec2i_t pos, vec2i_t size, rgba_t color, uint16_t texture_index) {
	render_push_2d_tile(pos, vec2i(0, 0), render_texture_size(texture_index), size, color, texture_index);
}

void render_draw_2d_texture_alpha(vec2i_t pos, vec2i_t size, uint16_t texture_index) {
	render_texture_t *t;
	float x0, y0, x1, y1;

	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);
	render_flush();
	t = &textures[texture_index];
	x0 = pos.x + 0.5f;
	y0 = pos.y + 0.5f;
	x1 = pos.x + size.x - 0.5f;
	y1 = pos.y + size.y - 0.5f;
	screen_2d_z += 0.001f;

	/* Match the known-good MiniGL alpha diagnostic exactly: do not involve
	 * GLColorPointer; take colour and alpha solely from the RGBA texture. */
	glBindTexture(GL_TEXTURE_2D, t->texId);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);             glVertex4f(x0, y0, screen_2d_z, 1.0f);
	glTexCoord2f(t->scale.x, 0.0f);       glVertex4f(x1, y0, screen_2d_z, 1.0f);
	glTexCoord2f(t->scale.x, t->scale.y); glVertex4f(x1, y1, screen_2d_z, 1.0f);
	glTexCoord2f(0.0f, t->scale.y);       glVertex4f(x0, y1, screen_2d_z, 1.0f);
	glEnd();
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_ALPHA_TEST);
}

void render_push_2d_tile(vec2i_t pos, vec2i_t uv_offset, vec2i_t uv_size, vec2i_t size, rgba_t color, uint16_t texture_index) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	screen_2d_z += 0.001f;
	float x0 = pos.x;
	float y0 = pos.y;
	float x1 = pos.x + size.x;
	float y1 = pos.y + size.y;

#if defined(__MORPHOS__)
	/*
	 * Keep fullscreen vertices away from MiniGL's exact clip boundaries.
	 * 0.5 and size-0.5 are the centres of the first and last framebuffer
	 * pixels, so the quad still covers the complete viewport without invoking
	 * the buggy boundary-clipping path for its lower-right triangle.
	 */
	if (pos.x == 0 && pos.y == 0 &&
		size.x == screen_size.x && size.y == screen_size.y) {
		x0 = 0.5f;
		y0 = 0.5f;
		x1 = screen_size.x - 0.5f;
		y1 = screen_size.y - 0.5f;
	}
#endif

	render_push_tris((tris_t){
		.vertices = {
			{
				.pos = {x0, y1, screen_2d_z},
				.uv = {uv_offset.x , uv_offset.y + uv_size.y},
				.color = color
			},
			{
				.pos = {x1, y0, screen_2d_z},
				.uv = {uv_offset.x +  uv_size.x, uv_offset.y},
				.color = color
			},
			{
				.pos = {x0, y0, screen_2d_z},
				.uv = {uv_offset.x , uv_offset.y},
				.color = color
			},
		}
	}, texture_index);

	render_push_tris((tris_t){
		.vertices = {
			{
				.pos = {x1, y1, screen_2d_z},
				.uv = {uv_offset.x + uv_size.x, uv_offset.y + uv_size.y},
				.color = color
			},
			{
				.pos = {x1, y0, screen_2d_z},
				.uv = {uv_offset.x + uv_size.x, uv_offset.y},
				.color = color
			},
			{
				.pos = {x0, y1, screen_2d_z},
				.uv = {uv_offset.x , uv_offset.y + uv_size.y},
				.color = color
			},
		}
	}, texture_index);
}

uint32_t upper_power_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
#define GL_CLAMP_TO_EDGE                  0x812F

#if 0
void resize_texture(rgba_t* original_pixels, int original_width, int original_height, rgba_t* resized_pixels) {

    int new_width;
	int new_height;

	if (original_width > 256)
    	new_width = 256;
	else
		new_width = original_width;

	if (original_height > 256)
    	new_height = 256;
	else
		new_height = original_height;


    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int orig_x = (x * original_width) / new_width;
            int orig_y = (y * original_height) / new_height;

            rgba_t original_pixel = original_pixels[orig_y * original_width + orig_x];
            resized_pixels[y * new_width + x] = original_pixel;
        }
    }
}

#endif
uint16_t render_texture_create(uint32_t tw, uint32_t th, rgba_t *pixels) {
	error_if(textures_len >= TEXTURES_MAX, "TEXTURES_MAX reached");
	rgba_t empty_pixel = rgba(0, 0, 0, 0);

	/*
	 * Some CMP archives contain intentional empty entries (for example 0x1).
	 * They still occupy a slot in the archive's texture index table.  Returning
	 * texture zero here would shift every following entry, so keep the slot as
	 * a transparent placeholder instead.
	 */
	if (tw == 0 || th == 0) {
		tw = 1;
		th = 1;
		pixels = &empty_pixel;
	}

	void *_pixels = pixels;
	rgba_t *pb = 0x0;
	uint32_t tex_width = tw;
	uint32_t tex_height = th;
	uint32_t used_width = tw;
	uint32_t used_height = th;
	
	//printf("padding texture 1  (%3d x %3d) -> (%3d x %3d)\n", tw, th, tex_width, tex_height);


	// Dreamcast, or other platform that requires pow2 textures
#if defined(_arch_dreamcast) || defined(__MORPHOS__)
	{
		const uint32_t min_texture_size = 8;

	#if defined(__MORPHOS__)
		const uint32_t max_texture_size = 256;
		uint32_t largest_side = max(tw, th);
		/* Keep the aspect ratio when an asset exceeds the Warp3D limit. */
		if (largest_side > max_texture_size) {
			used_width = max(1u, (tw * max_texture_size) / largest_side);
			used_height = max(1u, (th * max_texture_size) / largest_side);
		}
	#endif

		tex_width = max(min_texture_size, upper_power_of_two(used_width));
		tex_height = max(min_texture_size, upper_power_of_two(used_height));

		/*
		 * Allocate one buffer for both resampling and power-of-two padding.
		 * This also makes 1-pixel-wide/high assets legal instead of silently
		 * replacing them with texture zero.
		 */
		if (used_width != tw || used_height != th ||
			tex_width != tw || tex_height != th) {
			pb = mem_temp_alloc(sizeof(rgba_t) * tex_width * tex_height);
			memset(pb, 0, sizeof(rgba_t) * tex_width * tex_height);

			for (uint32_t y = 0; y < used_height; y++) {
				uint32_t src_y = (y * th) / used_height;
				for (uint32_t x = 0; x < used_width; x++) {
					uint32_t src_x = (x * tw) / used_width;
					pb[y * tex_width + x] = pixels[src_y * tw + src_x];
				}
			}
			_pixels = pb;
		}
	}
#endif

	GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);//GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);//GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _pixels);

	if(pb){
		mem_temp_free(pb);
	}

	uint16_t texture_index = textures_len;
	textures_len++;
	textures[texture_index] = (render_texture_t){
		{tw, th},
		{((float)used_width) / ((float)tex_width), ((float)used_height) / ((float)tex_height)},
		texId
	};

	//printf("created texture (%3d x %3d) size %dkb\n", tw, th, (tw*th)/1024);
	//render_texture_dump(texture_index);
	return texture_index;
}

vec2i_t render_texture_size(uint16_t texture_index) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);
	return textures[texture_index].size;
}

// Only used by pl_mpeg for intro video
void render_texture_replace_pixels(int16_t texture_index, rgba_t *pixels) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	render_texture_t *t = &textures[texture_index];
	glBindTexture(GL_TEXTURE_2D, t->texId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->size.x, t->size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

uint16_t render_textures_len() {
	return textures_len;
}

void render_textures_reset(uint16_t len) {
	error_if(len > textures_len, "Invalid texture reset len %d >= %d", len, textures_len);
	//printf("render_textures_reset: resetting to %d of %d\n", len, textures_len);
	render_flush();
	GLuint texId;

	// Clear completely and recreate the default white texture
	if (len == 0) {
		for(int i=0;i < textures_len; i++){
			texId = textures[i].texId;
			glDeleteTextures(1, &texId);
		}
		create_white_texture();
		return;
	}

	// Delete everything above
	for(int i=len;i < textures_len; i++){
		texId = textures[i].texId;
		glDeleteTextures(1, &texId);
	}

	textures_len = len;
}

#if defined(CUSTOM_OPENGL_IMPL)
void render_textures_dump(const char *path) { }
void render_texture_dump(unsigned int textureNum) { }
#else
void render_textures_dump(const char *path) {
	/*int width = ATLAS_SIZE * ATLAS_GRID;
	int height = ATLAS_SIZE * ATLAS_GRID;
	rgba_t *pixels = malloc(sizeof(rgba_t) * width * height);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	stbi_write_png(path, width, height, 4, pixels, 0);
	free(pixels);*/
	
}

void render_texture_dump(unsigned int textureNum) {/*
	int width = textures[textureNum].size.x;
	int height = textures[textureNum].size.y;
	width = upper_power_of_two(width);
	height = upper_power_of_two(height);

	rgba_t *pixels = malloc(sizeof(rgba_t) * width * height);
	glBindTexture(GL_TEXTURE_2D, textures[textureNum].texId);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	char path[64];
	memset(path, 0, 64);
	sprintf(path,"dump/texture_%d.png", textureNum);
	stbi_write_png(path, width, height, 4, pixels, 0);
	free(pixels);
	*/
}
#endif
