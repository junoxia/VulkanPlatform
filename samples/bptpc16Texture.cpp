#include "..\\framework\\include\\application.h"
#define TEST_CLASS_NAME CBptpc16Texture
class TEST_CLASS_NAME: public CApplication{
public:
	std::vector<Vertex3D> vertices3D = {
		{ { -1.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } ,{ 0.0f, 0.0f, 1.0f }},
		{ { 1.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } ,{ 0.0f, 0.0f, 1.0f }},
		{ { 1.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } ,{ 0.0f, 0.0f, 1.0f }},
		{ { -1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } ,{ 0.0f, 0.0f, 1.0f }}
	};
	std::vector<uint32_t> indices3D = { 0, 1, 2, 2, 3, 0};

	std::vector<std::pair<std::string, bool>> textureNames = {{"48bpt.png", true}}; //first: textureName, second: mipmap
	std::vector<int> textureList = {0};
	std::vector<int> modelList = {0}; 

    void initialize(){
		mainCamera.setPosition(glm::vec3(0.0f, 0.0f, -3.0f));
		mainCamera.setPerspective(90.0f, (float)windowWidth /  (float)windowHeight, 0.1f, 256.0f);

		appInfo.Object.Count = 1;
		appInfo.Object.Model.List = &modelList;
		appInfo.Object.Texture.Names = &textureNames;
		appInfo.Object.Texture.List = &textureList;
		appInfo.Shader.Vertex = "bptpc16Texture/vert.spv";
		appInfo.Shader.Fragment = "bptpc16Texture/frag.spv";
		appInfo.Uniform.GraphicsSampler.Enable = true;
		appInfo.Uniform.EnableGraphicsMVP = true;
		appInfo.Feature.EnableGraphicsMSAA = true;
		appInfo.Feature.EnableGraphics48BPT = true;
		appInfo.Buffer.GraphicsVertex.Enable = true;
		appInfo.Buffer.GraphicsVertex.Vertices3D = &vertices3D; 
		appInfo.Buffer.GraphicsVertex.Indices3D = &indices3D;

		//Activate_Uniform_Graphics_Sampler();
		//Activate_Uniform_Graphics_MVP();
		//Activate_Feature_Graphics_MSAA();
		//Activate_Feature_Graphics_48BPT();
		//Activate_Buffer_Graphics_Vertex(vertices3D, indices3D);
		//Activate_Texture(&textureNames);
		//Activate_Pipeline();

		//object.Register((CApplication*)this); //, vertices3D, indices3D

		CApplication::initialize();
	}

	void update(){
		CApplication::update();
	}

	void recordGraphicsCommandBuffer(){
		objectList[0].Draw();
	}

};

#ifndef ANDROID
#include "..\\windowsFramework\\include\\main.hpp"
#endif