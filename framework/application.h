#ifndef H_APPLICATION
#define H_APPLICATION

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "common.h"
#include "camera.hpp"
#include "instance.h"

#define LOGICAL_DEVICE *(instance->pickedPhysicalDevice->get()->getLogicalDevice())
#define PHYSICAL_DEVICE instance->pickedPhysicalDevice->get()->getHandle()
#define GRAPHICS_QUEUE *(instance->pickedPhysicalDevice->get()->getGraphicsQueue()) 
#define PRESENT_QUEUE *(instance->pickedPhysicalDevice->get()->getPresentQueue()) 
#define COMPUTE_QUEUE *(instance->pickedPhysicalDevice->get()->getComputeQueue()) 
struct Vertex3D {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex3D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex3D, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex3D, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex3D& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
		//glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			// attributeDescriptions[2].binding = 0;
			// attributeDescriptions[2].location = 2;
			// attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			// attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color;
		}
	};
namespace std {
	template<> struct hash<Vertex3D> {
		size_t operator()(Vertex3D const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}


typedef struct MyBuffer
{
	VkBuffer		buffer;
	VkDeviceMemory		deviceMemory;
	VkDeviceSize		size;
} MyBuffer;

typedef struct MyImageBuffer
{
	VkImage		image;
	VkDeviceMemory		deviceMemory;
	VkDeviceSize		size;
public:
    MyImageBuffer(): size(0){}
} MyImage;

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

typedef enum e_PipelineType {
	PIPELINE_BASIC,
	// PIPELINE_TEXTURE,
	// PIPELINE_MODEL,
	// PIPELINE_COMPUTE,
	// PIPELINE_MIPMAP
}PipelineType;


class CApplication{
public:
    CApplication();
    ~CApplication();

    GLFWwindow * window;

    //VkInstance instance;//01
    std::unique_ptr<CInstance> instance{nullptr};

    VkSurfaceKHR surface;//03
    
    std::vector<Vertex3D> vertices3D;
    std::vector<uint32_t> indices3D;
    bool framebufferResized = false;

    CDebugger * debugger;

	MyBuffer vertexDataBuffer;  //05
	MyBuffer indexDataBuffer; //05
    std::vector<MyBuffer> uniformBuffers; //05 用来创建MVP的uniform
	std::vector<void*> uniformBuffersMapped; //05 用来更新MVP的uniform

    std::vector<VkCommandBuffer> commandBuffers;//06
    VkCommandPool commandPool;//06

    VkSwapchainKHR swapChain;//08
	std::vector<VkImage> swapChainImages;//08
	VkFormat swapChainImageFormat;//08
	VkExtent2D swapChainExtent;//08
	std::vector<VkImageView> swapChainImageViews;//08

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkRenderPass renderPass; //10

    std::vector<VkFramebuffer> swapChainFramebuffers;//11

    VkShaderModule vertShaderModule;//12
	VkShaderModule fragShaderModule;//12

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

    std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 1;
    std::chrono::_V2::system_clock::time_point lastTime;

    //Resource for Texture
    int32_t texWidth, texHeight;
    //std::string texturePath = "";
	VkSampler textureSampler;
	MyImageBuffer textureImageBuffer; 
	VkImageView textureImageView; 

    //Resource for Depth Test
    bool bEnableDepthTest;
	MyImageBuffer depthImageBuffer;
	VkImageView depthImageView;

    //Resource for MSAA
    bool bEnableMSAA;
    MyImageBuffer msaaColorImageBuffer;
	VkImageView msaaColorImageView;
    VkSampleCountFlagBits msaaSamples;

    void run();

    void prepareGLFW();
    void createGLFWSurface();
    void prepareVulkanDevices();

    int FindMemoryByFlagAndType(VkMemoryPropertyFlagBits memoryFlagBits, uint32_t  memoryTypeBits);
	int FindMemoryThatIsHostVisible(uint32_t memoryTypeBits);
    VkResult InitDataBufferHelper(VkDeviceSize size, VkBufferUsageFlags usage, OUT MyBuffer * pMyBuffer);
    VkResult FillDataBufferHelper(IN MyBuffer myBuffer, IN void * data);
    //void Init05CreateVertexBuffer();
    void Init05CreateIndexBuffer();
    void Init05CreateUniformBuffers(std::vector<MyBuffer> &_uniformBuffers, std::vector<void*> &_uniformBuffersMapped, VkBufferUsageFlags usage, VkDeviceSize bufferSize);

    void Init06CreateCommandPool();
    void Init06CreateCommandBuffers();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    void Init08CreateSwapChain();

    std::vector<char> readFile(const std::string& filename);
    void Init12SpirvShader(std::string filename, VkShaderModule * pShaderModule);

    void createSyncObjects();

    void mainLoop();
    
    static Camera mainCamera;
    static bool NeedToExit;
    static void GLFWKeyboard(GLFWwindow * window, int key, int scancode, int action, int mods);
	static void GLFWMouseMotion(GLFWwindow *window, double xpos, double ypos);
	static void GLFWMouseButton(GLFWwindow *window, int button, int action, int mods);


    /*Clean up Functions*/
    void cleanupSwapChain();
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    
    float durationTime = 0;
    float deltaTime = 0;
    UniformBufferObject ubo{};
    void updateUniformBuffer(uint32_t currentFrame, float durationTime);

    uint32_t imageIndex = 0;
    //void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();

    //texture utility functions
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, MyImageBuffer &imageBuffer);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    uint32_t mipLevels = 1; //1 means no mipmap
	bool bEnableMipMap = false;
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, std::array<MyImageBuffer, MIPMAP_TEXTURE_COUNT> *tmpTextureBufferForRainbowMipmaps, bool bEnableRainbowMipmaps);
 
    /*Virtual function: base and derived class will implement. If drived class not implement, call base's verson*/
    virtual void initialize();
    virtual void update();
    virtual void recordCommandBuffer();
    
    /*Pure virtual function(=0): base class not implment, derived class must implement*/


};

#endif