#include "Obj.h"
#include "AndyOS.h"
#include "stdio.h"

using namespace std;

Obj::Obj(char* file)
{
	ReadFile(file);
	CreateVertices();
}

void Obj::ReadFile(char* file)
{
	return;
	/*String text = file;
	List<String> lines;
	text.Split(lines, '\n');

	debug_print("--START\n");

	for (int i = 0; i < lines.Count(); i++)
	{
		List<String> args;
		lines[i].Split(args, ' ');

		if (args[0] == "v")
		{
			float x = atof(args[1].ToChar());
			float y = atof(args[2].ToChar());
			float z = atof(args[3].ToChar());
			positions.Add(Vector3(x, y, z));
		}
		else if (args[0] == "vt")
		{
			float u = atof(args[1].ToChar());
			float v = atof(args[2].ToChar());
			uvs.Add(Vector3(u, v, 0));
		}
		else if (args[0] == "vn")
		{
			float x = atof(args[1].ToChar());
			float y = atof(args[2].ToChar());
			float z = atof(args[3].ToChar());
			normals.Add(Vector3(x, y, z));
		}
		else if (args[0] == "f")
		{
			//if (args.Count() == 5)
			//	Exceptions::ThrowException("Quad exception", "Obj file");

			Face face;
			for (int i = 0; i < 3; i++)
			{
				List<String> indexes;

				args[i + 1].Split(indexes, '/');
				face.positions[i] = atoi(indexes[0].ToChar());
				face.normals[i] = atoi(indexes[2].ToChar());

				if (indexes[1] == "")
					face.uvs[i] = 0;
				else
					face.uvs[i] = atoi(indexes[1].ToChar());

			}
			faces.Add(face);
		}
	}

	debug_print("--END\n");*/
}

void Obj::CreateVertices()
{
	/*int vertex_count = faces.Count() * 3;

	for (int i = 0; i < faces.Count(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			Vertex vert;

			Vector3 pos = positions[faces[i].positions[j] - 1];
			vert.pos.x = pos.x;
			vert.pos.y = pos.y;
			vert.pos.z = pos.z;

			Vector3 norm = normals[faces[i].normals[j] - 1];
			vert.normal = norm.ToVector4();

			Vector3 uv = uvs[faces[i].uvs[j] - 1];
			vert.tex_u = uv.x;
			vert.tex_v = uv.y;

			vert.color = Color(1, 1, 1);

			vertices[i * 3 + j] = vert;
		}
	}*/
}
