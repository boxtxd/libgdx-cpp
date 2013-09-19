
/*
    Copyright 2011 Aevum Software aevum @ aevumlab.com

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    @author Victor Vicente de Carvalho victor.carvalho@aevumlab.com
    @author Ozires Bortolon de Faria ozires@aevumlab.com
*/

#include "gdx-cpp/gl.hpp"
#include <stddef.h>

#include "VertexBufferObjectSubData.hpp"
#include "gdx-cpp/Gdx.hpp"
#include "gdx-cpp/Log.hpp"
#include "gdx-cpp/graphics/GL11.hpp"
#include "gdx-cpp/graphics/GL20.hpp"
#include "gdx-cpp/graphics/VertexAttribute.hpp"
#include "gdx-cpp/graphics/glutils/ShaderProgram.hpp"

using namespace gdx;

int VertexBufferObjectSubData::createBufferObject () {
    if (gl20 != nullptr) {
        gl20->glGenBuffers(1, (unsigned int*) &tmpHandle);
        gl20->glBindBuffer(gdx::GL::ARRAY_BUFFER, tmpHandle);
        gl20->glBufferData(gdx::GL::ARRAY_BUFFER, byteBuffer.capacity(), nullptr, usage);
        gl20->glBindBuffer(gdx::GL::ARRAY_BUFFER, 0);
    } else {
        gl11->glGenBuffers(1, &tmpHandle);
        gl11->glBindBuffer(gdx::GL::ARRAY_BUFFER, tmpHandle);
        gl11->glBufferData(gdx::GL::ARRAY_BUFFER, byteBuffer.capacity(), nullptr, usage);
        gl11->glBindBuffer(gdx::GL::ARRAY_BUFFER, 0);
    }
    return tmpHandle;
}

VertexAttributes& VertexBufferObjectSubData::getAttributes () {
    return attributes;
}

int VertexBufferObjectSubData::getNumVertices () {
    return buffer.limit() * 4 / attributes.vertexSize;
}

int VertexBufferObjectSubData::getNumMaxVertices () {
    return byteBuffer.capacity() / attributes.vertexSize;
}

float_buffer& VertexBufferObjectSubData::getBuffer () {
    isDirty = true;
    return buffer;
}

void VertexBufferObjectSubData::setVertices (const float* vertices, int offset, int count) {
    isDirty = true;
    if (isDirect) {
        byteBuffer.copy(vertices , count , offset);
        buffer.position(0);
        buffer.limit(count);
    } else {
        buffer.clear();
        buffer.put(vertices, count, offset, count);
        buffer.flip();
        byteBuffer.position(0);
        byteBuffer.limit(buffer.limit() << 2);
    }

    if (isBound) {
        if (gl20 != nullptr) {
            GL20& gl = *gl20;
            gl.glBufferSubData(gdx::GL::ARRAY_BUFFER, 0, byteBuffer.limit(), byteBuffer);
        } else {
            GL11& gl = *gl11;
            gl.glBufferSubData(gdx::GL::ARRAY_BUFFER, 0, byteBuffer.limit(), byteBuffer);
        }
        isDirty = false;
    }
}

void VertexBufferObjectSubData::bind () {
    GL11& gl = *gl11;

    gl.glBindBuffer(gdx::GL::ARRAY_BUFFER, bufferHandle);
    if (isDirty) {
        byteBuffer.limit(buffer.limit() * 4);
        gl.glBufferSubData(gdx::GL::ARRAY_BUFFER, 0, byteBuffer.limit(), byteBuffer);
// gl.glBufferData(gdx::GL::ARRAY_BUFFER, byteBuffer.limit(),
// byteBuffer, usage);
        isDirty = false;
    }

    int textureUnit = 0;
    int numAttributes = attributes.size();

    for (int i = 0; i < numAttributes; i++) {
        VertexAttribute attribute = attributes.get(i);

        switch (attribute.usage) {
        case VertexAttributes::Usage::Position:
            gl.glEnableClientState(gdx::GL::VERTEX_ARRAY);
            gl.glVertexPointer(attribute.numComponents, gdx::GL::FLOAT, attributes.vertexSize, &attribute.offset);
            break;

        case VertexAttributes::Usage::Color:
        case VertexAttributes::Usage::ColorPacked:
        {
            int colorType = gdx::GL::FLOAT;
            if (attribute.usage == VertexAttributes::Usage::ColorPacked) colorType = gdx::GL::UNSIGNED_BYTE;

            gl.glEnableClientState(gdx::GL::COLOR_ARRAY);
            gl.glColorPointer(attribute.numComponents, colorType, attributes.vertexSize, &attribute.offset);
            break;
        }
        case VertexAttributes::Usage::Normal:
            gl.glEnableClientState(gdx::GL::NORMAL_ARRAY);
            gl.glNormalPointer(gdx::GL::FLOAT, attributes.vertexSize, &attribute.offset);
            break;

        case VertexAttributes::Usage::TextureCoordinates:
            gl.glClientActiveTexture(gdx::GL::TEXTURE0 + textureUnit);
            gl.glEnableClientState(gdx::GL::TEXTURE_COORD_ARRAY);
            gl.glTexCoordPointer(attribute.numComponents, gdx::GL::FLOAT, attributes.vertexSize, &attribute.offset);
            textureUnit++;
            break;

        default:{
            gdx_log_error("gdx","unkown vertex attribute type: %d", attribute.usage);
        }
        }
    }

    isBound = true;
}

void VertexBufferObjectSubData::bind (ShaderProgram& shader) {
    GL20& gl = *gl20;

    gl.glBindBuffer(gdx::GL::ARRAY_BUFFER, bufferHandle);
    if (isDirty) {
        byteBuffer.limit(buffer.limit() * 4);
        gl.glBufferSubData(gdx::GL::ARRAY_BUFFER, 0, byteBuffer.limit(), byteBuffer);
// gl.glBufferData(gdx::GL::ARRAY_BUFFER, byteBuffer.limit(),
// byteBuffer, usage);
        isDirty = false;
    }

    int numAttributes = attributes.size();
    for (int i = 0; i < numAttributes; i++) {
        VertexAttribute& attribute = attributes.get(i);
        shader.enableVertexAttribute(attribute.alias);
        int colorType = gdx::GL::FLOAT;
        bool normalize = false;
        if (attribute.usage == VertexAttributes::Usage::ColorPacked) {
            colorType = gdx::GL::UNSIGNED_BYTE;
            normalize = true;
        }
        shader.setVertexAttribute(attribute.alias, attribute.numComponents, colorType, normalize, attributes.vertexSize,
                                  attribute.offset);
    }
    isBound = true;
}

void VertexBufferObjectSubData::unbind () {
    GL11& gl = *gl11;
    int textureUnit = 0;
    int numAttributes = attributes.size();

    for (int i = 0; i < numAttributes; i++) {

        VertexAttribute& attribute = attributes.get(i);
        switch (attribute.usage) {
        case VertexAttributes::Usage::Position:
            break; // no-op, we also need a position bound in gles
        case VertexAttributes::Usage::Color:
        case VertexAttributes::Usage::ColorPacked:
            gl.glDisableClientState(gdx::GL::COLOR_ARRAY);
            break;
        case VertexAttributes::Usage::Normal:
            gl.glDisableClientState(gdx::GL::NORMAL_ARRAY);
            break;
        case VertexAttributes::Usage::TextureCoordinates:
            gl.glClientActiveTexture(gdx::GL::TEXTURE0 + textureUnit);
            gl.glDisableClientState(gdx::GL::TEXTURE_COORD_ARRAY);
            textureUnit++;
            break;
        default:
            gdx_log_error("gdx", "unkown vertex attribute type: %d", attribute.usage);        
        }
    }

    gl.glBindBuffer(gdx::GL::ARRAY_BUFFER, 0);
    isBound = false;
}

void VertexBufferObjectSubData::unbind (ShaderProgram& shader) {
    GL20& gl = *gl20;
    int numAttributes = attributes.size();
    for (int i = 0; i < numAttributes; i++) {
        VertexAttribute attribute = attributes.get(i);
        shader.disableVertexAttribute(attribute.alias);
    }
    gl.glBindBuffer(gdx::GL::ARRAY_BUFFER, 0);
    isBound = false;
}

void VertexBufferObjectSubData::invalidate () {
    bufferHandle = createBufferObject();
    isDirty = true;
}

void VertexBufferObjectSubData::dispose () {
    if (gl20 != nullptr) {
        tmpHandle = bufferHandle;
        
        GL20& gl = *gl20;
        gl.glBindBuffer(gdx::GL::ARRAY_BUFFER, 0);
        gl.glDeleteBuffers(1, (unsigned int*) &tmpHandle);

        bufferHandle = 0;
    } else {
        tmpHandle = bufferHandle;
        
        GL11& gl = *gl11;
        gl.glBindBuffer(gdx::GL::ARRAY_BUFFER, 0);
        gl.glDeleteBuffers(1, &tmpHandle);
        bufferHandle = 0;
    }
}

int VertexBufferObjectSubData::getBufferHandle () {
    return bufferHandle;
}

VertexBufferObjectSubData::VertexBufferObjectSubData(bool isStatic, int numVertices, const std::vector< VertexAttribute >& attributes)
        :
        attributes(attributes),
buffer(byteBuffer.convert<float>()),
byteBuffer(this->attributes.vertexSize * numVertices),
bufferHandle(0),
isDirect(true),
isStatic(isStatic),
usage(isStatic ? gdx::GL::STATIC_DRAW : gdx::GL::DYNAMIC_DRAW),
isDirty(false),
isBound(false),
tmpHandle(0)
{
    bufferHandle = createBufferObject();
    buffer.flip();
    byteBuffer.flip();
}


