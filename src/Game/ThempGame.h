#pragma once
#include <vector>
namespace Themp
{
	class Actor;
	class Tile;
	class Object3D;
	class Camera;
	//class Resources;
	//struct Texture;
	class Game
	{
	public:
		Game() {};
		void Start();
		void Update(double dt);
		void Stop();

		std::vector<Themp::Object3D*> m_Objects3D;
		Camera* m_Camera;
		char m_Keys[1024];//Keeping track of input;
		POINT m_CursorPos;
		float m_CursorDeltaX, m_CursorDeltaY;
	};
};
