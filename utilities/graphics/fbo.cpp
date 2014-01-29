#include "stdafx.h"
#include "fbo.h"

namespace augs {
	namespace graphics {
		GLuint fbo::currently_bound_fbo = 0u;

		fbo::fbo() : created(false), fboId(0u), textureId(0u) {
		}

		fbo::fbo(int width, int height) : created(false) {
			create(width, height);
		}

		void fbo::create(int w, int h) {
			if (created) return;

			created = true;
			width = w;
			height = h;

			glGenTextures(1, &textureId);
			glBindTexture(GL_TEXTURE_2D, textureId);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glGenFramebuffers(1, &fboId);
			use();

			// attach the texture to FBO color attachment point
			glFramebufferTexture2D(GL_FRAMEBUFFER,        // 1. fbo target: GL_FRAMEBUFFER 
				GL_COLOR_ATTACHMENT0,  // 2. attachment point
				GL_TEXTURE_2D,         // 3. tex target: GL_TEXTURE_2D
				textureId,             // 4. tex ID
				0);                    // 5. mipmap level: 0(base)

			// check FBO status
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "an error occured during FBO creation" << std::endl;
		}

		void fbo::use() {
			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			currently_bound_fbo = fboId;
		}

		void fbo::guarded_use() {
			if (currently_bound_fbo != fboId)
				use();
		}

		void fbo::use_default() {
			if (currently_bound_fbo != 0) {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				currently_bound_fbo = 0;
			}
		}

		int fbo::get_width() const {
			return width;
		}

		int fbo::get_height() const {
			return height;
		}

		void fbo::destroy() {
			created = false;
			width = 0;
			height = 0;
			fboId = 0;
			textureId = 0;
			glDeleteFramebuffers(1, &fboId);
			glDeleteTextures(1, &textureId);
		}

		fbo::~fbo() {
			destroy();
		}

		GLuint fbo::get_texture_id() const {
			return textureId;
		}
	}
}