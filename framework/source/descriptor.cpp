#include "../include/descriptor.h"

//Camera CApplication::mainCamera;

CDescriptor::CDescriptor(){
    //debugger = new CDebugger("../logs/descriptor.log");
	//bUseCustomUniformBuffer = false;
    //bUseMVP = false;
    //bUseVP = false;
    //bUseSampler = false;
    //bUseStorageBuffer = false;
	m_customUniformBufferSize = 0;
    m_storageBufferSize = 0;
    //m_storageBufferSize_2 = 0;
	mvpUBO.model = glm::mat4(1.0f);
    //textureSampler = NULL;
    textureSamplers.resize(1);
    textureSamplerCount = 0;

    m_storageImageSize = 0;

    uniformBufferUsageFlags = 0;
}

CDescriptor::~CDescriptor(){
    //if (!debugger) delete debugger;
}

void CDescriptor::addCustomUniformBuffer(VkDeviceSize customUniformBufferSize){
	//bUseCustomUniformBuffer = true;
    uniformBufferUsageFlags |= UNIFORM_BUFFER_CUSTOM_BIT;

	customUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	customUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	m_customUniformBufferSize = customUniformBufferSize;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkResult result = customUniformBuffers[i].init(m_customUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		vkMapMemory(CContext::GetHandle().GetLogicalDevice(), customUniformBuffers[i].deviceMemory, 0, m_customUniformBufferSize, 0, &customUniformBuffersMapped[i]);
	}
}

void CDescriptor::addMVPUniformBuffer(){
    //bUseMVP = true;
    uniformBufferUsageFlags |= UNIFORM_BUFFER_MVP_BIT;
	
    mvpUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    mvpUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result = mvpUniformBuffers[i].init(sizeof(MVPUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        //REPORT("InitDataBufferHelper");

        vkMapMemory(CContext::GetHandle().GetLogicalDevice(), mvpUniformBuffers[i].deviceMemory, 0, sizeof(MVPUniformBufferObject), 0, &mvpUniformBuffersMapped[i]);
    }
}

void CDescriptor::addVPUniformBuffer(){
    //bUseVP = true;
    uniformBufferUsageFlags |= UNIFORM_BUFFER_VP_BIT;
	
    vpUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    vpUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result = vpUniformBuffers[i].init(sizeof(VPUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        //REPORT("InitDataBufferHelper");

        vkMapMemory(CContext::GetHandle().GetLogicalDevice(), vpUniformBuffers[i].deviceMemory, 0, sizeof(VPUniformBufferObject), 0, &vpUniformBuffersMapped[i]);
    }
}

void CDescriptor::addImageSamplerUniformBuffer(uint32_t mipLevels){
    //bUseSampler = true;
    uniformBufferUsageFlags |= UNIFORM_BUFFER_SAMPLER_BIT;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(CContext::GetHandle().GetPhysicalDevice(), &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	
	if (mipLevels > 1) {
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;// VK_SAMPLER_MIPMAP_MODE_NEAREST;
	 	samplerInfo.minLod = 0.0f;
	 	samplerInfo.maxLod = static_cast<float>(mipLevels / 2);
	 	samplerInfo.mipLodBias = 0.0f;
	}

	VkResult result = vkCreateSampler(CContext::GetHandle().GetLogicalDevice(), &samplerInfo, nullptr, &textureSamplers[textureSamplerCount++]);
	//REPORT("vkCreateSampler");
}

void CDescriptor::addStorageBuffer(VkDeviceSize storageBufferSize, VkBufferUsageFlags usage){
    //bUseStorageBuffer = true;

    //if(!(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_1_BIT)){
        uniformBufferUsageFlags |= UNIFORM_BUFFER_STORAGE_BIT;

        storageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        storageBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        m_storageBufferSize = storageBufferSize;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            //VkResult result = InitDataBufferHelper(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &shaderStorageBuffers_compute[i]);// Create a staging buffer used to upload data to the gpu
            //FillDataBufferHelper(shaderStorageBuffers_compute[i], (void *)(particles.data()));// Copy initial particle data to all storage buffers
            //shaderStorageBuffers_compute[i].init(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            storageBuffers[i].init(storageBufferSize, usage);
            vkMapMemory(CContext::GetHandle().GetLogicalDevice(), storageBuffers[i].deviceMemory, 0, storageBufferSize, 0, &storageBuffersMapped[i]);
        }
    //}
    //else if(!(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_2_BIT)){
    //     uniformBufferUsageFlags |= UNIFORM_BUFFER_STORAGE_2_BIT;

    //     storageBuffers_2.resize(MAX_FRAMES_IN_FLIGHT);
    //     storageBuffersMapped_2.resize(MAX_FRAMES_IN_FLIGHT);

    //     m_storageBufferSize_2 = storageBufferSize;

    //     for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //         storageBuffers_2[i].init(storageBufferSize, usage); 
    //         vkMapMemory(CContext::GetHandle().GetLogicalDevice(), storageBuffers_2[i].deviceMemory, 0, storageBufferSize, 0, &storageBuffersMapped_2[i]);
    //     }
    // }
}

void CDescriptor::addStorageImage(VkDeviceSize storageImageSize, VkBufferUsageFlags usage){
        uniformBufferUsageFlags |= UNIFORM_IMAGE_STORAGE_BIT;

        storageImages.resize(MAX_FRAMES_IN_FLIGHT);
        storageImagesMapped.resize(MAX_FRAMES_IN_FLIGHT);

        m_storageImageSize = storageImageSize;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            storageImages[i].init(storageImageSize, usage);
            vkMapMemory(CContext::GetHandle().GetLogicalDevice(), storageImages[i].deviceMemory, 0, storageImageSize, 0, &storageImagesMapped[i]);
        }
}
void CDescriptor::updateStorageImage(){

}



void CDescriptor::createDescriptorPool(){//VkDescriptorType type
    //Descriptor Step 1/3
	//HERE_I_AM("CreateDescriptorPool");

	poolSizes.resize(getDescriptorSize());
	int counter = 0;

	if(uniformBufferUsageFlags & UNIFORM_BUFFER_CUSTOM_BIT){
        poolSizes[counter].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	 	poolSizes[counter].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		counter++;
	}
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_MVP_BIT || uniformBufferUsageFlags & UNIFORM_BUFFER_VP_BIT){
        poolSizes[counter].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	 	poolSizes[counter].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		counter++;
    }
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_SAMPLER_BIT){
        for(int i = 0; i < textureSamplers.size(); i++){
            poolSizes[counter].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	 	    poolSizes[counter].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		    counter++;
        }
    }
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_BIT){
        poolSizes[counter].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	    poolSizes[counter].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); 
        counter++;
    //}
    //if(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_2_BIT){
        poolSizes[counter].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	    poolSizes[counter].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); 
        counter++;
    }
    if(uniformBufferUsageFlags & UNIFORM_IMAGE_STORAGE_BIT){
        poolSizes[counter].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    poolSizes[counter].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); 
        counter++;
    }

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkResult result = vkCreateDescriptorPool(CContext::GetHandle().GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool);
	if (result != VK_SUCCESS) throw std::runtime_error("failed to create descriptor pool!");
	//REPORT("vkCreateDescriptorPool");
}

void CDescriptor::createDescriptorSetLayout(VkDescriptorSetLayoutBinding *customBinding){
    //Descriptor Step 2/3
    //VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount, const VkSampler*  pImmutableSamplers){
	//HERE_I_AM("CreateDescriptorSetLayout");

    bindings.resize(getDescriptorSize());
	int counter = 0;

	if(uniformBufferUsageFlags & UNIFORM_BUFFER_CUSTOM_BIT){
        bindings[counter].binding = counter;
		bindings[counter].descriptorCount = customBinding->descriptorCount;
		bindings[counter].descriptorType = customBinding->descriptorType;
		bindings[counter].pImmutableSamplers = customBinding->pImmutableSamplers;
		bindings[counter].stageFlags = customBinding->stageFlags;
		counter++;
	}
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_MVP_BIT){
        VkDescriptorSetLayoutBinding binding = MVPUniformBufferObject::GetBinding();
        bindings[counter].binding = counter;
		bindings[counter].descriptorCount = binding.descriptorCount;
		bindings[counter].descriptorType = binding.descriptorType;
		bindings[counter].pImmutableSamplers = binding.pImmutableSamplers;
		bindings[counter].stageFlags = binding.stageFlags;
		counter++;
    }
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_VP_BIT){
        VkDescriptorSetLayoutBinding binding = VPUniformBufferObject::GetBinding();
        bindings[counter].binding = counter;
		bindings[counter].descriptorCount = binding.descriptorCount;
		bindings[counter].descriptorType = binding.descriptorType;
		bindings[counter].pImmutableSamplers = binding.pImmutableSamplers;
		bindings[counter].stageFlags = binding.stageFlags;
		counter++;
    }
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_SAMPLER_BIT){
        for(int i = 0; i < textureSamplers.size(); i++){
            bindings[counter].binding = counter;
            bindings[counter].descriptorCount = 1;
            bindings[counter].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[counter].pImmutableSamplers = nullptr;
            bindings[counter].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            counter++;
        }
    }
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_BIT){
        bindings[counter].binding = counter;
        bindings[counter].descriptorCount = 1;
        bindings[counter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[counter].pImmutableSamplers = nullptr;
        bindings[counter].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        counter++;
    //}
    //if(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_2_BIT){
        bindings[counter].binding = counter;
        bindings[counter].descriptorCount = 1;
        bindings[counter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[counter].pImmutableSamplers = nullptr;
        bindings[counter].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        counter++;
    }
    if(uniformBufferUsageFlags & UNIFORM_IMAGE_STORAGE_BIT){
        bindings[counter].binding = counter;
        bindings[counter].descriptorCount = 1;
        bindings[counter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[counter].pImmutableSamplers = nullptr;
        bindings[counter].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        counter++;
    }

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkResult result = vkCreateDescriptorSetLayout(CContext::GetHandle().GetLogicalDevice(), &layoutInfo, nullptr, OUT &descriptorSetLayout);
	if (result != VK_SUCCESS) throw std::runtime_error("failed to create descriptor set layout!");
	//REPORT("vkCreateDescriptorSetLayout");
}

void CDescriptor::createDescriptorSets(std::vector<CTextureImage> *textureImages){
    //Descriptor Step 3/3
    //HERE_I_AM("wxjCreateDescriptorSets");

    int descriptorSize = getDescriptorSize();

    VkResult result = VK_SUCCESS;

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    //Step 3
    result = vkAllocateDescriptorSets(CContext::GetHandle().GetLogicalDevice(), &allocInfo, descriptorSets.data());
    if (result != VK_SUCCESS) throw std::runtime_error("failed to allocate descriptor sets!");
    //REPORT("vkAllocateDescriptorSets");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        //VkDescriptorBufferInfo storageBufferInfoLastFrame{}; //for compute shader
        //VkDescriptorBufferInfo storageBufferInfoCurrentFrame{}; //for compute shader

        descriptorWrites.resize(descriptorSize);
        int counter = 0;

        VkDescriptorBufferInfo customBufferInfo{}; //for custom uniform
        if(uniformBufferUsageFlags & UNIFORM_BUFFER_CUSTOM_BIT){
            customBufferInfo.buffer = customUniformBuffers[i].buffer;
            customBufferInfo.offset = 0;
            customBufferInfo.range = m_customUniformBufferSize;
            descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[counter].dstSet = descriptorSets[i];
            descriptorWrites[counter].dstBinding = counter;
            descriptorWrites[counter].dstArrayElement = 0;
            descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[counter].descriptorCount = 1;
            descriptorWrites[counter].pBufferInfo = &customBufferInfo;
            counter++;
        }
        VkDescriptorBufferInfo mvpBufferInfo{}; //for mvp
        if(uniformBufferUsageFlags & UNIFORM_BUFFER_MVP_BIT){
            mvpBufferInfo.buffer = mvpUniformBuffers[i].buffer;
            mvpBufferInfo.offset = 0;
            mvpBufferInfo.range = sizeof(MVPUniformBufferObject);
            descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[counter].dstSet = descriptorSets[i];
            descriptorWrites[counter].dstBinding = counter;
            descriptorWrites[counter].dstArrayElement = 0;
            descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[counter].descriptorCount = 1;
            descriptorWrites[counter].pBufferInfo = &mvpBufferInfo;
            counter++;
        }
        VkDescriptorBufferInfo vpBufferInfo{}; //for vp
        if(uniformBufferUsageFlags & UNIFORM_BUFFER_VP_BIT){
            vpBufferInfo.buffer = vpUniformBuffers[i].buffer;
            vpBufferInfo.offset = 0;
            vpBufferInfo.range = sizeof(VPUniformBufferObject);
            descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[counter].dstSet = descriptorSets[i];
            descriptorWrites[counter].dstBinding = counter;
            descriptorWrites[counter].dstArrayElement = 0;
            descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[counter].descriptorCount = 1;
            descriptorWrites[counter].pBufferInfo = &vpBufferInfo;
            counter++;
        }
        std::vector<VkDescriptorImageInfo> imageInfo{}; //for texture sampler
        if(uniformBufferUsageFlags & UNIFORM_BUFFER_SAMPLER_BIT){
            imageInfo.resize(textureSamplers.size());
            for(int j = 0; j < textureSamplers.size(); j++){
                imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //test compute storage image:  VK_IMAGE_LAYOUT_GENERAL
                imageInfo[j].imageView = (*textureImages)[j].textureImageBuffer.view;
                imageInfo[j].sampler = textureSamplers[j];
                descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[counter].dstSet = descriptorSets[i];
                descriptorWrites[counter].dstBinding = counter;
                descriptorWrites[counter].dstArrayElement = 0;
                descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[counter].descriptorCount = 1;
                descriptorWrites[counter].pImageInfo = &imageInfo[j];
                counter++;
            }
        }
        
        if(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_BIT){ //for storage buffer 1
            VkDescriptorBufferInfo storageBufferInfo_1{};
            storageBufferInfo_1.buffer = storageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT].buffer; //storage buffer of last frame in flight as compute shader input
            storageBufferInfo_1.offset = 0;
            storageBufferInfo_1.range = m_storageBufferSize;//sizeof(uint32_t) * 4;//sizeof(Particle) * PARTICLE_COUNT;

            descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[counter].dstSet = descriptorSets[i];
            descriptorWrites[counter].dstBinding = counter;
            descriptorWrites[counter].dstArrayElement = 0;
            descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[counter].descriptorCount = 1;
            descriptorWrites[counter].pBufferInfo = &storageBufferInfo_1;
            counter++;
        //}
            VkDescriptorBufferInfo storageBufferInfo_2{};
        //if(uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_2_BIT){ //for storage buffer 2
            storageBufferInfo_2.buffer = storageBuffers[i].buffer; //storage buffer of the current frame in flight as compute shader output
            storageBufferInfo_2.offset = 0;
            storageBufferInfo_2.range = m_storageBufferSize;

            descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[counter].dstSet = descriptorSets[i];
            descriptorWrites[counter].dstBinding = counter;
            descriptorWrites[counter].dstArrayElement = 0;
            descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[counter].descriptorCount = 1;
            descriptorWrites[counter].pBufferInfo = &storageBufferInfo_2;
            counter++;
        }
        if(uniformBufferUsageFlags & UNIFORM_IMAGE_STORAGE_BIT){
            //VkDescriptorBufferInfo storageBufferInfo_1{};
            //storageBufferInfo_1.buffer = storageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT].buffer; //storage buffer of last frame in flight as compute shader input
            //storageBufferInfo_1.offset = 0;
            //storageBufferInfo_1.range = m_storageBufferSize;//sizeof(uint32_t) * 4;//sizeof(Particle) * PARTICLE_COUNT;
            VkDescriptorImageInfo storageImageInfo{
               // VK_NULL_HANDLE, // sampler
                //(*textureImages)[0].textureImageBuffer.view,
                //VK_IMAGE_LAYOUT_GENERAL
            };
            storageImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            storageImageInfo.imageView = (*textureImages)[0].textureImageBuffer.view;
            storageImageInfo.sampler = VK_NULL_HANDLE; //textureSamplers[0];

            descriptorWrites[counter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[counter].dstSet = descriptorSets[i];
            descriptorWrites[counter].dstBinding = counter;
            descriptorWrites[counter].dstArrayElement = 0;
            descriptorWrites[counter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorWrites[counter].descriptorCount = 1;
            descriptorWrites[counter].pImageInfo = &storageImageInfo;
            counter++;
        }

        /*switch (pt) {
        // case PIPELINE_COMPUTE:
        //     descriptorWrites.resize(3);

        //     //VkDescriptorBufferInfo uniformBufferInfo{};
        //     bufferInfo.buffer = uniformBuffers_compute[i].buffer;
        //     bufferInfo.offset = 0;
        //     bufferInfo.range = sizeof(UniformBufferObject_compute);

        //     //std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        //     descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //     descriptorWrites[0].dstSet = descriptorSets[i];
        //     descriptorWrites[0].dstBinding = 0;
        //     descriptorWrites[0].dstArrayElement = 0;
        //     descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        //     descriptorWrites[0].descriptorCount = 1;
        //     descriptorWrites[0].pBufferInfo = &bufferInfo;

        //     //VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        //     storageBufferInfoLastFrame.buffer = shaderStorageBuffers_compute[(i - 1) % MAX_FRAMES_IN_FLIGHT].buffer;
        //     storageBufferInfoLastFrame.offset = 0;
        //     storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

        //     descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //     descriptorWrites[1].dstSet = descriptorSets[i];
        //     descriptorWrites[1].dstBinding = 1;
        //     descriptorWrites[1].dstArrayElement = 0;
        //     descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        //     descriptorWrites[1].descriptorCount = 1;
        //     descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        //     //VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        //     storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers_compute[i].buffer;
        //     storageBufferInfoCurrentFrame.offset = 0;
        //     storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

        //     descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //     descriptorWrites[2].dstSet = descriptorSets[i];
        //     descriptorWrites[2].dstBinding = 2;
        //     descriptorWrites[2].dstArrayElement = 0;
        //     descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        //     descriptorWrites[2].descriptorCount = 1;
        //     descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;
        //     break;
        }*/

        //Step 4
        vkUpdateDescriptorSets(CContext::GetHandle().GetLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

    }
}


void CDescriptor::updateMVPUniformBuffer(uint32_t currentFrame, float durationTime, Camera &mainCamera) {
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_MVP_BIT){
        // UniformBufferObject ubo{};
        // ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // glm::mat4x4 m = glm::perspective(glm::radians(45.0f), WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 10.0f);
        // m[1][1] *= -1;
        // ubo.proj = m;
        // ubo.model = glm::rotate(glm::mat4(1.0f), durationTime * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        //UniformBufferObject ubo{};
        mvpUBO.view = mainCamera.matrices.view;
        mvpUBO.proj = mainCamera.matrices.perspective;

        memcpy(mvpUniformBuffersMapped[currentFrame], &mvpUBO, sizeof(mvpUBO));
    }
}

void CDescriptor::updateVPUniformBuffer(uint32_t currentFrame, float durationTime, Camera &mainCamera) {
    if(uniformBufferUsageFlags & UNIFORM_BUFFER_VP_BIT){
        //UniformBufferObject ubo{};
        vpUBO.view = mainCamera.matrices.view;
        vpUBO.proj = mainCamera.matrices.perspective;

        memcpy(vpUniformBuffersMapped[currentFrame], &vpUBO, sizeof(vpUBO));
    }
}

int CDescriptor::getDescriptorSize(){
	int descriptorSize = 0;
	descriptorSize += uniformBufferUsageFlags & UNIFORM_BUFFER_CUSTOM_BIT ? 1:0;
	descriptorSize += (uniformBufferUsageFlags & UNIFORM_BUFFER_MVP_BIT || uniformBufferUsageFlags & UNIFORM_BUFFER_VP_BIT) ? 1:0;
	descriptorSize += uniformBufferUsageFlags & UNIFORM_BUFFER_SAMPLER_BIT ? textureSamplers.size():0;
    descriptorSize += uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_BIT ? 2:0;
    //descriptorSize += uniformBufferUsageFlags & UNIFORM_BUFFER_STORAGE_2_BIT ? 1:0;
    descriptorSize += uniformBufferUsageFlags & UNIFORM_IMAGE_STORAGE_BIT ? 1:0;
	return descriptorSize;
}


void CDescriptor::DestroyAndFree(){
    for(int i = 0; i < textureSamplers.size(); i++){
    //if(textureSampler) 
        vkDestroySampler(CContext::GetHandle().GetLogicalDevice(), textureSamplers[i], nullptr);
    }
    for (size_t i = 0; i < customUniformBuffers.size(); i++) {
        customUniformBuffers[i].DestroyAndFree();
    }

    for (size_t i = 0; i < mvpUniformBuffers.size(); i++) {
        mvpUniformBuffers[i].DestroyAndFree();
    }

    for (size_t i = 0; i < vpUniformBuffers.size(); i++) {
        vpUniformBuffers[i].DestroyAndFree();
    }

    for (size_t i = 0; i < storageBuffers.size(); i++) {
        storageBuffers[i].DestroyAndFree();
    }

    //for (size_t i = 0; i < storageBuffers_2.size(); i++) {
    //    storageBuffers_2[i].DestroyAndFree();
    //}

    //no need to destroy descriptorSets, because they are from descriptorPool
    vkDestroyDescriptorPool(CContext::GetHandle().GetLogicalDevice(), descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(CContext::GetHandle().GetLogicalDevice(), descriptorSetLayout, nullptr);
}

