#include "../include/swapchain.h"

CSwapchain::CSwapchain(){
    //debugger = new CDebugger("../logs/swapchain.log");

    imageSize = 0; //0 means the size is not set, will query for the value
    bEnableMSAA = false;
    msaaSamples = VK_SAMPLE_COUNT_1_BIT;

#ifndef ANDROID
    logManager.setLogFile("swapChain.log");
#endif
}
CSwapchain::~CSwapchain(){
    //if (!debugger) delete debugger;
}

// void CSwapchain::GetPhysicalDevice(CPhysicalDevice *physical_device){
//     m_physical_device = physical_device;
// }

// bool CSwapchain::CheckFormatSupport(VkPhysicalDevice gpu, VkFormat format, VkFormatFeatureFlags requestedSupport) {///!!!!
//     VkFormatProperties vkFormatProperties;
//     vkGetPhysicalDeviceFormatProperties(gpu, format, &vkFormatProperties);
//     return (vkFormatProperties.optimalTilingFeatures & requestedSupport) == requestedSupport;
// }

void CSwapchain::createImages(VkSurfaceKHR surface, int width, int height){
    //vulkan draws on the vkImage(s)
    //SwapChain will set vkImage to present on the screen
    //Surface will tell the format of the vkImage
    //PresentQueue is a queue to present

    //Need preare surface and presentQueue first.
    //swapchain.GetPhysicalDevice(CContext::GetHandle().physicalDevice->get());

    VkResult result = VK_SUCCESS;

    //try to find all available swapChainSupport(format, colorSpace and presentMode)
    SwapChainSupportDetails swapChainSupport = CContext::GetHandle().physicalDevice->get()->querySwapChainSupport(surface);
    displaySwapchainInfo(swapChainSupport);

    //choose format and color space
    VkSurfaceFormatKHR surfaceFormat;// = chooseSwapSurfaceFormat(swapChainSupport.formats);
    for (const auto& availableFormat : swapChainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }
    logManager.print("Choose default swapchain format: format %4d, colorSpace %12d", surfaceFormat.format, surfaceFormat.colorSpace); 

	if(bComputeSwapChainImage){//added VK_IMAGE_USAGE_STORAGE_BIT for image storage
        //choose format (again) for requestedSupport
        VkFormatFeatureFlags requestedSupport = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
        VkFormatProperties vkFormatProperties;
        for (auto& swapchain_format : swapChainSupport.formats) {///!!!!
            vkGetPhysicalDeviceFormatProperties(CContext::GetHandle().GetPhysicalDevice(), swapchain_format.format, &vkFormatProperties);
            if ((vkFormatProperties.optimalTilingFeatures & requestedSupport) == requestedSupport){
                logManager.print("Choose swapchain format for storage image: format %4d, colorSpace %12d", swapchain_format.format, swapchain_format.colorSpace); 
                surfaceFormat.format = swapchain_format.format;///!!!!
                break;///!!!!
            }

            //if (CheckFormatSupport(CContext::GetHandle().GetPhysicalDevice(), format.format, requestedSupport)) {///!!!!
            //    surfaceFormat.format = format.format;///!!!!
            //    break;///!!!!
            //}
        }
    }

    //tests for textureCompute
    //surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;//50 //validation error
    //surfaceFormat.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;//64 no validation error, but looks wrong
    //surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;//44 okay
    //surfaceFormat.format = VK_FORMAT_R8G8B8A8_UINT;//41 doesnt work for swapchain
    //surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;//37 looks okay, but validation error
    //surfaceFormat.format = VK_FORMAT_R8G8B8A8_SRGB;//43 looks okay, but validation error
    //About BGR and RGB: olde graphics hardware (VGA) used to be BGR. Windows still expects BGR by default, and internally display hardware might still be BGR.
    //It is quite common that the display controller in your GPU works with BGRA, not RGBA. So you get your swapchain images in the format that can be displayed so there's no extra hidden conversion needed.

    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

    if(imageSize == 0) imageSize = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageSize > swapChainSupport.capabilities.maxImageCount) {
        imageSize = swapChainSupport.capabilities.maxImageCount;
    }
    logManager.print("swapChainSupport.capabilities.minImageCount = %d", (int)swapChainSupport.capabilities.minImageCount);
    logManager.print("swapChainSupport.capabilities.maxImageCount = %d", (int)swapChainSupport.capabilities.maxImageCount);
    logManager.print("imageSize = %d", (int)imageSize);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageSize;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 
    if(bComputeSwapChainImage)
        createInfo.imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT; //added VK_IMAGE_USAGE_STORAGE_BIT for image storage

    QueueFamilyIndices indices = CContext::GetHandle().physicalDevice->get()->findQueueFamilies(surface, "Find Queue Families when creating swapchain images");
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    //generate swapChain images (swapChainImages)
    //result = vkCreateSwapchainKHR(LOGICAL_DEVICE, &createInfo, nullptr, &swapChain);

    result = vkCreateSwapchainKHR(CContext::GetHandle().GetLogicalDevice(), &createInfo, nullptr, &handle);
    if (result != VK_SUCCESS) throw std::runtime_error("failed to create swap chain!");
    //REPORT("vkCreateSwapchainKHR");
    //logManager.print("wxjtest");
    result = vkGetSwapchainImagesKHR(CContext::GetHandle().GetLogicalDevice(), handle, &imageSize, nullptr);
    //REPORT("vkGetSwapchainImagesKHR(Get imageCount)");
    images.resize(imageSize);
    result = vkGetSwapchainImagesKHR(CContext::GetHandle().GetLogicalDevice(), handle, &imageSize, images.data());
    //REPORT("vkGetSwapchainImagesKHR");

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void CSwapchain::createImageViews(VkImageAspectFlags aspectFlags){
    // present views for the double-buffering:
    views.resize(imageSize);
    for (size_t i = 0; i < imageSize; i++) {
        CWxjImageBuffer dummyImageBuffer; //dummyImageBuffer doesn't really matter here, just use it's create function
		views[i] = dummyImageBuffer.createImageView(images[i], swapChainImageFormat, aspectFlags, 1);
    }
}

void CSwapchain::displaySwapchainInfo(SwapChainSupportDetails details){
    logManager.print("vkGetPhysicalDeviceSurfaceCapabilitiesKHR:");
    logManager.print("\tminImageCount = %d; maxImageCount = %d", (int)details.capabilities.minImageCount, (int)details.capabilities.maxImageCount);
    logManager.print("\tcurrentExtent = %d x %d", (int)details.capabilities.currentExtent.width, (int)details.capabilities.currentExtent.height);
    logManager.print("\tminImageExtent = %d x %d", (int)details.capabilities.minImageExtent.width, (int)details.capabilities.minImageExtent.height);
    logManager.print("\tmaxImageExtent = %d x %d", (int)details.capabilities.maxImageExtent.width, (int)details.capabilities.maxImageExtent.height);
    logManager.print("\tmaxImageArrayLayers = %d", (int)details.capabilities.maxImageArrayLayers);
    logManager.print("\tsupportedTransforms = 0x%04x", (int)details.capabilities.supportedTransforms);
    logManager.print("\tcurrentTransform = 0x%04x", (int)details.capabilities.currentTransform);
    logManager.print("\tsupportedCompositeAlpha = 0x%04x", (int)details.capabilities.supportedCompositeAlpha);
    logManager.print("\tsupportedUsageFlags = 0x%04x", (int)details.capabilities.supportedUsageFlags);
   
   
    logManager.print("\nFound %d Surface Formats:",  (int)details.formats.size());
    for (uint32_t i = 0; i < details.formats.size(); i++) {
        logManager.print("%3d: format %4d, colorSpace %12d", i, details.formats[i].format, details.formats[i].colorSpace); 
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)			    logManager.print("\tVK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT)		logManager.print("\tVK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)		logManager.print( "\tVK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_DCI_P3_LINEAR_EXT)			    logManager.print("\tVK_COLOR_SPACE_DCI_P3_LINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT)		    logManager.print("\tVK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_BT709_LINEAR_EXT)			    logManager.print( "\tVK_COLOR_SPACE_BT709_LINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_BT709_NONLINEAR_EXT)			logManager.print( "\tVK_COLOR_SPACE_BT709_NONLINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_BT2020_LINEAR_EXT)			    logManager.print("\tVK_COLOR_SPACE_BT2020_LINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)			    logManager.print("\tVK_COLOR_SPACE_HDR10_ST2084_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_DOLBYVISION_EXT)			    logManager.print( "\tVK_COLOR_SPACE_DOLBYVISION_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_HDR10_HLG_EXT)			        logManager.print("\tVK_COLOR_SPACE_HDR10_HLG_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT)			logManager.print("\tVK_COLOR_SPACE_ADOBERGB_LINEAR_EXT");
        if (details.formats[i].colorSpace == VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT)		    logManager.print("\tVK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT");
    }

    logManager.print("\nFound %d Present Modes:", (int)details.presentModes.size());
    for (uint32_t i = 0; i < details.presentModes.size(); i++) {
        logManager.print("%3d: presentModes %4d", i, details.presentModes[i]);
        if (details.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)			        logManager.print("\tVK_PRESENT_MODE_IMMEDIATE_KHR");
        if (details.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)			            logManager.print("\tVK_PRESENT_MODE_MAILBOX_KHR");
        if (details.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR)			            logManager.print("\tVK_PRESENT_MODE_FIFO_KHR");
        if (details.presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)		        logManager.print("\tVK_PRESENT_MODE_FIFO_RELAXED_KHR");
        if (details.presentModes[i] == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR)	    logManager.print("\tVK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR");
        if (details.presentModes[i] == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR)	logManager.print("\tVK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR");
    }

    logManager.print("");
}

// VkSurfaceFormatKHR CSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
//     for (const auto& availableFormat : availableFormats) {
//         if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//             return availableFormat;
//         }
//     }

//     return availableFormats[0];
// }

VkPresentModeKHR CSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D CSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void CSwapchain::createMSAAImages(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties){
    msaaColorImageBuffer.createImage(swapChainExtent.width,swapChainExtent.height, 1, msaaSamples, swapChainImageFormat, tiling, usage, properties);
}
void CSwapchain::createMSAAImageViews(VkImageAspectFlags aspectFlags){
    msaaColorImageBuffer.createImageView(swapChainImageFormat, aspectFlags, 1);
}
void CSwapchain::createDepthImages(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties){
    depthImageBuffer.createImage(swapChainExtent.width,swapChainExtent.height, 1, msaaSamples, format, tiling, usage, properties);
}
void CSwapchain::createDepthImageViews(VkFormat format, VkImageAspectFlags aspectFlags){
    depthImageBuffer.createImageView(format, aspectFlags, 1);
}

void CSwapchain::CreateFramebuffers(VkRenderPass &renderPass){
	//HERE_I_AM("CreateFramebuffers");

	VkResult result = VK_SUCCESS;

	swapChainFramebuffers.resize(views.size());

	for (size_t i = 0; i < imageSize; i++) {
		std::vector<VkImageView> imageViews_to_attach; 
		 if (bEnableDepthTest && bEnableMSAA) {//Renderpass attachment(render target) order: Color, Depth, ColorResolve
		    imageViews_to_attach.push_back(msaaColorImageBuffer.view);
		    imageViews_to_attach.push_back(depthImageBuffer.view);
			imageViews_to_attach.push_back(views[i]);
		 }else if(bEnableDepthTest){//Renderpass attachment(render target) order: Color, Depth
			imageViews_to_attach.push_back(views[i]);
			imageViews_to_attach.push_back(depthImageBuffer.view);
		}else{ //Renderpass attachment(render target) order: Color
			imageViews_to_attach.push_back(views[i]);
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews_to_attach.size());
		framebufferInfo.pAttachments = imageViews_to_attach.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		result = vkCreateFramebuffer(CContext::GetHandle().GetLogicalDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
		if (result != VK_SUCCESS) throw std::runtime_error("failed to create framebuffer!");
		//REPORT("vkCreateFrameBuffer");
	}	
}

void CSwapchain::GetMaxUsableSampleCount(){
	msaaSamples = CContext::GetHandle().physicalDevice->get()->getMaxUsableSampleCount();
	//msaaSamples = VK_SAMPLE_COUNT_1_BIT;
}

void CSwapchain::CleanUp(){
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(CContext::GetHandle().GetLogicalDevice(), framebuffer, nullptr);
    }

    for (auto imageView : views) {
        vkDestroyImageView(CContext::GetHandle().GetLogicalDevice(), imageView, nullptr);
    }

    vkDestroySwapchainKHR(CContext::GetHandle().GetLogicalDevice(), handle, nullptr);

    depthImageBuffer.destroy();
    msaaColorImageBuffer.destroy();
}