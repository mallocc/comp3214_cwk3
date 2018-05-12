#pragma once

#include "glm.h"
#include "opengl.h"

struct FBO
{
	GLuint fbo, tex;

	int w = 0, h = 0;

	FBO() {}

	FBO(int w, int h)
	{
		this->w = w; this->h = h;
		getFrameBuffer(&fbo, &tex);
	}


	void bind()
	{
		// Render to our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		// Set the list of draw buffers.
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
	}

	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void getFrameBuffer(GLuint * FramebufferName, GLuint * renderedTexture)
	{
		*FramebufferName = 0;
		glGenFramebuffers(1, FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, *FramebufferName);
		// The texture we're going to render to
		glGenTextures(1, renderedTexture);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, *renderedTexture);

		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// The depth buffer
		GLuint depthrenderbuffer;
		glGenRenderbuffers(1, &depthrenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *renderedTexture, 0);
	}

	
};

//void get3DTexture(GLuint * tex, int w, int h, int d, GLuint * data)
//{
//	//create and bind texture
//	glGenTextures(1, tex);
//	glBindTexture(GL_TEXTURE_2D_ARRAY, *tex);
//
//	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, w, h, d, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//
//	// set texture wrapping to GL_REPEAT (default wrapping method)
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//	// set texture filtering parameters
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//	for (int i = 0; i < d; ++i)
//		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
//
//}