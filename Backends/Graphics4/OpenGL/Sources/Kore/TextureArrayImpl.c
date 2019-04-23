#include "pch.h"

#include <Kinc/Graphics4/TextureArray.h>
#include <Kinc/Graphics4/TextureUnit.h>

#include <Kore/ogl.h>

void Kinc_G4_TextureArray_Create(Kinc_G4_TextureArray *array, Kinc_Image **textures, int count) {
#ifdef GL_VERSION_4_2
	glGenTextures(1, &array->impl.texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, array->impl.texture);
	// glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, textures[0]->width, textures[0]->height, count, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, textures[0]->width, textures[0]->height, count);
	for (int i = 0; i < count; ++i) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, textures[i]->width, textures[i]->height, 1, GL_RGBA, GL_UNSIGNED_BYTE, textures[i]->data);
	}
#endif
}

void Kinc_G4_Internal_TextureArraySet(Kinc_G4_TextureArray *array, Kinc_G4_TextureUnit unit) {
#ifdef GL_VERSION_4_2
	glActiveTexture(GL_TEXTURE0 + unit.impl.unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, array->impl.texture);
#endif
}