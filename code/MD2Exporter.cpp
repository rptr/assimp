#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_MD2_EXPORTER

#include "MD2Exporter.h"
#include "../include/assimp/version.h"
#include "MD2FileData.h"

using namespace Assimp;
namespace Assimp	{

// ----------------------------------------------------------------------
// Worker function for exporting a scene . Prototyped and registered in 
// Exporter.cpp
void ExportSceneMD2(const char* pFile, 
                    IOSystem* pIOSystem,
                    const aiScene* pScene)
{
	// invoke the exporter 
	MD2Exporter exporter(pFile, pScene);

	// we're still here - export successfully completed. Write both the main 
	// OBJ file and the material script
	{
		boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wtb"));
		outfile->Write (exporter.mOutput.str().c_str(), 
		                static_cast<size_t>(exporter.mOutput.tellp()),
		                1);
	}
	{
		boost::scoped_ptr<IOStream> outfile 
		            (
		            pIOSystem->Open(exporter.GetMaterialLibFileName(),"wt")
		            );
		outfile->Write (exporter.mOutputMat.str().c_str(), 
		                static_cast<size_t>(exporter.mOutputMat.tellp()),
		                1);
	}
}

} // end of namespace Assimp


// ----------------------------------------------------------------------------
MD2Exporter :: MD2Exporter (const char* _filename, const aiScene* pScene)
: filename(_filename)
, pScene(pScene)
, endl("\n")
{
	// make sure that all formatting happens using the standard, C locale and 
	// not the user's current locale
	const std::locale& l = std::locale("C");
	mOutput.imbue(l);
	mOutputMat.imbue(l);

	WriteGeometryFile();
	WriteMaterialFile();
}

// ----------------------------------------------------------------------------
std::string MD2Exporter :: GetMaterialLibName()
{	
	// within the Obj file, we use just the relative file name with the path 
	// stripped
	const std::string& s = GetMaterialLibFileName();
	std::string::size_type il = s.find_last_of("/\\");
	if (il != std::string::npos) {
		return s.substr(il + 1);
	}

	return s;
}

// ----------------------------------------------------------------------------
std::string MD2Exporter :: GetMaterialLibFileName()
{	
	return filename + ".mtl";
}

// ----------------------------------------------------------------------------
void MD2Exporter :: WriteHeader(std::ostringstream& out)
{
//	out << "# File produced by Open Asset Import Library (http://www.assimp.sf.net)" << endl;
//	out << "# (assimp v" << aiGetVersionMajor() << '.' << aiGetVersionMinor() << '.' << aiGetVersionRevision() << ")" << endl  << endl;

    // Only write the first mesh
    aiMesh *mesh = pScene->mMeshes[0];

    MD2::Header head;
    head.magic              = 844121161;
    head.version            = 8;
    head.skinWidth          = 1;
    head.skinHeight         = 1;
    head.frameSize          = 1;
    head.numSkins           = 1;
    head.numVertices        = mesh->mNumVertices;
    head.numTexCoords       = mesh->mNumVertices;
    head.numTriangles       = mesh->mNumFaces;
    head.numGlCommands      = 0;
    head.numFrames          = 844121161;
    
    // 68 = size of header
    uint32_t offset = 68;
    head.offsetSkins        = offset;
    offset += head.numSkins * 64;
    
    head.offsetTexCoords    = offset;
    offset += head.numTexCoords * 4;
    
    head.offsetTriangles    = offset;
    offset += head.numTriangles * 12;

    head.offsetGlCommands   = offset;
    offset += 0;
    
    head.offsetFrames       = offset;
    offset += head.numFrames * 30 + head.numVertices * 4;
    
    head.offsetEnd          = offset;
    
    printf("export offsets: %u %u %u %u %u %u\n", 
           head.offsetSkins,
           head.offsetTexCoords,
           head.offsetTriangles,
           head.offsetGlCommands,
           head.offsetFrames,
           head.offsetEnd);

    WriteInt32 (head.magic);
    WriteInt32 (head.version);
    WriteInt32 (head.skinWidth);
    WriteInt32 (head.skinHeight);
    WriteInt32 (head.frameSize);
    WriteInt32 (head.numSkins);
    WriteInt32 (head.numVertices);
    WriteInt32 (head.numTexCoords);
    WriteInt32 (head.numTriangles);
    WriteInt32 (head.numGlCommands);
    WriteInt32 (head.numFrames);
    WriteInt32 (head.offsetSkins);
    WriteInt32 (head.offsetTexCoords);
    WriteInt32 (head.offsetTriangles);
    WriteInt32 (head.offsetFrames);
    WriteInt32 (head.offsetGlCommands);
    WriteInt32 (head.offsetEnd);

}

// ------------------------------------------------------------------------------------------------
std::string MD2Exporter :: GetMaterialName(unsigned int index)
{
	const aiMaterial* const mat = pScene->mMaterials[index];
	aiString s;
	if(AI_SUCCESS == mat->Get(AI_MATKEY_NAME,s)) {
		return std::string(s.data,s.length);
	}

	char number[ sizeof(unsigned int) * 3 + 1 ];
	ASSIMP_itoa10(number,index);
	return "$Material_" + std::string(number);
}

// ------------------------------------------------------------------------------------------------
void MD2Exporter :: WriteMaterialFile()
{
	WriteHeader(mOutputMat);

	for(unsigned int i = 0; i < pScene->mNumMaterials; ++i) {
		const aiMaterial* const mat = pScene->mMaterials[i];

		int illum = 1;
		mOutputMat << "newmtl " << GetMaterialName(i)  << endl;

		aiColor4D c;
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE,c)) {
			mOutputMat << "kd " << c.r << " " << c.g << " " << c.b << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_AMBIENT,c)) {
			mOutputMat << "ka " << c.r << " " << c.g << " " << c.b << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR,c)) {
			mOutputMat << "ks " << c.r << " " << c.g << " " << c.b << endl;
		}

		float o;
		if(AI_SUCCESS == mat->Get(AI_MATKEY_OPACITY,o)) {
			mOutputMat << "d " << o << endl;
		}

		if(AI_SUCCESS == mat->Get(AI_MATKEY_SHININESS,o) && o) {
			mOutputMat << "Ns " << o << endl;
			illum = 2;
		}

		mOutputMat << "illum " << illum << endl;

		aiString s;
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0),s)) {
			mOutputMat << "map_kd " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_AMBIENT(0),s)) {
			mOutputMat << "map_ka " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_SPECULAR(0),s)) {
			mOutputMat << "map_ks " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_SHININESS(0),s)) {
			mOutputMat << "map_ns " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_HEIGHT(0),s) || AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_NORMALS(0),s)) {
			// implementations seem to vary here, so write both variants
			mOutputMat << "bump " << s.data << endl;
			mOutputMat << "map_bump " << s.data << endl;
		}

		mOutputMat << endl;
	}
}

// ------------------------------------------------------------------------------------------------
void MD2Exporter :: WriteGeometryFile()
{
	WriteHeader(mOutput);
	mOutput << "mtllib "  << GetMaterialLibName() << endl << endl;

	// collect mesh geometry
	aiMatrix4x4 mBase;
	AddNode(pScene->mRootNode,mBase);

	// write vertex positions
	mOutput << "# " << vp.size() << " vertex positions" << endl;
	BOOST_FOREACH(const aiVector3D& v, vp) {
		mOutput << "v  " << v.x << " " << v.y << " " << v.z << endl;
	}
	mOutput << endl;

	// write uv coordinates
	mOutput << "# " << vt.size() << " UV coordinates" << endl;
	BOOST_FOREACH(const aiVector3D& v, vt) {
		mOutput << "vt " << v.x << " " << v.y << " " << v.z << endl;
	}
	mOutput << endl;

	// write vertex normals
	mOutput << "# " << vn.size() << " vertex normals" << endl;
	BOOST_FOREACH(const aiVector3D& v, vn) {
		mOutput << "vn " << v.x << " " << v.y << " " << v.z << endl;
	}
	mOutput << endl;

	// now write all mesh instances
	BOOST_FOREACH(const MeshInstance& m, meshes) {
		mOutput << "# Mesh \'" << m.name << "\' with " << m.faces.size() << " faces" << endl;
		mOutput << "g " << m.name << endl;
		mOutput << "usemtl " << m.matname << endl;

		BOOST_FOREACH(const Face& f, m.faces) {
			mOutput << f.kind << ' ';
			BOOST_FOREACH(const FaceVertex& fv, f.indices) {
				mOutput << ' ' << fv.vp;

				if (f.kind != 'p') {
					if (fv.vt || f.kind == 'f') {
						mOutput << '/';
					}
					if (fv.vt) {
						mOutput << fv.vt;
					}
					if (f.kind == 'f') {
						mOutput << '/';
						if (fv.vn) {
							mOutput << fv.vn;
						}
					}
				}
			}

			mOutput << endl;
		}
		mOutput << endl;
	}
}

// ------------------------------------------------------------------------------------------------
void MD2Exporter :: AddMesh(const aiString& name, const aiMesh* m, const aiMatrix4x4& mat)
{
	meshes.push_back(MeshInstance());
	MeshInstance& mesh = meshes.back();

	mesh.name = std::string(name.data,name.length) + (m->mName.length ? "_"+std::string(m->mName.data,m->mName.length) : "");
	mesh.matname = GetMaterialName(m->mMaterialIndex);

	mesh.faces.resize(m->mNumFaces);
	for(unsigned int i = 0; i < m->mNumFaces; ++i) {
		const aiFace& f = m->mFaces[i];

		Face& face = mesh.faces[i];
		switch (f.mNumIndices) {
			case 1: 
				face.kind = 'p';
				break;
			case 2: 
				face.kind = 'l';
				break;
			default: 
				face.kind = 'f';
		}
		face.indices.resize(f.mNumIndices);

		for(unsigned int a = 0; a < f.mNumIndices; ++a) {
			const unsigned int idx = f.mIndices[a];

			// XXX need a way to check if this is an unique vertex or if we had it already, 
			// in which case we should instead reference the previous occurrence.
			ai_assert(m->mVertices);
			vp.push_back( mat * m->mVertices[idx] );
			face.indices[a].vp = vp.size();

			if (m->mNormals) {
				vn.push_back( m->mNormals[idx] );
			}
			face.indices[a].vn = vn.size();

			if (m->mTextureCoords[0]) {
				vt.push_back( m->mTextureCoords[0][idx] );
			}
			face.indices[a].vt = vt.size();
		}
	}
}

// ------------------------------------------------------------------------------------------------
void MD2Exporter :: AddNode(const aiNode* nd, const aiMatrix4x4& mParent)
{
	const aiMatrix4x4& mAbs = mParent * nd->mTransformation;

	for(unsigned int i = 0; i < nd->mNumMeshes; ++i) {
		AddMesh(nd->mName, pScene->mMeshes[nd->mMeshes[i]],mAbs);
	}

	for(unsigned int i = 0; i < nd->mNumChildren; ++i) {
		AddNode(nd->mChildren[i],mAbs);
	}
}

void MD2Exporter :: WriteInt32 (uint32_t i)
{
    // MD2 files are little-endian
#ifdef AI_BUILD_BIG_ENDIAN
    ByteSwap::Swap4 (&i);
#endif

    char buf[4] = {(char) i & 0xff,
                    (char) (i >> 8) & 0xff,
                    (char) (i >> 16) & 0xff,
                    (char) (i >> 24) & 0xff};

    mOutput.write (buf, 4);
}


#endif
#endif
