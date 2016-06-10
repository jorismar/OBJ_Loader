#include "Model3D.h"

Model3D::Model3D() {
	this->object		= NULL;
	this->scene_list	= 0;

	this->ar_scale[0]	= 1.0;
	this->ar_scale[1]	= 1.0;
	this->ar_scale[2]	= 1.0;
}

//***********************************************************************************************************************

Model3D::~Model3D() {
	// NO IMPLEMENTATION NEEDED.
}

//***********************************************************************************************************************

bool Model3D::loadModel(const std::string& path) {
	this->path = path;

	// Check if file exists
	std::ifstream filein(this->path.c_str());		

	// Check and capture error
	if (filein.fail()) {
		std::cout << "The requested file not found." << std::endl;
		return false;
	}

	filein.close();

	// Load model
	this->object = this->importer.ReadFile(this->path, aiProcessPreset_TargetRealtime_Quality);		

	// Checks if object was loaded.
	if (!this->object) {
		std::cout << importer.GetErrorString() << std::endl;
		return false;
	}

	return true;
}

//***********************************************************************************************************************

bool Model3D::loadTexture() {
	// Check DevIL version
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION) {
		std::cout << "Wrong DevIL version. Old devil.dll in system32/SysWow64?" << std::endl;
		return false;
	}

	// Initializes DevIL
	ilInit();		

	// Checks if the model has textures
	if (this->object->HasTextures()) {
		std::cout << "Support for meshes with embedded textures is not implemented" << std::endl;
		return false;
	}

	// Get the textures paths from material descriptor
	for (unsigned int m = 0; m < this->object->mNumMaterials; m++) {
		aiString texture_path;

		for (unsigned int t = 0; AI_SUCCESS == this->object->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, t, &texture_path); t++) {
			this->textures[texture_path.data] = NULL;
		}
	}

	unsigned int num_textures= this->textures.size();	// Number of textures
	GLuint* texture_ids	= new GLuint[num_textures];		// Array to storage textures ids
	ILuint* img_ids	= new ILuint[num_textures];			// Array to storage images ids

	// Generation of DevIL image ids
	ilGenImages(num_textures, img_ids);

	// Generation of Textures ids
	glGenTextures(num_textures, texture_ids);

	// Get the model folder path
	size_t pos = this->path.find_last_of("\\/");
	std::string basepath = (std::string::npos == pos) ? "" : this->path.substr(0, pos + 1);

	// Get iterator to sequential access of map
	std::map<std::string, GLuint*>::iterator itr = this->textures.begin();

	// Read all textures of model
	for (unsigned int i = 0; i < num_textures; i++) {
		std::string filename = basepath + (*itr).first;	// Get filename
		(*itr).second = &texture_ids[i];				// Save texture id of this texture

		// Binding image id
		ilBindImage(img_ids[i]);				

		// Load image
		if (!ilLoadImage((ILstring)filename.c_str())) {
			std::cout << "Failed to read the image " + filename << std::endl;
			return false;
		}

		// Convert every colour component into unsigned byte.
		if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
			std::cout << "Couldn't convert image" << std::endl;
			return false;
		}
		
		// Binding of texture id
		glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
		
		// Use Linear Interpolation for magnification filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Use Linear Interpolation for minifying filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		// Texture specification
		glTexImage2D(GL_TEXTURE_2D, 
			0, 
			ilGetInteger(IL_IMAGE_BPP), 
			ilGetInteger(IL_IMAGE_WIDTH),
			ilGetInteger(IL_IMAGE_HEIGHT), 
			0, 
			ilGetInteger(IL_IMAGE_FORMAT),
			GL_UNSIGNED_BYTE,
			ilGetData()
		);

		itr++;											// Next texture
	}

	// Release memory used by image.
	ilDeleteImages(num_textures, img_ids);

	// Cleanup
	delete[] img_ids;
	img_ids = NULL;

	return true;
}

//***********************************************************************************************************************

void Model3D::render() {
	// Render and map the texture
	recursive_render(this->object, this->object->mRootNode);
}

//***********************************************************************************************************************

void Model3D::scale(double factor) {
	this->ar_scale[0] = factor;
	this->ar_scale[1] = factor;
	this->ar_scale[2] = factor;
}

//***********************************************************************************************************************

void Model3D::scale(double x, double y, double z) {
	// Sets the scale factor (scale is applied during rendering)
	this->ar_scale[0] = x;
	this->ar_scale[1] = y;
	this->ar_scale[2] = z;
}

//************************************************ PRIVATE FUNCTIONS ************************************************

void Model3D::apply_material(const aiMaterial *mtl)
{
	float color[4];

	for (unsigned int t = 0; t < this->textures.size(); t++) {

		{ // Bind texture
			aiString texture_path;

			if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path))
				glBindTexture(GL_TEXTURE_2D, *this->textures[texture_path.data]);
		}

		{ // Apply the diffuse illumination attributes of the material
			aiColor4D diffuse;

			PUSH_TO_ARRAY4(color, 1.0f, 1.0f, 1.0f, 1.0f);

			if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
				PUSH_TO_ARRAY4(color, diffuse.r, diffuse.g, diffuse.b, diffuse.a);

			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
		}

		{ // Apply the specular illumination attributes of the material
			aiColor4D specular;

			PUSH_TO_ARRAY4(color, 0.0f, 0.0f, 0.0f, 1.0f);

			if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
				PUSH_TO_ARRAY4(color, specular.r, specular.g, specular.b, specular.a);

			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
		}
		
		{ // Apply the ambient illumination attributes of the material
			aiColor4D ambient;

			PUSH_TO_ARRAY4(color, 1.0f, 1.0f, 1.0f, 1.0f);

			if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
				PUSH_TO_ARRAY4(color, ambient.r, ambient.g, ambient.b, ambient.a);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
		}

		{ // Apply the lighting emission attributes of the material
			aiColor4D emission;

			PUSH_TO_ARRAY4(color, 0.0f, 0.0f, 0.0f, 1.0f);

			if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
				PUSH_TO_ARRAY4(color, emission.r, emission.g, emission.b, emission.a);

			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
		}

		unsigned int max = 1;
		
		{ // Apply the shininess of the material
			float shininess;//, strength;

			if ((aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max) == AI_SUCCESS)) // && (aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max) == AI_SUCCESS))
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess); // * strength);
			else {
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
				PUSH_TO_ARRAY4(color, 0.0f, 0.0f, 0.0f, 1.0f);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
			}
		}

		{ // Sets the fill of the faces
			int wireframe = 0;

			aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max);
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
		}

		{ // Enable the Backface Culling
			int two_sided;

			if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
				glEnable(GL_CULL_FACE);
			else glDisable(GL_CULL_FACE);
		}
	}
}

//***********************************************************************************************************************

void Model3D::recursive_render(const struct aiScene *sc, const struct aiNode* nd) {
	glPushMatrix(); {
		// Apply scale
		glScalef(this->ar_scale[0], this->ar_scale[1], this->ar_scale[2]);

		// Draw all meshes assigned to this node
		for (unsigned int n = 0; n < nd->mNumMeshes; n++) {
			unsigned int m = nd->mMeshes[n];

			// Apply the material of this mesh
			apply_material(sc->mMaterials[this->object->mMeshes[m]->mMaterialIndex]);

			// Draw all faces assigned to this mesh
			for (unsigned int f = 0; f < this->object->mMeshes[m]->mNumFaces; f++) {
				GLenum face_mode;

				// Identifies the type of face
				switch (this->object->mMeshes[m]->mFaces[f].mNumIndices) {
					case 1:
						face_mode = GL_POINTS;
						break;
					case 2:
						face_mode = GL_LINES;
						break;
					case 3:
						face_mode = GL_TRIANGLES;
						break;
					default:
						face_mode = GL_POLYGON;
						break;
				}

				glBegin(face_mode); {
					for (unsigned int i = 0; i < this->object->mMeshes[m]->mFaces[f].mNumIndices; i++) {						// go through all vertices in face
						int v = this->object->mMeshes[m]->mFaces[f].mIndices[i];						// get group index for current index

						// Sets the vertex color
						if (this->object->mMeshes[m]->mColors[0] != NULL) {
							glColor4f(
								this->object->mMeshes[m]->mColors[0][v].r,
								this->object->mMeshes[m]->mColors[0][v].g,
								this->object->mMeshes[m]->mColors[0][v].b,
								this->object->mMeshes[m]->mColors[0][v].a
							);
						}

						// Maps the texture
						if (this->object->mMeshes[m]->mNormals != NULL) {

							// Sets the texture coordinates
							if (this->object->mMeshes[m]->HasTextureCoords(0))
								glTexCoord2f(this->object->mMeshes[m]->mTextureCoords[0][v].x, 1 - this->object->mMeshes[m]->mTextureCoords[0][v].y);

							// Sets the normal vector
							glNormal3fv(&this->object->mMeshes[m]->mNormals[v].x);
						}

						// Sets the vertex
						glVertex3fv(&this->object->mMeshes[m]->mVertices[v].x);
					}
				} glEnd();
			}
		}

		// Draw all childrens
		for (unsigned int n = 0; n < nd->mNumChildren; n++)
			this->recursive_render(sc, nd->mChildren[n]);

	} glPopMatrix();
}
