#include "..\\framework\\include\\application.h"
#define TEST_CLASS_NAME CSimplePhongLighting
#include "object.h"
class TEST_CLASS_NAME: public CApplication{
public:
	struct CustomUniformBufferObject {
		glm::vec3 lightPos;

		static VkDescriptorSetLayoutBinding GetBinding(){
			VkDescriptorSetLayoutBinding binding;
			binding.binding = 0;//not important, will be reset
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.pImmutableSamplers = nullptr;
			binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			return binding;
		}
	};
	CustomUniformBufferObject customUBO{};

	CObject object;

    void initialize(){
		mainCamera.setPosition(glm::vec3(0.0f, -2.5f, -2.5f));
    	mainCamera.setRotation(glm::vec3(45.0f, 0.0f, 0.0f));
    	mainCamera.setPerspective(60.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 256.0f);
		
		swapchain.bEnableMSAA = true;//!To enable MSAA, make sure to enable depth test 
		swapchain.bEnableDepthTest = true; 

		

		renderer.CreateCommandPool(surface);
		renderer.CreateGraphicsCommandBuffer();

		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		textureManager.CreateTextureImage("viking_room.png", usage, renderer.commandPool);

		if(swapchain.bEnableMSAA){
			swapchain.GetMaxUsableSampleCount();
			usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchain.createMSAAImages(VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			swapchain.createMSAAImageViews(VK_IMAGE_ASPECT_COLOR_BIT);
		}

		VkFormat depthFormat = renderProcess.findDepthFormat();
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		swapchain.createDepthImages(depthFormat, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		swapchain.createDepthImageViews(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		if(swapchain.bEnableMSAA) imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderProcess.addColorAttachment(swapchain.swapChainImageFormat, swapchain.msaaSamples, imageLayout); //add this function will enable color attachment (bUseColorAttachment = true)
		renderProcess.addDepthAttachment(); //add this function will enable depth attachment(bUseDepthAttachment = true)
		if(swapchain.bEnableMSAA) renderProcess.addColorAttachmentResolve(); //add this function will enable color resolve attachment (bUseColorAttachmentResolve = true)
		renderProcess.createSubpass();
		VkPipelineStageFlags srcPipelineStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		VkPipelineStageFlags dstPipelineStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		renderProcess.createDependency(srcPipelineStageFlag, dstPipelineStageFlag);
		renderProcess.createRenderPass();

		swapchain.CreateFramebuffers(renderProcess.renderPass);
  
		shaderManager.CreateShader("simplePhoneLighting/vert.spv", shaderManager.VERT);
		shaderManager.CreateShader("simplePhoneLighting/frag.spv", shaderManager.FRAG); 
	

		CGraphicsDescriptorManager::addMVPUniformBuffer();
		uint32_t mipLevels = textureManager.textureImages[0].mipLevels;
		CGraphicsDescriptorManager::addImageSamplerUniformBuffer(mipLevels);
		CGraphicsDescriptorManager::addCustomUniformBuffer(sizeof(CustomUniformBufferObject));

		CDescriptorManager::createDescriptorPool();

		VkDescriptorSetLayoutBinding customBinding = CustomUniformBufferObject::GetBinding();
		CGraphicsDescriptorManager::createDescriptorSetLayout(&customBinding);
		CGraphicsDescriptorManager::createTextureDescriptorSetLayout();

		graphicsDescriptorManager.createDescriptorSets();
		object.Register((CApplication*)this, "viking_room.obj");

		//support multiple descriptors in one piplines: bind multiple descriptor layouts in one pipeline
		std::vector<VkDescriptorSetLayout> dsLayouts;
		dsLayouts.push_back(CGraphicsDescriptorManager::descriptorSetLayout);
		dsLayouts.push_back(CGraphicsDescriptorManager::textureDescriptorSetLayout); //set = 1

		renderProcess.createGraphicsPipelineLayout(dsLayouts);
		renderProcess.createGraphicsPipeline<Vertex3D>(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
			shaderManager.vertShaderModule,  
			shaderManager.fragShaderModule);

		CApplication::initialize();
	} 

	void update(){
		customUBO.lightPos = glm::vec3(0.75f * sin(durationTime * 3), 0.75f * sin(durationTime * 2), 0.75f * sin(durationTime * 1));
		graphicsDescriptorManager.updateCustomUniformBuffer<CustomUniformBufferObject>(renderer.currentFrame, durationTime, customUBO);

		//CGraphicsDescriptorManager::mvpUBO.mvpData[0].model = glm::rotate(glm::mat4(1.0f),  glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		object.SetRotation(glm::rotate(glm::mat4(1.0f),  glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		CApplication::update();
	}

	void recordGraphicsCommandBuffer(){
		object.Draw();
	}		
};

#ifndef ANDROID
#include "..\\windowsFramework\\include\\main.hpp"
#endif