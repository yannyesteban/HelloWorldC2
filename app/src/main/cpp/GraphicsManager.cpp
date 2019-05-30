//
// Created by yanny on 25/5/2019.
//

#include "GraphicsManager.h"

//#include <string.h>
#include <cstring>

#include "Log.h"
//#include "../../../../../../../sdk/ndk-bundle/sources/libpng/jni/png.h"

#include <png.h>
void callback_readPng(png_structp pStruct, png_bytep pData, png_size_t pSize) {
    Resource* resource = ((Resource*) png_get_io_ptr(pStruct));
    if (resource->read(pData, pSize) != STATUS_OK) {
        resource->close();
    }
}
GraphicsManager::GraphicsManager(android_app* pApplication) :
        mApplication(pApplication),
        mRenderWidth(0), mRenderHeight(0),

        mDisplay(EGL_NO_DISPLAY),
        mSurface(EGL_NO_SURFACE),
        mContext(EGL_NO_CONTEXT),
        mProjectionMatrix(),
        mTextures(), mTextureCount(0),
        //mElements(), mElementCount(0),
        mShaders(), mShaderCount(0),
        mComponents(), mComponentCount(0){
    Log::info("Creating GraphicsManager.");
}
GraphicsManager::~GraphicsManager() {
    Log::info("Destroying GraphicsManager.");
    /*for (int32_t i = 0; i < mElementCount; ++i) {
        delete mElements[i];
    }*/
}
void GraphicsManager::registerComponent(GraphicsComponent* pComponent)
{
    mComponents[mComponentCount++] = pComponent;
}
/*
GraphicsElement* GraphicsManager::registerElement(int32_t pHeight, int32_t pWidth) {
    mElements[mElementCount] = new GraphicsElement(pHeight, pWidth);
    return mElements[mElementCount++];
}
*/
status GraphicsManager::start() {
    Log::info("Starting GraphicsManager.");

    EGLint format, numConfigs, errorResult; GLenum status;
    EGLConfig config;
// Defines display requirements. 16bits mode here.
    const EGLint DISPLAY_ATTRIBS[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 5,
            EGL_GREEN_SIZE, 6,
            EGL_RED_SIZE, 5,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };
// Request an OpenGL ES 2 context.
    const EGLint CONTEXT_ATTRIBS[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (mDisplay == EGL_NO_DISPLAY)
        goto ERROR;
    if (!eglInitialize(mDisplay, NULL, NULL))
        goto ERROR;
    if(!eglChooseConfig(mDisplay, DISPLAY_ATTRIBS, &config, 1, &numConfigs) || (numConfigs <= 0))
        goto ERROR;

    if (!eglGetConfigAttrib(mDisplay, config, EGL_NATIVE_VISUAL_ID, &format))
        goto ERROR;

    ANativeWindow_setBuffersGeometry(mApplication->window, 0, 0, format);
    mSurface = eglCreateWindowSurface(mDisplay, config, mApplication->window, NULL);

    if (mSurface == EGL_NO_SURFACE)
        goto ERROR;
    mContext = eglCreateContext(mDisplay, config, NULL, CONTEXT_ATTRIBS);
    if (mContext == EGL_NO_CONTEXT)
        goto ERROR;
    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)
       || !eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &mRenderWidth)
       || !eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mRenderHeight)
       || (mRenderWidth <= 0) || (mRenderHeight <= 0))
        goto ERROR;

    glViewport(0, 0, mRenderWidth, mRenderHeight);
    glDisable(GL_DEPTH_TEST);

    // Prepares the projection matrix with viewport dimesions.
    memset(mProjectionMatrix[0], 0, sizeof(mProjectionMatrix));
    mProjectionMatrix[0][0] = 2.0f / GLfloat(mRenderWidth);
    mProjectionMatrix[1][1] = 2.0f / GLfloat(mRenderHeight);
    mProjectionMatrix[2][2] = -1.0f; mProjectionMatrix[3][0] = -1.0f;
    mProjectionMatrix[3][1] = -1.0f; mProjectionMatrix[3][2] = 0.0f;
    mProjectionMatrix[3][3] = 1.0f;
// Loads graphics components.
    for (int32_t i = 0; i < mComponentCount; ++i) {
        if (mComponents[i]->load() != STATUS_OK) {
            return STATUS_KO;
        }
    }

    return STATUS_OK;
    ERROR:
        Log::error("Error while starting GraphicsManager");
        stop();
        return STATUS_KO;

    /*
    // Forces 32 bits format.
    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_setBuffersGeometry(mApplication->window,
                                         0, 0,
                                         WINDOW_FORMAT_RGBX_8888) < 0) {
        Log::info("Error while setting buffer geometry.");
        return STATUS_KO;
    }
    // Needs to lock the window buffer to get its properties.
    if (ANativeWindow_lock(mApplication->window, &windowBuffer, nullptr) >= 0) {
        mRenderWidth = windowBuffer.width;
        mRenderHeight = windowBuffer.height;
        ANativeWindow_unlockAndPost(mApplication->window);
    } else {
        Log::info("Error while locking window.");
        return STATUS_KO;
    }
    return STATUS_OK;
     */
}

void GraphicsManager::stop() {
    Log::info("Stopping GraphicsManager.");
    for (int32_t i = 0; i < mTextureCount; ++i) {
        glDeleteTextures(1, &mTextures[i].texture);
    }
    mTextureCount = 0;
    for (int32_t i = 0; i < mShaderCount; ++i) {
        glDeleteProgram(mShaders[i]);
    }
    mShaderCount = 0;
    // Destroys OpenGL context.
    if (mDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (mContext != EGL_NO_CONTEXT) {
            eglDestroyContext(mDisplay, mContext);
            mContext = EGL_NO_CONTEXT;
        }
        if (mSurface != EGL_NO_SURFACE) {
            eglDestroySurface(mDisplay, mSurface);
            mSurface = EGL_NO_SURFACE;
        }
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
    }
}
status GraphicsManager::update() {
    static float clearColor = 0.0f;
    clearColor += 0.001f;
    glClearColor(clearColor, clearColor, clearColor, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int32_t i = 0; i < mComponentCount; ++i) {
        mComponents[i]->draw();
    }

    if (eglSwapBuffers(mDisplay, mSurface) != EGL_TRUE) {
        Log::error("Error %d swapping buffers.", eglGetError());
        return STATUS_KO;
    } else {
        return STATUS_OK;
    }
}

TextureProperties* GraphicsManager::loadTexture(Resource& pResource) {
    for (int32_t i = 0; i < mTextureCount; ++i) {
        if (pResource == *mTextures[i].textureResource) {
            Log::info("Found %s in cache", pResource.getPath());
            return &mTextures[i];
        }
    }
    /*****************/
    Log::info("Loading texture %s", pResource.getPath());
    TextureProperties* textureProperties; GLuint texture; GLint format;
    png_byte header[8];
    png_structp pngPtr = NULL; png_infop infoPtr = NULL;
    png_byte* image = NULL; png_bytep* rowPtrs = NULL;
    png_int_32 rowSize; bool transparency;
    if (pResource.open() != STATUS_OK) goto ERROR;
    Log::info("Checking signature.");
    if (pResource.read(header, sizeof(header)) != STATUS_OK)
        goto ERROR;
    if (png_sig_cmp(header, 0, 8) != 0) goto ERROR;

    /******************/

    Log::info("Creating required structures.");
    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngPtr) goto ERROR;
    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) goto ERROR;
// Prepares reading operation by setting-up a read callback.
    png_set_read_fn(pngPtr, &pResource, callback_readPng);
// Set-up error management. If an error occurs while reading,
// code will come back here and jump
    if (setjmp(png_jmpbuf(pngPtr))) goto ERROR;


    /*******************/
    // Ignores first 8 bytes already read.
    png_set_sig_bytes(pngPtr, 8);
// Retrieves PNG info and updates PNG struct accordingly.
    png_read_info(pngPtr, infoPtr);
    png_int_32 depth, colorType;
    png_uint_32 width, height;
    png_get_IHDR(pngPtr, infoPtr, &width, &height,
                 &depth, &colorType, NULL, NULL, NULL);

    /*********/

    // Creates a full alpha channel if transparency is encoded as
// an array of palette entries or a single transparent color.
    transparency = false;
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(pngPtr);
        transparency = true;
    }
// Expands PNG with less than 8bits per channel to 8bits.
    if (depth < 8) {
        png_set_packing (pngPtr);
// Shrinks PNG with 16bits per color channel down to 8bits.
    } else if (depth == 16) {
        png_set_strip_16(pngPtr);
    }
// Indicates that image needs conversion to RGBA if needed.
    switch (colorType) {
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(pngPtr);
            format = transparency ? GL_RGBA : GL_RGB;
            break;
        case PNG_COLOR_TYPE_RGB:
            format = transparency ? GL_RGBA : GL_RGB;
            break;
        case PNG_COLOR_TYPE_RGBA:
            format = GL_RGBA;
            break;
        case PNG_COLOR_TYPE_GRAY:
            png_set_expand_gray_1_2_4_to_8(pngPtr);
            format = transparency ? GL_LUMINANCE_ALPHA:GL_LUMINANCE;
            break;
        case PNG_COLOR_TYPE_GA:
            png_set_expand_gray_1_2_4_to_8(pngPtr);
            format = GL_LUMINANCE_ALPHA;
            break;
    }
// Validates all transformations.
    png_read_update_info(pngPtr, infoPtr);
    /******************/
// Get row size in bytes.
    rowSize = png_get_rowbytes(pngPtr, infoPtr);
    if (rowSize <= 0) goto ERROR;
// Ceates the image buffer that will be sent to OpenGL.
    image = new png_byte[rowSize * height];
    if (!image) goto ERROR;
// Pointers to each row of the image buffer. Row order is
// inverted because different coordinate systems are used by
// OpenGL (1st pixel is at bottom left) and PNGs (top-left).
    rowPtrs = new png_bytep[height];
    if (!rowPtrs) goto ERROR;
    for (int32_t i = 0; i < height; ++i) {
        rowPtrs[height - (i + 1)] = image + i * rowSize;
    }

// Reads image content.
    png_read_image(pngPtr, rowPtrs);
// Frees memory and resources.
    pResource.close();
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
    delete[] rowPtrs;

    GLenum errorResult;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Set-up texture properties.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Loads image data into OpenGL.
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, image);
    // Finished working with the texture.
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] image;
    if (glGetError() != GL_NO_ERROR) goto ERROR;
    Log::info("Texture size: %d x %d", width, height);

    // Caches the loaded texture.
    textureProperties = &mTextures[mTextureCount++];
    textureProperties->texture = texture;
    textureProperties->textureResource = &pResource;
    textureProperties->width = width;
    textureProperties->height = height;
    return textureProperties;

    ERROR:
    Log::error("Error loading texture into OpenGL.");
    pResource.close();
    delete[] rowPtrs; delete[] image;
    if (pngPtr != NULL) {
        png_infop* infoPtrP = infoPtr != NULL ? &infoPtr: NULL;
        png_destroy_read_struct(&pngPtr, infoPtrP, NULL);
    }
    return NULL;
}

GLuint GraphicsManager::loadShader(const char* pVertexShader,
                                   const char* pFragmentShader) {
    GLint result; char log[256];
    GLuint vertexShader, fragmentShader, shaderProgram;
// Builds the vertex shader.
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &pVertexShader, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        glGetShaderInfoLog(vertexShader, sizeof(log), 0, log);
        Log::error("Vertex shader error: %s", log);
        goto ERROR;
    }
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &pFragmentShader, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        glGetShaderInfoLog(fragmentShader, sizeof(log), 0, log);
        Log::error("Fragment shader error: %s", log);
        goto ERROR;
    }

    /**********/
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (result == GL_FALSE) {
        glGetProgramInfoLog(shaderProgram, sizeof(log), 0, log);
        Log::error("Shader program error: %s", log);
        goto ERROR;
    }

    mShaders[mShaderCount++] = shaderProgram;
    return shaderProgram;
    ERROR:
    Log::error("Error loading shader.");
    if (vertexShader > 0) glDeleteShader(vertexShader);
    if (fragmentShader > 0) glDeleteShader(fragmentShader);
    return 0;
}
// Builds the fragment shader.


/*
status GraphicsManager::update() {
// Locks the window buffer and draws on it.
    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_lock(mApplication->window, &windowBuffer, nullptr) < 0) {
        Log::info("Error while starting GraphicsManager");
        return STATUS_KO;
    }
// Clears the window.
    memset(windowBuffer.bits, 0, windowBuffer.stride * windowBuffer.height * sizeof(int32_t));
    //memset(windowBuffer.bits, 0, windowBuffer.stride * windowBuffer.height*4);
    // Renders graphic elements.
    int32_t maxX = windowBuffer.width - 1;
    int32_t maxY = windowBuffer.height - 1;
    for (int32_t i = 0; i < mElementCount; ++i) {
        GraphicsElement* element = mElements[i];
// Computes coordinates.
        int32_t leftX = element->location.x - element->width / 2;
        int32_t rightX = element->location.x + element->width / 2;
        int32_t leftY = windowBuffer.height - element->location.y - element->height / 2;
        int32_t rightY = windowBuffer.height - element->location.y + element->height / 2;
// Clips coordinates.
        if (rightX < 0 || leftX > maxX || rightY < 0 || leftY > maxY)
            continue;
        if (leftX < 0)
            leftX = 0;
        else if (rightX > maxX)
            rightX = maxX;
        if (leftY < 0)
            leftY = 0;
        else if (rightY > maxY)
            rightY = maxY;
        // Draws a rectangle.
        uint32_t* line = (uint32_t*) (windowBuffer.bits) + (windowBuffer.stride * leftY);
        for (int iY = leftY; iY <= rightY; iY++) {
            for (int iX = leftX; iX <= rightX; iX++) {

                line[iX] = 0X001010FF; // Red color
            }
            line = line + windowBuffer.stride;
        }
    }
    // Finshed drawing.
    ANativeWindow_unlockAndPost(mApplication->window);
    Log::error("Pintando");
    return STATUS_OK;
}

 */

