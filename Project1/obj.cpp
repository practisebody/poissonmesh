#include "stdafx.h"
#include "obj.h"
#include "mtl.h"
#include "HalfEdge.h"

void ReadFace(const char* line, map<pair<int, int>, HEEdge*>& edgePairs,
			  vector<HEVert*>& rawVertices, vector<HEVert*>& _Vertices, vector<Point*>& _TextPoints,
			  vector<Vector*>& _NormVectors, HEFace* _face, Material* material)
{
	HEVert* tempVert;
	HEEdge* tempEdge;
	GLint vFrom, vTo, vFirst;
	GLint pText, pLastText;
	GLint pNorm, pLastNorm;
	HEEdge* lastEdge = NULL;
	HEEdge* firstEdge;
	map<pair<int, int>, HEEdge*>::iterator mapIt;
	_face->m_mtl = material;
	sscanf(line, "%d%*c%d%*c%d%[^\n]", &vFrom, &pLastText, &pLastNorm, line);
	int count = 1;
	vFirst = vFrom;
	while (sscanf(line, "%d%*c%d%*c%d%[^\n]", &vTo, &pText, &pNorm, line) != EOF)
	{
		++count;
		tempEdge = new HEEdge();
		tempVert = new HEVert(*rawVertices[vFrom]);
		tempVert->m_realVert = vFrom;
		_Vertices.push_back(tempVert);
		tempVert->m_edge = tempEdge;
		tempEdge->m_vert = tempVert;
		tempEdge->m_face = _face;
		tempEdge->m_text = _TextPoints[pLastText];
		tempEdge->m_norm = _NormVectors[pLastNorm];
		if ((mapIt = edgePairs.find(pair<int, int>(vTo, vFrom))) != edgePairs.end())
		{
			tempEdge->m_pair = mapIt->second;
			mapIt->second->m_pair = tempEdge;
			edgePairs.erase(mapIt);
		}
		else
		{
			bool flag = edgePairs.insert(pair<pair<int,int>, HEEdge*>(pair<int,int>(vFrom, vTo), tempEdge)).second;
			MYASSERT(flag);
		}
		if (lastEdge != NULL)
		{
			lastEdge->m_next = tempEdge;
		}
		else
		{
			firstEdge = tempEdge;
		}
		vFrom = vTo;
		lastEdge = tempEdge;
		pLastText = pText;
		pLastNorm = pNorm;
	}
	tempEdge = new HEEdge();
	lastEdge->m_next = tempEdge;
	tempEdge->m_face = _face;
	tempEdge->m_text = _TextPoints[pLastText];
	tempEdge->m_norm = _NormVectors[pLastNorm];
	if ((mapIt = edgePairs.find(pair<int, int>(vFirst, vTo))) != edgePairs.end())
	{
		tempEdge->m_pair = mapIt->second;
		mapIt->second->m_pair = tempEdge;
		edgePairs.erase(mapIt);
	}
	else
	{
		bool flag = edgePairs.insert(pair<pair<int,int>, HEEdge*>(pair<int,int>(vTo, vFirst), tempEdge)).second
		MYASSERT(flag);
	}
	tempVert = new HEVert(*rawVertices[vTo]);
	tempVert->m_realVert = vTo;
	_Vertices.push_back(tempVert);
	tempVert->m_edge = tempEdge;
	tempEdge->m_vert = tempVert;
	tempEdge->m_next = firstEdge;
	_face->m_edge = tempEdge;
}

void ReadOBJ(const char* filename, vector<HEVert*>& rawVertices,
		vector<HEVert*>& _Vertices, vector<Point*>& _TextPoints, vector<Vector*>& _NormVectors,
		vector<HEFace*>& _Faces, map<string, Material*>& _Materials)
{
	FILE* fd = fopen(filename, "r");
	char* line = new char[Parameters::MAX_LINE_LENGTH];
	char* prefix = new char[Parameters::MAX_PREFIX_LENGTH];
	char* strName = new char[Parameters::MAX_LINE_LENGTH];
	char* word = new char[Parameters::MAX_LINE_LENGTH];
	HEVert* tempVert;
	HEFace* tempFace;
	Point* tempPoint;
	Vector* tempVector;
	Material* material = NULL;
	map<pair<int, int>, HEEdge*> edgePairs;
	map<pair<int, int>, HEEdge*>::iterator mapIt;

	// push to start from 1
	rawVertices.push_back(NULL);
	_Vertices.push_back(NULL);
	_TextPoints.push_back(NULL);
	_NormVectors.push_back(NULL);
	while (fgets(line, Parameters::MAX_LINE_LENGTH, fd) != NULL)
	{
		strcat(line, " ");
		sscanf(line, "%s %[^\n]", prefix, line);
		if (strcmp(prefix, "mtllib") == 0)
		{
			sscanf(line, "%s", strName);
			ReadMTL(strName, _Materials);
		}
		else if (strcmp(prefix, "v") == 0)
		{
			GLfloat x, y, z;
			sscanf(line, "%f%f%f", &x, &y, &z);
			tempVert = new HEVert(x, y, z);
			Utility::SetMax(Parameters::m_vMax, tempVert->m_vert);
			Utility::SetMin(Parameters::m_vMin, tempVert->m_vert);
			rawVertices.push_back(tempVert);
		}
		else if (strcmp(prefix, "vn") == 0)
		{
			GLfloat x, y, z;
			sscanf(line, "%f%f%f", &x, &y, &z);
			tempVector = new Vector(x, y, z);
			_NormVectors.push_back(tempVector);
		}
		else if (strcmp(prefix, "vt") == 0)
		{
			GLfloat x, y, z;
			sscanf(line, "%f%f%f", &x, &y, &z);
			tempPoint = new Point(x, y, z);
			_TextPoints.push_back(tempPoint);
		}
		else if (strcmp(prefix, "f") == 0)
		{
			tempFace = new HEFace();
			ReadFace(line, edgePairs, rawVertices, _Vertices, _TextPoints, _NormVectors, tempFace, material);
			_Faces.push_back(tempFace);
		}
		else if (strcmp(prefix, "usemtl") == 0)
		{
			sscanf(line, "%s", strName);
			material = _Materials.find(strName)->second;
		}
	}
	MYASSERT(edgePairs.empty());
	// Fix Single Edge
	/*while (edgePairs.empty() == false)
	{
		GLint vFirst, vSecond;
		mapIt = edgePairs.begin();
		vFirst = mapIt->first.first;
		vSecond = mapIt->first.second;
		if ((++mapIt)->first.first == vSecond)
		{
			sprintf(line, "%d/0/0 %d/0/0 ", vSecond, vFirst);
			do
			{
				sprintf(word, "%d/0/0 ", mapIt->first.second);
				strcat(word, line);
				strcpy(line, word);
			}
			while ((++mapIt)->first.second != vFirst);
		}
		else if (mapIt->first.second == vFirst)
		{
			sprintf(line, "%d/0/0 %d/0/0 ", vSecond, vFirst);
			do
			{
				sprintf(word, "%d/0/0 ", mapIt->first.first);
				strcat(line, word);
			}
			while ((++mapIt)->first.first != vSecond);
		}
		else
		{
			MYASSERT(false);
		}
		tempFace = new HENullFace();
		ReadFace(line, edgePairs, rawVertices, _Vertices, _TextPoints, _NormVectors, tempFace, NULL);
		_Faces.push_back(tempFace);
	}*/
	printf("%d\n", edgePairs.size());
	delete [] line;
	delete [] prefix;
	delete [] strName;
	delete [] word;
	fclose(fd);

	// set parameters
	Parameters::nMaxDrawNumber = INT_MAX;
	Parameters::fMaxDrawSize = (Parameters::m_vMax - Parameters::m_vMin).Module() / 8;
}