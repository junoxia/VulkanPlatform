#include "../include/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdParty/stb_image.h"

CTextureImage::CTextureImage(){
#ifndef ANDROID
    imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
#else
	imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
#endif
	//debugger = new CDebugger("../logs/texture.log");
}
CTextureImage::~CTextureImage(){
	//if (!debugger) delete debugger;
}


void CTextureImage::CreateTextureImage(const std::string texturePath, VkImageUsageFlags usage, VkCommandPool &commandPool) {
	pCommandPool = &commandPool;
	//Step 1: prepare staging buffer with texture pixels inside
	int texChannels;
#ifndef ANDROID
	std::string fullTexturePath = TEXTURE_PATH + texturePath;
	uint8_t* pixels = stbi_load(fullTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
#else
	std::vector<uint8_t> fileBits;
	std::string fullTexturePath = ANDROID_TEXTURE_PATH + texturePath;
	CContext::GetHandle().androidManager.AssetReadFile(fullTexturePath.c_str(), fileBits);
	uint8_t* pixels = stbi_load_from_memory(fileBits.data(), fileBits.size(), &texWidth, &texHeight, &texChannels, 4);//stbi_uc
#endif
	CreateTextureImage(pixels, usage, textureImageBuffer);
}

void CTextureImage::CreateTextureImage(uint8_t* pixels, VkImageUsageFlags usage, CWxjImageBuffer &imageBuffer) {
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	mipLevels = bEnableMipMap ? (static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1) : 1;

	if (!pixels) throw std::runtime_error("failed to load texture image!");

	CWxjBuffer stagingBuffer;
	VkResult result = stagingBuffer.init(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer.fill(pixels);

	stbi_image_free(pixels);

	//Step 2: create(allocate) image buffer
	imageBuffer.createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, imageFormat, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//Step 3: copy stagingBuffer(pixels) to imageBuffer(empty)
	//To perform the copy, need change imageBuffer's layout: undefined->transferDST->shader-read-only 
	//If the image is mipmap enabled, keep the transferDST layout (it will be mipmaped anyway)
	//(transfer image in non-DST layout is not optimal???)
	if(mipLevels == 1){
		transitionImageLayout(imageBuffer.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
 		copyBufferToImage(stagingBuffer.buffer, imageBuffer.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
      	transitionImageLayout(imageBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}else{
		transitionImageLayout(imageBuffer.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer.buffer, imageBuffer.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	}

	stagingBuffer.DestroyAndFree();
}

void CTextureImage::CreateImageView(VkImageAspectFlags aspectFlags){
    textureImageBuffer.createImageView(imageFormat, aspectFlags, mipLevels);
}

void CTextureImage::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void CTextureImage::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer CTextureImage::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = *pCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(CContext::GetHandle().GetLogicalDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void CTextureImage::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(CContext::GetHandle().GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(CContext::GetHandle().GetGraphicsQueue());

    vkFreeCommandBuffers(CContext::GetHandle().GetLogicalDevice(), *pCommandPool, 1, &commandBuffer);
}

void CTextureImage::generateMipmaps(){
    if(mipLevels <= 1) return;
    generateMipmaps(textureImageBuffer.image);
}

void CTextureImage::generateMipmaps(VkImage image, bool bMix, std::array<CWxjImageBuffer, MIPMAP_TEXTURE_COUNT> *textureImageBuffers_mipmaps) {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(CContext::GetHandle().GetPhysicalDevice(), imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		if (bMix) {
            //TODO: (Validation Error)
            //command buffer VkCommandBuffer 0x62f45f8[] expects VkImage 0xab64de0000000020[] 
            //(subresource: aspectMask 0x1 array layer 0, mip level 3) to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL--instead, 
            //current layout is VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			int j = i > MIPMAP_TEXTURE_COUNT ? MIPMAP_TEXTURE_COUNT : i;
			vkCmdBlitImage(commandBuffer,
				(*textureImageBuffers_mipmaps)[j-1].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);
		}else {
			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        //barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void CTextureImage::generateMipmaps(std::string rainbowCheckerboardTexturePath, VkImageUsageFlags usage){ //rainbow mipmaps case
    if(mipLevels <= 1) return;

	std::array<CWxjImageBuffer, MIPMAP_TEXTURE_COUNT> tmpTextureBufferForRainbowMipmaps;//create temp mipmaps
	for (int i = 0; i < MIPMAP_TEXTURE_COUNT; i++) {//fill temp mipmaps
		int texChannels;
#ifndef ANDROID
		std::string fullTexturePath = TEXTURE_PATH + rainbowCheckerboardTexturePath + std::to_string(i) + ".png";
		uint8_t* pixels = stbi_load(fullTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
#else
		std::vector<uint8_t> fileBits;
		std::string fullTexturePath = ANDROID_TEXTURE_PATH + rainbowCheckerboardTexturePath + std::to_string(i) + ".png";
		CContext::GetHandle().androidManager.AssetReadFile(fullTexturePath.c_str(), fileBits);
		uint8_t* pixels = stbi_load_from_memory(fileBits.data(), fileBits.size(), &texWidth, &texHeight, &texChannels, 4);
#endif
		CreateTextureImage(pixels, usage, tmpTextureBufferForRainbowMipmaps[i]);
        generateMipmaps(tmpTextureBufferForRainbowMipmaps[i].image);
	}
	//Generate mipmaps for image, using tmpTextureBufferForRainbowMipmaps
	generateMipmaps(textureImageBuffer.image, true, &tmpTextureBufferForRainbowMipmaps);
	//Clean up
	for (int i = 0; i < MIPMAP_TEXTURE_COUNT; i++) {
        tmpTextureBufferForRainbowMipmaps[i].destroy();
	}
}

void CTextureImage::Destroy(){
    textureImageBuffer.destroy();
}

