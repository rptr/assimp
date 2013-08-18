
/** @file MD2Exporter.h
 * Declares the MD2 exporter class
 */
#ifndef AI_MD2EXPORTER_H_INC
#define AI_MD2EXPORTER_H_INC

#include <sstream>

struct aiScene;
struct aiNode;

namespace Assimp
{

// ----------------------------------------------------------------------------
/** Helper class to export a given scene to an MD2 file. */
// ----------------------------------------------------------------------------
class MD2Exporter
{
public:
	/// Constructor for a specific scene to export
	MD2Exporter(const char* filename, const aiScene* pScene);

public:

	std::string GetMaterialLibName();
	std::string GetMaterialLibFileName();
	
public:

	/// public stringstreams to write all output into
	std::ostringstream mOutput, mOutputMat;

private:

	// intermediate data structures
	struct FaceVertex 
	{
		FaceVertex()
			: vp(),vn(),vt() 
		{
		}

		// one-based, 0 means: 'does not exist'
		unsigned int vp,vn,vt;
	};

	struct Face {
		char kind;
		std::vector<FaceVertex> indices;
	};

	struct MeshInstance {

		std::string name, matname;
		std::vector<Face> faces;
	};

	void WriteHeader(std::ostringstream& out);

	void WriteMaterialFile();
	void WriteGeometryFile();

	std::string GetMaterialName(unsigned int index);

	void AddMesh(const aiString& name, const aiMesh* m, const aiMatrix4x4& mat);
	void AddNode(const aiNode* nd, const aiMatrix4x4& mParent);

    // Binary write
    void WriteInt32(uint32_t i);

private:

	const std::string filename;
	const aiScene* const pScene;

	std::vector<aiVector3D> vp, vn, vt;
	std::vector<MeshInstance> meshes;

	// this endl() doesn't flush() the stream
	const std::string endl;
};

}

#endif
