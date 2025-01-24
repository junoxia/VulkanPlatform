#include "../include/renderProcess.h"

CRenderProcess::CRenderProcess(){
    //debugger = new CDebugger("../logs/renderProcess.log");
    
    bUseAttachmentColorPresent = false;
    bUseAttachmentDepth = false;
    bUseAttachmentColorMultisample = false;

	bUseColorBlendAttachment = false;

    m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
}
CRenderProcess::~CRenderProcess(){
	//if (!debugger) delete debugger;
}

void CRenderProcess::createSubpass(){ 
	uint32_t attachmentCount = 0;

	//clear values for each attachment. The array is indexed by attachment number
	//Only elements corresponding to cleared attachments are used. Other elements of pClearValues are ignored.
	//each attachment has its clearvalues

	//the subpass attachment ref must match the order in attachment description order
	//note that fragment shader will write color to buffer pointed by pColorAttachments
	//fragment shader output is not always single samplered, so if msaa is enabled, shader output can't be used to present; in this case, use pResolveAttachment for present

	if(bUseAttachmentColorPresent){
		clearValues.push_back({0.0f, 0.0f, 0.0f, 1.0f}); //color attachment: the background color is set to black (0,0,0,1)
		attachment_reference_color_present.attachment = attachmentCount++; 
		attachment_reference_color_present.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if(bUseAttachmentColorMultisample) subpass.pResolveAttachments = &attachment_reference_color_present;
		else subpass.pColorAttachments = &attachment_reference_color_present; //fragment shader output here
	}
	if(bUseAttachmentDepth){//added for model
		clearValues.push_back({1.0f, 0}); //The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane.
		attachment_reference_depth.attachment = attachmentCount++; 
		attachment_reference_depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		subpass.pDepthStencilAttachment = &attachment_reference_depth; 
	}
	if(bUseAttachmentColorMultisample){ //added for MSAA
		clearValues.push_back({0.0f, 0.0f, 0.0f, 1.0f}); 
		attachment_reference_color_multisample.attachment = attachmentCount++; 
		attachment_reference_color_multisample.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pColorAttachments = &attachment_reference_color_multisample; //fragment shader output here
	}

	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
}

void CRenderProcess::createDependency(VkPipelineStageFlags srcPipelineStageFlag, VkPipelineStageFlags dstPipelineStageFlag){  
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = srcPipelineStageFlag;
	dependency.srcAccessMask = 0;//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	dependency.dstStageMask = dstPipelineStageFlag;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	if (bUseAttachmentDepth) 
		dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
}

void CRenderProcess::createRenderPass(){ 
	VkResult result = VK_SUCCESS;

	//attachment descriptions in renderpass must match the same order as in framebuffer:
	//#1: a color attachment with sampler number = 1
	//#2: if depth test is enabled, a depth attachment with sampler number = n
	//#3: if msaa is enabled, a color attachment with sampler number = n
	std::vector<VkAttachmentDescription> attachmentDescriptions;
	if(bUseAttachmentColorPresent) attachmentDescriptions.push_back(attachment_description_color_present); 
	if(bUseAttachmentDepth) attachmentDescriptions.push_back(attachment_description_depth);
	if(bUseAttachmentColorMultisample) attachmentDescriptions.push_back(attachment_description_color_multisample);

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data(); //1
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;//2
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;//3

	result = vkCreateRenderPass(CContext::GetHandle().GetLogicalDevice(), &renderPassInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
	//REPORT("vkCreateRenderPass");
		 
}

void CRenderProcess::enableColorAttachmentDescriptionColorPresent(VkFormat swapChainImageFormat){  
	bUseAttachmentColorPresent = true;

	attachment_description_color_present.format = swapChainImageFormat;
	attachment_description_color_present.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_description_color_present.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description_color_present.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description_color_present.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description_color_present.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description_color_present.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_description_color_present.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

void CRenderProcess::enableAttachmentDescriptionDepth(VkFormat depthFormat, VkSampleCountFlagBits msaaSamples){  
	bUseAttachmentDepth = true;

	//added for model
	attachment_description_depth.format = depthFormat;//findDepthFormat();
	//std::cout<<"addDepthAttachment::depthAttachment.format = "<<depthAttachment.format<<std::endl;
	attachment_description_depth.samples = msaaSamples;
	std::cout<<"attachment_description_depth.samples = "<<attachment_description_depth.samples<<std::endl;
	attachment_description_depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description_depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description_depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description_depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description_depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_description_depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
}

void CRenderProcess::enableAttachmentDescriptionColorMultiSample(VkFormat swapChainImageFormat, bool bEnableDepthTest, bool bEnableMSAA, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples, VkImageLayout imageLayout){  
	//Concept of attachment in Vulkan is like render target in OpenGL
	//Subpass is a procedure to write/read attachments (a render process can has many subpasses, aka a render pass)
	//Subpass is designed for mobile TBDR architecture
	//At the beginning of subpass, attachment is loaded; at the end of attachment, attachment is stored
	bUseAttachmentColorMultisample = true;
	
	m_msaaSamples = msaaSamples;
    m_swapChainImageFormat = swapChainImageFormat;
	
	// if(bEnableDepthTest) {
	// 	enableAttachmentDescriptionDepth(depthFormat);
	// 	//std::cout<<"Depth Attachment added. Depth Test enabled. "<<std::endl;
	// }

	attachment_description_color_multisample.finalLayout = imageLayout;

	//if(msaaSamples > 1) {//msaaSamples > 1 means swapchains'MSAA feature is enabled
	// if(bEnableMSAA) {
	// 	imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// 	addColorAttachmentResolve();
	// 	//std::cout<<"Color Attachment Resolve added. MSAA enabled, msaaSamples = "<<msaaSamples<<std::endl;
	// }

	attachment_description_color_multisample.format = m_swapChainImageFormat;
	//std::cout<<"addColorAttachment::colorAttachment.format = "<<colorAttachment.format<<std::endl;
	attachment_description_color_multisample.samples = m_msaaSamples;
	std::cout<<"attachment_description_color_multisample.samples = "<<attachment_description_color_multisample.samples<<std::endl;
	attachment_description_color_multisample.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description_color_multisample.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description_color_multisample.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description_color_multisample.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description_color_multisample.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	

	//std::cout<<"Color Attachment added. "<<std::endl;
}


void CRenderProcess::addColorBlendAttachment(VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, 
											 VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor){
	bUseColorBlendAttachment = true;

	//Blend Algorithm
	//oldColor: the color already in framebuffer
	//newColor: the color output from fragment shader
	//if blendEnable:
	//  finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb)
	//  finalColor.a   = (srcAlphaBlendFactor * newColor.a  ) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a  )
	//else:
	//  finalColor = newColor

	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	colorBlendAttachment.blendEnable = VK_TRUE;

	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
}

void CRenderProcess::createComputePipelineLayout(VkDescriptorSetLayout &descriptorSetLayout){
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(CContext::GetHandle().GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) 
		throw std::runtime_error("failed to create compute pipeline layout!");
}
void CRenderProcess::createComputePipeline(VkShaderModule &computeShaderModule){
	bCreateComputePipeline = true;
	
	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = computePipelineLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(CContext::GetHandle().GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create compute pipeline!");
	}
}

void CRenderProcess::createGraphicsPipelineLayout(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts, int graphicsPipelineLayout_id){
	VkPushConstantRange dummyPushConstantRange;
	createGraphicsPipelineLayout(descriptorSetLayouts, dummyPushConstantRange, false, graphicsPipelineLayout_id);
}
void CRenderProcess::createGraphicsPipelineLayout(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts, VkPushConstantRange &pushConstantRange, bool bUsePushConstant, int graphicsPipelineLayout_id){
	VkResult result = VK_SUCCESS;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// if (pt == PIPELINE_COMPUTE) {
	// 	pipelineLayoutInfo.setLayoutCount = 0;
	// 	pipelineLayoutInfo.pSetLayouts = nullptr;
	// }else {
	//std::cout<<"DEBUG: create graphics pipeline layout: descriptorSetLayouts.size()="<<descriptorSetLayouts.size()<<std::endl;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();//  descriptorSetLayout;//todo: LAYOUT
	//}

	if(bUsePushConstant){
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	}

	//Create Graphics Pipeline Layout
	VkPipelineLayout newlayout;
	graphicsPipelineLayouts.push_back(newlayout);
	//std::cout<<"before vkCreatePipelineLayout()"<<std::endl;
	result = vkCreatePipelineLayout(CContext::GetHandle().GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &graphicsPipelineLayouts[graphicsPipelineLayout_id]);
	//std::cout<<"after vkCreatePipelineLayout()"<<std::endl;
	//result = vkCreatePipelineLayout(CContext::GetHandle().GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout);
	
	if (result != VK_SUCCESS) throw std::runtime_error("failed to create pipeline layout!");
	//REPORT("vkCreatePipelineLayout");
}

void CRenderProcess::Cleanup(){
	if(renderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(CContext::GetHandle().GetLogicalDevice(), renderPass, nullptr);

	if(bCreateGraphicsPipeline){
		for(int i = 0; i < graphicsPipelines.size(); i++){
			vkDestroyPipeline(CContext::GetHandle().GetLogicalDevice(), graphicsPipelines[i], nullptr);
			vkDestroyPipelineLayout(CContext::GetHandle().GetLogicalDevice(), graphicsPipelineLayouts[i], nullptr);
		}
	}

	if(bCreateComputePipeline){
		vkDestroyPipeline(CContext::GetHandle().GetLogicalDevice(), computePipeline, nullptr);
    	vkDestroyPipelineLayout(CContext::GetHandle().GetLogicalDevice(), computePipelineLayout, nullptr);
	}
	
}