#include "..\\framework\\include\\application.h"
#define TEST_CLASS_NAME CSimpleObjLoader
class TEST_CLASS_NAME: public CApplication{
public:
	std::vector<Vertex3D> vertices3D;
	std::vector<uint32_t> indices3D;

	std::vector<VkClearValue> clearValues{ {  1.0f, 1.0f, 1.0f, 1.0f  },  { 1.0f, 0 } };

    void initialize(){
		swapchain.bEnableDepthTest = true; 
		
		modelManager.LoadObjModel("viking_room.obj", vertices3D, indices3D);

		renderer.CreateVertexBuffer<Vertex3D>(vertices3D);
		renderer.CreateIndexBuffer(indices3D);
		renderer.CreateCommandPool(surface);
		renderer.CreateCommandBuffers();

		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;	
		textureImage.CreateTextureImage("viking_room.png", usage, renderer.commandPool);
		textureImage.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		VkFormat depthFormat = renderProcess.findDepthFormat();
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		swapchain.createDepthImages(depthFormat, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		swapchain.createDepthImageViews(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		renderProcess.addColorAttachment(swapchain.swapChainImageFormat); //add this function will enable color attachment (bUseColorAttachment = true)
		renderProcess.addDepthAttachment(); //add this function will enable depth attachment(bUseDepthAttachment = true)
		renderProcess.createSubpass();
		VkPipelineStageFlags srcPipelineStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		VkPipelineStageFlags dstPipelineStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		renderProcess.createDependency(srcPipelineStageFlag, dstPipelineStageFlag);
		renderProcess.createRenderPass();

		swapchain.CreateFramebuffers(renderProcess.renderPass);

		shaderManager.CreateVertexShader("model/vert.spv");
		shaderManager.CreateFragmentShader("model/frag.spv");

		descriptor.addMVPUniformBuffer();
		descriptor.addImageSamplerUniformBuffer(textureImage.mipLevels);
		descriptor.createDescriptorPool();
		descriptor.createDescriptorSetLayout();
		descriptor.createDescriptorSets(&textureImage.textureImageBuffer.view);

		renderProcess.createLayout(descriptor.descriptorSetLayout);
		renderProcess.createGraphicsPipeline<Vertex3D>(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
			shaderManager.vertShaderModule, 
			shaderManager.fragShaderModule);

		CApplication::initialize();
	}

	void update(){
		descriptor.mvpUBO.model = glm::rotate(glm::mat4(1.0f), durationTime * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		CApplication::update();
	}

	void recordCommandBuffer(){
		RENDER_START

		renderer.BindVertexBuffer();
		renderer.BindIndexBuffer();
		renderer.DrawIndexed(indices3D);

		RENDER_END
	}
};

#ifndef ANDROID
#include "..\\windowsFramework\\include\\main.hpp"
#endif