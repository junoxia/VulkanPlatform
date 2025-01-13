#include "../include/application.h"

//static class members must be defined outside. 
//otherwise invoke 'undefined reference' error when linking
Camera CApplication::mainCamera;
bool CApplication::NeedToExit = false; 
bool CApplication::NeedToPause = false;
std::vector<CObject> CApplication::objectList; 
std::vector<CLight> CApplication::lightList; 

CApplication::CApplication(){
    //debugger = new CDebugger("../logs/application.log");

    //NeedToExit = false;
    windowWidth = 0;
    windowHeight = 0;
}

#ifndef ANDROID
void CApplication::run(){ //Entrance Function
    CContext::Init();

    /**************** 
    * Five steps with third-party(GLFW or SDL) initialization
    * Step 1: Create Window
    *****************/
    m_sampleName.erase(0, 1);
#ifdef SDL
    sdlManager.createWindow(OUT windowWidth, OUT windowHeight, m_sampleName);
#else
    glfwManager.createWindow(OUT windowWidth, OUT windowHeight, m_sampleName);
#endif
	PRINT("run: Created Window. Window width = %d,  height = %d.", windowWidth, windowHeight);

    /**************** 
    * Step 2: Select required layers
    *****************/
    const std::vector<const char*> requiredValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    
    /**************** 
    * Step 3: Select required instance extensions
    *****************/
    std::vector<const char*> requiredInstanceExtensions;
#ifdef SDL
    sdlManager.queryRequiredInstanceExtensions(OUT requiredInstanceExtensions);
#else    
    glfwManager.queryRequiredInstanceExtensions(OUT requiredInstanceExtensions);
#endif
    if(enableValidationLayers) requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    /**************** 
    * Step 4: create instance
    *****************/
    //prepareVulkanDevices();
    instance = std::make_unique<CInstance>(requiredValidationLayers, requiredInstanceExtensions);

    /**************** 
    * Step 5: create surface
    * Surface is to store view format information for creating swapchain. 
    * Only third party(glfw or sdl) knows what kind of surface can be attached to its window.
    *****************/
#ifdef SDL   
    sdlManager.createSurface(IN instance, OUT surface);
#else  
    glfwManager.createSurface(IN instance, OUT surface);
#endif

    /**************** 
    * General initialization begins
    * Select required queue families
    * Select required device extensions
    *****************/
    VkQueueFlagBits requiredQueueFamilies = VK_QUEUE_GRAPHICS_BIT; //& VK_QUEUE_COMPUTE_BIT
    const std::vector<const char*>  requireDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    instance->findAllPhysicalDevices();

    CContext::GetHandle().physicalDevice = instance->pickSuitablePhysicalDevice(surface, requireDeviceExtensions, requiredQueueFamilies);
    //App dev can only query properties from physical device, but can not directly operate it
    //App dev operates logical device, can logical device communicate with physical device by command queues
    //App dev will fill command buffer with commands later
    //instance->pickedPhysicalDevice->get()->createLogicalDevices(surface, requiredValidationLayers, requireDeviceExtensions);
    CContext::GetHandle().physicalDevice->get()->createLogicalDevices(surface, requiredValidationLayers, requireDeviceExtensions);

    //query  basic capabilities of surface
    //VkSurfaceCapabilitiesKHR*                   pSurfaceCapabilities;
    //std::cout<<vkGetPhysicalDeviceSurfaceCapabilitiesKHR(CContext::GetHandle().GetPhysicalDevice(), surface, pSurfaceCapabilities)<<std::endl;
    //std::cout<<"Surface min extent: width="<<pSurfaceCapabilities->minImageExtent.width<<", Surface min extent: height="<<pSurfaceCapabilities->minImageExtent.height<<std::endl;
    //std::cout<<"Surface max extent: width="<<pSurfaceCapabilities->maxImageExtent.width<<", Surface max extent: height="<<pSurfaceCapabilities->maxImageExtent.height<<std::endl;

    swapchain.createImages(surface, windowWidth, windowHeight);
	swapchain.createImageViews(VK_IMAGE_ASPECT_COLOR_BIT);

    CSupervisor::Register((CApplication*)this);

    std::cout<<"======================================="<<std::endl;
    std::cout<<"======Welcome to Vulkan Platform======="<<std::endl;
    std::cout<<"======================================="<<std::endl;

    auto startInitialzeTime = std::chrono::high_resolution_clock::now();
    initialize();
    auto endInitializeTime = std::chrono::high_resolution_clock::now();
    auto durationInitializationTime = std::chrono::duration<float, std::chrono::seconds::period>(endInitializeTime - startInitialzeTime).count() * 1000;
    std::cout<<"Total Initialization cost: "<<durationInitializationTime<<" milliseconds"<<std::endl;

#ifdef SDL   
    while(sdlManager.bStillRunning) {
        sdlManager.eventHandle();
        if(!NeedToPause) UpdateRecordRender();
        if(NeedToExit) break;
    }
#else  
    while (!glfwWindowShouldClose(glfwManager.window)) {
        glfwPollEvents();
        if(!NeedToPause) UpdateRecordRender();
        if(NeedToExit) break;
	}
#endif

	vkDeviceWaitIdle(CContext::GetHandle().GetLogicalDevice());//Wait GPU to complete all jobs before CPU destroy resources
}
#endif

void CApplication::initialize(){
    std::string fullYamlName = "../samples/yaml/" + m_sampleName + ".yaml";
    YAML::Node config;
    try{
        config = YAML::LoadFile(fullYamlName);
    } catch (...){
        std::cout<<"Error loading yaml file"<<std::endl;
        return;
    }

    /****************************
    * 1 Initialize ObjectList and LightList
    ****************************/
    if (config["Objects"]) {
        int max_object_id = 0;
        for (const auto& obj : config["Objects"]) {
            int object_id = obj["object_id"] ? obj["object_id"].as<int>() : 0;
            max_object_id = (object_id > max_object_id) ? object_id : max_object_id;
        }
        objectList.resize(((max_object_id+1) < config["Objects"].size())?(max_object_id+1):config["Objects"].size()); 
        std::cout<<"Object Size: "<<objectList.size()<<std::endl;
    }
    if (config["Lights"]) {
        int max_light_d = 0;
        for (const auto& light : config["Lights"]) {
            int light_id = light["light_id"] ? light["light_id"].as<int>() : 0;
            max_light_d = (light_id > max_light_d) ? light_id : max_light_d;
        }
        lightList.resize(((max_light_d+1) < config["Lights"].size())?(max_light_d+1):config["Lights"].size()); 
        std::cout<<"Light Size: "<<lightList.size()<<std::endl;
    }

    /****************************
    * 2 Read Features
    ****************************/   
    renderer.m_renderMode = appInfo.Render.Mode;
    if(appInfo.Buffer.GraphicsVertex.StructureType != VertexStructureTypes::NoType) CSupervisor::Activate_Buffer_Graphics_Vertex(appInfo.Buffer.GraphicsVertex.StructureType);

    bool b_feature_graphics_depth_test = config["Features"]["feature_graphics_depth_test"] ? config["Features"]["feature_graphics_depth_test"].as<bool>() : false;
    bool b_feature_graphics_msaa = config["Features"]["feature_graphics_msaa"] ? config["Features"]["feature_graphics_msaa"].as<bool>() : false;
    bool b_feature_graphics_48pbt = config["Features"]["feature_graphics_48pbt"] ? config["Features"]["feature_graphics_48pbt"].as<bool>() : false;
    bool b_feature_graphics_push_constant = config["Features"]["feature_graphics_push_constant"] ? config["Features"]["feature_graphics_push_constant"].as<bool>() : false;
    bool b_feature_graphics_blend = config["Features"]["feature_graphics_blend"] ? config["Features"]["feature_graphics_blend"].as<bool>() : false;
    bool b_feature_graphics_rainbow_mipmap = config["Features"]["feature_graphics_rainbow_mipmap"] ? config["Features"]["feature_graphics_rainbow_mipmap"].as<bool>() : false;
    int feature_graphics_pipeline_skybox_id = config["Features"]["feature_graphics_pipeline_skybox_id"] ? config["Features"]["feature_graphics_pipeline_skybox_id"].as<int>() : -1;
    
    if(b_feature_graphics_depth_test){
        swapchain.EnableDepthTest();
    }
    if(b_feature_graphics_msaa){
        swapchain.EnableMSAA();
        swapchain.EnableDepthTest();//If enable MSAA, must also enable Depth Test
    }
    if(b_feature_graphics_48pbt){
        CSupervisor::Activate_Feature_Graphics_48BPT();
    }
    if(b_feature_graphics_push_constant){
        CSupervisor::Activate_Feature_Graphics_PushConstant();
    }
    if(b_feature_graphics_blend){
        renderProcess.addColorBlendAttachment(
            VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);        
    }
    if(b_feature_graphics_rainbow_mipmap){
        CSupervisor::Activate_Feature_Graphics_RainbowMipmap();
    }
    renderProcess.skyboxID = feature_graphics_pipeline_skybox_id;

    /****************************
    * 4 Read Uniforms
    ****************************/
    bool b_uniform_graphics_custom = false;
    bool b_uniform_graphics_mvp = false;
    bool b_uniform_graphics_vp = false;
    bool b_uniform_graphics_lighting = false;
    bool b_uniform_compute_custom = false;
    bool b_uniform_compute_storage = false;
    bool b_uniform_compute_swapchain_storage = false;
    bool b_uniform_compute_texture_storage = false;

    for (const auto& uniform : config["Uniforms"]) {
        if (uniform["Graphics"]) {
            for (const auto& graphicsUniform : uniform["Graphics"]) {
                std::string name = graphicsUniform["uniform_graphics_name"] ? graphicsUniform["uniform_graphics_name"].as<std::string>() : "Default";
                b_uniform_graphics_custom = graphicsUniform["uniform_graphics_custom"] ? graphicsUniform["uniform_graphics_custom"].as<bool>() : false;
                b_uniform_graphics_mvp = graphicsUniform["uniform_graphics_mvp"] ? graphicsUniform["uniform_graphics_mvp"].as<bool>() : false;
                b_uniform_graphics_vp = graphicsUniform["uniform_graphics_vp"] ? graphicsUniform["uniform_graphics_vp"].as<bool>() : false;
                b_uniform_graphics_lighting = graphicsUniform["uniform_graphics_lighting"] ? graphicsUniform["uniform_graphics_lighting"].as<bool>() : false;

                if(b_uniform_graphics_custom){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_CUSTOM_GRAPHICS_BIT;
                    CGraphicsDescriptorManager::addCustomUniformBuffer(appInfo.Uniform.GraphicsCustom.Size);
                }

                if(b_uniform_graphics_mvp){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_MVP_BIT;
                    CGraphicsDescriptorManager::addMVPUniformBuffer();
                }

                if(b_uniform_graphics_vp){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_VP_BIT;
                    CGraphicsDescriptorManager::addVPUniformBuffer();
                }

                if(b_uniform_graphics_lighting){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_LIGHTING_GRAPHICS_BIT;
                    CGraphicsDescriptorManager::addLightingUniformBuffer();
                }
            }
        }

        if (uniform["Compute"]) {
            for (const auto& computeUniform : uniform["Compute"]) {
                std::string name = computeUniform["uniform_compute_name"] ? computeUniform["uniform_compute_name"].as<std::string>() : "Default";
                b_uniform_compute_custom = computeUniform["uniform_compute_custom"] ? computeUniform["uniform_compute_custom"].as<bool>() : false;
                b_uniform_compute_storage = computeUniform["uniform_compute_storage"] ? computeUniform["uniform_compute_storage"].as<bool>() : false;
                b_uniform_compute_swapchain_storage = computeUniform["uniform_compute_swapchain_storage"] ? computeUniform["uniform_compute_swapchain_storage"].as<bool>() : false;
                b_uniform_compute_texture_storage = computeUniform["uniform_compute_texture_storage"] ? computeUniform["uniform_compute_texture_storage"].as<bool>() : false;
            
                if(b_uniform_compute_custom){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_CUSTOM_COMPUTE_BIT;
                    CComputeDescriptorManager::addCustomUniformBuffer(appInfo.Uniform.ComputeCustom.Size);
                }

                if(b_uniform_compute_storage){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_STORAGE_BIT;
                    CComputeDescriptorManager::addStorageBuffer(appInfo.Uniform.ComputeStorageBuffer.Size, appInfo.Uniform.ComputeStorageBuffer.Usage);
                }

                if(b_uniform_compute_swapchain_storage){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_IMAGE_STORAGE_SWAPCHAIN_BIT;
                    CComputeDescriptorManager::addStorageImage(UNIFORM_IMAGE_STORAGE_SWAPCHAIN_BIT);
                }

                if(b_uniform_compute_texture_storage){
                    CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_IMAGE_STORAGE_TEXTURE_BIT;
                    CComputeDescriptorManager::addStorageImage(UNIFORM_IMAGE_STORAGE_TEXTURE_BIT);
                }
            }  
        }

        if (uniform["Samplers"]) {
            std::vector<int> miplevels;
            for (const auto& samplerUniform : uniform["Samplers"]) {
                std::string name = samplerUniform["uniform_sampler_name"] ? samplerUniform["uniform_sampler_name"].as<std::string>() : "Default";
                int miplevel = samplerUniform["uniform_sampler_miplevel"] ? samplerUniform["uniform_sampler_miplevel"].as<int>() : 1;
                miplevels.push_back(miplevel);
            }
            CDescriptorManager::uniformBufferUsageFlags |= UNIFORM_BUFFER_SAMPLER_BIT;
            CGraphicsDescriptorManager::addImageSamplerUniformBuffer(miplevels);
        }
    }

    bool b_uniform_graphics = b_uniform_graphics_custom || b_uniform_graphics_mvp || b_uniform_graphics_vp;
    bool b_uniform_compute = b_uniform_compute_custom || b_uniform_compute_storage || b_uniform_compute_swapchain_storage || b_uniform_compute_texture_storage;

    // std::cout<<"b_uniform_graphics_custom="<<b_uniform_graphics_custom<<std::endl;
    // std::cout<<"b_uniform_graphics_mvp="<<b_uniform_graphics_mvp<<std::endl;
    // std::cout<<"b_uniform_graphics_vp="<<b_uniform_graphics_vp<<std::endl;
    // std::cout<<"b_uniform_compute_custom="<<b_uniform_compute_custom<<std::endl;
    // std::cout<<"b_uniform_compute_storage="<<b_uniform_compute_storage<<std::endl;
    // std::cout<<"b_uniform_compute_swapchain_storage="<<b_uniform_compute_swapchain_storage<<std::endl;
    // std::cout<<"b_uniform_compute_texture_storage="<<b_uniform_compute_texture_storage<<std::endl;


    /****************************
    * 5 Read Resources
    ****************************/
    //When creating texture resource, need uniform information, so must read uniforms before read resources
    for (const auto& resource : config["Resources"]) {
        if (resource["Models"]) {
            for (const auto& model : resource["Models"]) {
                std::string name = model["resource_model_name"] ? model["resource_model_name"].as<std::string>() : "Default";
                //std::cout<<"model name: "<<name<<std::endl;
                //id is not really useful here, because the model id must be in order
                int id = model["resource_model_id"] ? model["resource_model_id"].as<int>() : 0;

                CSupervisor::VertexStructureType = VertexStructureTypes::ThreeDimension;
                if(name == "CUSTOM3D0"){
                    renderer.CreateVertexBuffer<Vertex3D>(modelManager.customModels3D[0].vertices); 
                    renderer.CreateIndexBuffer(modelManager.customModels3D[0].indices);
                    
                    modelManager.modelLengths.push_back(modelManager.customModels3D[0].length);
                    modelManager.modelLengthsMin.push_back(modelManager.customModels3D[0].lengthMin);
                    modelManager.modelLengthsMax.push_back(modelManager.customModels3D[0].lengthMax);
                }else if(name == "CUSTOM2D0"){
                    CSupervisor::VertexStructureType = VertexStructureTypes::TwoDimension;
                    renderer.CreateVertexBuffer<Vertex2D>(modelManager.customModels2D[0].vertices); 

                    modelManager.modelLengths.push_back(modelManager.customModels2D[0].length);
                    modelManager.modelLengthsMin.push_back(modelManager.customModels2D[0].lengthMin);
                    modelManager.modelLengthsMax.push_back(modelManager.customModels2D[0].lengthMax);
                }else{
                    CSupervisor::VertexStructureType = VertexStructureTypes::ThreeDimension;
                    std::vector<Vertex3D> modelVertices3D;
                    std::vector<uint32_t> modelIndices3D;
                    modelManager.LoadObjModel(name, modelVertices3D, modelIndices3D);
                    renderer.CreateVertexBuffer<Vertex3D>(modelVertices3D); 
                    renderer.CreateIndexBuffer(modelIndices3D);
                }
            }
        }

        if (resource["Textures"]) {
            //texture id is allocated by engine, instead of user, in order
            for (const auto& texture : resource["Textures"]) {
                std::string name = texture["resource_texture_name"].as<std::string>();
                //int id = texture["resource_texture_id"].as<int>();
                int miplevel = texture["resource_texture_miplevels"].as<int>();
                bool enableCubemap = texture["resource_texture_cubmap"].as<bool>();
                int samplerid = texture["uniform_Sampler_id"].as<int>();

                VkImageUsageFlags usage;// = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                //VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
                //for(int i = 0; i < textureAttributes->size(); i++){
                    //auto startTextureTime = std::chrono::high_resolution_clock::now();

                if(miplevel > 1) //mipmap
                    usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                else 
                    if(CSupervisor::Query_Uniform_Compute_StorageImage()) usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
                    else usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                if(!CSupervisor::b48bpt) //24bpt
                    if(CSupervisor::Query_Uniform_Compute_StorageImage_Swapchain()) textureManager.CreateTextureImage(name, usage, renderer.commandPool, miplevel, samplerid, swapchain.swapChainImageFormat);
                    else textureManager.CreateTextureImage(name, usage, renderer.commandPool, miplevel, samplerid, VK_FORMAT_R8G8B8A8_SRGB, 8, enableCubemap);  
                else //48bpt
                    textureManager.CreateTextureImage(name, usage, renderer.commandPool, miplevel, samplerid, VK_FORMAT_R16G16B16A16_UNORM, 16, enableCubemap); 
                
                if(CSupervisor::bRainbowMipmap){
                    VkImageUsageFlags usage_mipmap = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                    if(miplevel > 1) textureManager.textureImages[textureManager.textureImages.size()-1].generateMipmaps("checkerboard", usage_mipmap);
                }else if(miplevel > 1) textureManager.textureImages[textureManager.textureImages.size()-1].generateMipmaps();
                
                    //auto endTextureTime = std::chrono::high_resolution_clock::now();
                    //auto durationTime = std::chrono::duration<float, std::chrono::seconds::period>(endTextureTime - startTextureTime).count()*1000;
                    //std::cout<<"Load Texture '"<< (*textureNames)[i].first <<"' cost: "<<durationTime<<" milliseconds"<<std::endl;
                //}
            }
        }

        //shaders id is allocated by engine, not user, in order

        if (resource["VertexShaders"]) {
            auto vertexShaderList = std::make_unique<std::vector<std::string>>(std::vector<std::string>());
            for (const auto& vertexShader : resource["VertexShaders"]) {
                vertexShaderList->push_back(vertexShader["resource_vertexshader_name"].as<std::string>());
            }
            appInfo.Object.Pipeline.VertexShader = std::move(vertexShaderList);
            //std::cout<<"vertex shader:"<<appInfo.Object.Pipeline.VertexShader->size()<<std::endl;
        }

        if (resource["FragmentShaders"]) {
            auto fragmentShaderList = std::make_unique<std::vector<std::string>>(std::vector<std::string>());
            for (const auto& fragmentShader : resource["FragmentShaders"]) {
                fragmentShaderList->push_back(fragmentShader["resource_fragmentshader_name"].as<std::string>());
            }
            appInfo.Object.Pipeline.FragmentShader = std::move(fragmentShaderList);
        }

        if (resource["ComputeShaders"]) {
            auto computeShaderList = std::make_unique<std::vector<std::string>>(std::vector<std::string>());
            for (const auto& computeShader : resource["ComputeShaders"]) {
                computeShaderList->push_back(computeShader["resource_computeshader_name"].as<std::string>());
            }
            appInfo.Object.Pipeline.ComputeShader = std::move(computeShaderList);
        }
    }

    /****************************
    * 6 Create Uniform Descriptors
    ****************************/
    //UNIFORM STEP 1/3 (Pool)
    CDescriptorManager::createDescriptorPool(objectList.size()); 
    //UNIFORM STEP 2/3 (Layer)
    if(b_uniform_graphics){
        if(b_uniform_graphics_custom) 
             CGraphicsDescriptorManager::createDescriptorSetLayout(&appInfo.Uniform.GraphicsCustom.Binding); 
        else CGraphicsDescriptorManager::createDescriptorSetLayout(); 
        if(CGraphicsDescriptorManager::textureSamplers.size()>0) CGraphicsDescriptorManager::createTextureDescriptorSetLayout(); 
    }
    if(b_uniform_compute){
        if(b_uniform_compute_custom) CComputeDescriptorManager::createDescriptorSetLayout(&appInfo.Uniform.ComputeCustom.Binding);
        else CComputeDescriptorManager::createDescriptorSetLayout();
    }
    //UNIFORM STEP 3/3 (Set)
    if(b_uniform_graphics)
        graphicsDescriptorManager.createDescriptorSets(); 
    if(b_uniform_compute){
        if(b_uniform_compute_swapchain_storage) {
            if(b_uniform_compute_texture_storage)
                computeDescriptorManager.createDescriptorSets(&(textureManager.textureImages), &(swapchain.views));//this must be called after texture resource is loaded
            else computeDescriptorManager.createDescriptorSets(NULL, &(swapchain.views));
        }else computeDescriptorManager.createDescriptorSets();
    }
    //std::cout<<"test2"<<std::endl;

    /****************************
    * 7 Create Pipelines
    ****************************/
    //std::cout<<"before Activate_Pipeline()"<<std::endl;
    CSupervisor::Activate_Pipeline();
    //std::cout<<"after Activate_Pipeline()"<<std::endl;

    /****************************
    * 8 Read and Register Objects
    ****************************/
    if (config["Objects"]) {
        //std::cerr << "No 'Objects' key found in the YAML file!" << std::endl;
        for (const auto& obj : config["Objects"]) {
            int object_id = obj["object_id"] ? obj["object_id"].as<int>() : 0;
            if(objectList[object_id].bRegistered) {
                std::cout<<"WARNING: Trying to register a registered Object id("<<object_id<<")!"<<std::endl;
                continue;
            }

            //std::cout<<"before register Object id("<<object_id<<")!"<<std::endl;
            int resource_model_id = obj["resource_model_id"] ? obj["resource_model_id"].as<int>() : 0;
            auto resource_texture_id_list = obj["resource_texture_id_list"] ? obj["resource_texture_id_list"].as<std::vector<int>>() : std::vector<int>(1, 0);
            int resource_graphics_pipeline_id = obj["resource_graphics_pipeline_id"] ? obj["resource_graphics_pipeline_id"].as<int>() : 0;
            //must load resources before object register
            objectList[object_id].Register((CApplication*)this, object_id, resource_texture_id_list, resource_model_id, resource_graphics_pipeline_id);
            //std::cout<<"after register Object id("<<object_id<<")!"<<std::endl;

            std::string name = obj["object_name"] ? obj["object_name"].as<std::string>() : "Default";
            objectList[object_id].Name = name;

            //set scale after model is registered, otherwise the length will not be computed correctly
            float object_scale = obj["object_scale"] ? obj["object_scale"].as<float>() : 1.0f;
            objectList[object_id].SetScale(object_scale);

            auto position = obj["object_position"] ? obj["object_position"].as<std::vector<float>>(): std::vector<float>(3, 0);
            objectList[object_id].SetPosition(position[0], position[1], position[2]);

            auto rotation = obj["object_rotation"] ? obj["object_rotation"].as<std::vector<float>>(): std::vector<float>(3, 0);
            objectList[object_id].SetRotation(rotation[0], rotation[1], rotation[2]);

            auto velocity = obj["object_velocity"] ? obj["object_velocity"].as<std::vector<float>>(): std::vector<float>(3, 0);
            objectList[object_id].SetVelocity(velocity[0], velocity[1], velocity[2]);

            auto angular_velocity = obj["object_angular_velocity"] ? obj["object_angular_velocity"].as<std::vector<float>>(): std::vector<float>(3, 0);
            objectList[object_id].SetAngularVelocity(angular_velocity[0], angular_velocity[1], angular_velocity[2]);

            bool isSkybox = obj["object_skybox"] ? obj["object_skybox"].as<bool>() : false;
            objectList[object_id].bSkybox = isSkybox;
            //if(graphics_pipeline_id == appInfo.Feature.GraphicsPipelineSkyboxID)  objectList[i].bSkybox = true;

            std::cout<<"ObjectId:("<<object_id<<") Name:("<<objectList[object_id].Name<<") Length:("<<objectList[object_id].Length.x<<","<<objectList[object_id].Length.y<<","<<objectList[object_id].Length.z<<")"
                <<" Position:("<<objectList[object_id].Position.x<<","<<objectList[object_id].Position.y<<","<<objectList[object_id].Position.z<<")"<<std::endl;
        }
        for(int i = 0; i < objectList.size(); i++)
            if(!objectList[i].bRegistered) std::cout<<"WARNING: Object id("<<i<<") is not registered!"<<std::endl;
    }

    /****************************
    * 3 Read Lightings
    ****************************/
    if (config["Lights"]) {
        for (const auto& light : config["Lights"]) {
            int id = light["light_id"] ? light["light_id"].as<int>() : 0;
            if(lightList[id].bRegistered) {
                std::cout<<"WARNING: Trying to register a registered Light id("<<id<<")!"<<std::endl;
                continue;
            }
            
            std::string name = light["light_name"] ? light["light_name"].as<std::string>() : "Default";

            auto position = light["light_position"] ? light["light_position"].as<std::vector<float>>(): std::vector<float>(3,0);
            glm::vec3 glm_position(position[0], position[1], position[2]);
            //graphicsDescriptorManager.m_lightingUBO.lights[id].lightPos = glm::vec4(glm_position, 0);

            auto intensity = light["light_intensity"] ? light["light_intensity"].as<std::vector<float>>(): std::vector<float>(4,0);
            // graphicsDescriptorManager.m_lightingUBO.lights[id].ambientIntensity = intensity[0];
            // graphicsDescriptorManager.m_lightingUBO.lights[id].diffuseIntensity = intensity[1];
            // graphicsDescriptorManager.m_lightingUBO.lights[id].specularIntensity = intensity[2];
            // graphicsDescriptorManager.m_lightingUBO.lights[id].dimmerSwitch = intensity[3];
            
            lightList[id].Register(name, id, glm_position, intensity);
            std::cout<<"LightId:("<<id<<") Name:("<<lightList[id].GetLightName()<<") Intensity:("<<lightList[id].GetIntensity(0)<<","<<lightList[id].GetIntensity(1)<<","<<lightList[id].GetIntensity(2)<<","<<lightList[id].GetIntensity(3)<<")"
                <<" Position:("<<lightList[id].GetLightPosition().x<<","<<lightList[id].GetLightPosition().y<<","<<lightList[id].GetLightPosition().z<<")"<<std::endl;
 
        }
        for(int i = 0; i < lightList.size(); i++)
            if(!lightList[i].bRegistered) std::cout<<"WARNING: Light id("<<i<<") is not registered!"<<std::endl;
    }
    



    // if(config["Lighting"]["Position"].size() > 0) {
    //     std::unique_ptr<std::vector<std::vector<float>>> lightPosition = std::make_unique<std::vector<std::vector<float>>>(config["Lighting"]["Position"].as<std::vector<std::vector<float>>>());
    //     for(int i = 0; i < lightPosition->size(); i++) //SetLightPosition(i, (*lightPosition)[i]);
    //         graphicsDescriptorManager.m_lightingUBO.lights[i].lightPos = glm::vec4(glm::vec3((*lightPosition)[i][0], (*lightPosition)[i][1], (*lightPosition)[i][2]), 0);
    //     LightCount = lightPosition->size();
    // }
    // if(config["Lighting"]["Intensity"].size() > 0) {
    //     std::unique_ptr<std::vector<std::vector<float>>> lightIntensity = std::make_unique<std::vector<std::vector<float>>>(config["Lighting"]["Intensity"].as<std::vector<std::vector<float>>>());
    //     for(int i = 0; i < lightIntensity->size(); i++){
    //         graphicsDescriptorManager.m_lightingUBO.lights[i].ambientIntensity = (*lightIntensity)[i][0];
    //         graphicsDescriptorManager.m_lightingUBO.lights[i].diffuseIntensity = (*lightIntensity)[i][1];
    //         graphicsDescriptorManager.m_lightingUBO.lights[i].specularIntensity = (*lightIntensity)[i][2];
    //         graphicsDescriptorManager.m_lightingUBO.lights[i].dimmerSwitch = (*lightIntensity)[i][3];
    //     }
    // }


    /****************************
    * 9 Read Main Camera
    ****************************/
    bool b_camera_free_mode = config["MainCamera"]["camera_free_mode"] ? config["MainCamera"]["camera_free_mode"].as<bool>() : false;
    if(b_camera_free_mode) mainCamera.cameraType = Camera::CameraType::freemove;
    mainCamera.SetPosition(
        config["MainCamera"]["camera_position"][0].as<float>(), 
        config["MainCamera"]["camera_position"][1].as<float>(), 
        config["MainCamera"]["camera_position"][2].as<float>());
    mainCamera.SetRotation(
        config["MainCamera"]["camera_rotation"][0].as<float>(), 
        config["MainCamera"]["camera_rotation"][1].as<float>(), 
        config["MainCamera"]["camera_rotation"][2].as<float>());
    mainCamera.SetTargetPosition(
        config["MainCamera"]["camera_target_position"][0].as<float>(), 
        config["MainCamera"]["camera_target_position"][1].as<float>(), 
        config["MainCamera"]["camera_target_position"][2].as<float>());
    mainCamera.setPerspective(
        config["MainCamera"]["camera_fov"].as<float>(),  
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        config["MainCamera"]["camera_z"][0].as<float>(), 
        config["MainCamera"]["camera_z"][1].as<float>());

    /****************************
    * 10 Create Sync Objects and Clean up Shaders
    ****************************/
    renderer.CreateSyncObjects(swapchain.imageSize);
    shaderManager.Destroy();

    // CContext::GetHandle().logManager.print("Test single string!\n");
    // CContext::GetHandle().logManager.print("Test interger: %d!\n", 999);
    // CContext::GetHandle().logManager.print("Test float: %f!\n", 1.234f);
    // CContext::GetHandle().logManager.print("Test string: %s!\n", "another string");
    // float mat[4] = {1.1, 2.2, 3.3, 4.4};
    // CContext::GetHandle().logManager.print("Test vector: \n", mat, 4);
    // CContext::GetHandle().logManager.print("Test two floats:  %f, %f!\n", 1.2, 2.3);

    // PRINT("Test single string!");
    // PRINT("Test interger: %d!", 999);
    // PRINT("Test float: %f!", 1.234f);
    // PRINT("Test string: %s!", "another string");
    // float mat[4] = {1.1, 2.2, 3.3, 4.4};
    // PRINT("Test vector: ", mat, 4);
    // PRINT("Test two floats:  %f, %f!", 1.2, 2.3);    
}

void CApplication::update(){
    static auto startTime = std::chrono::high_resolution_clock::now();
    static auto lastTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    durationTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
    lastTime = currentTime;

    mainCamera.update(deltaTime);
    for(int i = 0; i < objectList.size(); i++) objectList[i].Update(deltaTime, renderer.currentFrame, mainCamera); 
    for(int i = 0; i < lightList.size(); i++) lightList[i].Update(deltaTime, renderer.currentFrame, mainCamera); 
    
}

void CApplication::recordGraphicsCommandBuffer(){}
void CApplication::recordComputeCommandBuffer(){}
void CApplication::postUpdate(){}

void CApplication::UpdateRecordRender(){
    update();

    /**************************
     * 
     * Universial Render Functions
     * 
     * ***********************/
    switch(renderer.m_renderMode){
        case CRenderer::RENDER_GRAPHICS_Mode:
        //case renderer.RENDER_GRAPHICS_Mode:
            //std::cout<<"RENDER_GRAPHICS_Mode"<<std::endl;

            //must wait for fence before record command buffer
            renderer.WaitForGraphicsFence();
            //must aquire swap image before record command buffer
            renderer.AquireSwapchainImage(swapchain); 

            vkResetCommandBuffer(renderer.commandBuffers[renderer.graphicsCmdId][renderer.currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

            renderer.StartRecordGraphicsCommandBuffer(
                renderProcess.renderPass, 
                swapchain.swapChainFramebuffers,swapchain.swapChainExtent, 
                renderProcess.clearValues);
            recordGraphicsCommandBuffer();
            renderer.EndRecordGraphicsCommandBuffer();

            renderer.SubmitGraphics();

            renderer.PresentSwapchainImage(swapchain); 
        break;
        case CRenderer::RENDER_COMPUTE_Mode:
        //case renderer.RENDER_COMPUTE_Mode:
            //std::cout<<"Application: RENDER_COMPUTE_Mode."<<std::endl;
            renderer.WaitForComputeFence();//must wait for fence before record
            //std::cout<<"Application: renderer.WaitForComputeFence()"<<std::endl;

            vkResetCommandBuffer(renderer.commandBuffers[renderer.computeCmdId][renderer.currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
            //std::cout<<"Application: vkResetCommandBuffer"<<std::endl;

            renderer.StartRecordComputeCommandBuffer(renderProcess.computePipeline, renderProcess.computePipelineLayout);
            recordComputeCommandBuffer();
            renderer.EndRecordComputeCommandBuffer();
            //std::cout<<"Application: recordComputeCommandBuffer()"<<std::endl;

            renderer.SubmitCompute();
            //std::cout<<"Application: renderer.SubmitCompute()"<<std::endl;

           // renderer.PresentSwapchainImage(swapchain); //???
        break;
        case CRenderer::RENDER_COMPUTE_SWAPCHAIN_Mode:
        //case renderer.RENDER_COMPUTE_SWAPCHAIN_Mode:
            //must wait for fence before record
            renderer.WaitForComputeFence();
            //must aquire swap image before record command buffer
            renderer.AquireSwapchainImage(swapchain);
            //std::cout<<"Application: renderer.imageIndex = "<<renderer.imageIndex<< std::endl;
            //std::cout<<"Application: renderer.currentFrame = "<<renderer.currentFrame<< std::endl;

            //vkResetCommandBuffer(renderer.commandBuffers[renderer.computeCmdId][renderer.currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

            //in this mode, nothing is recorded(all commands are pre-recorded), for NOW. But still, swapchain will be presented.
            //renderer.StartRecordComputeCommandBuffer(renderProcess.computePipeline, renderProcess.computePipelineLayout);
            //recordComputeCommandBuffer();
            //renderer.EndRecordComputeCommandBuffer();

            renderer.SubmitCompute(); 

            renderer.PresentSwapchainImage(swapchain); 
        break;
        case CRenderer::RENDER_COMPUTE_GRAPHICS_Mode:
        //case renderer.RENDER_COMPUTE_GRAPHICS_Mode:
            renderer.WaitForComputeFence();//must wait for fence before record
            renderer.WaitForGraphicsFence();//must wait for fence before record
            renderer.AquireSwapchainImage(swapchain);//must aquire swap image before record command buffer

            vkResetCommandBuffer(renderer.commandBuffers[renderer.graphicsCmdId][renderer.currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
            vkResetCommandBuffer(renderer.commandBuffers[renderer.computeCmdId][renderer.currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
            
            renderer.StartRecordComputeCommandBuffer(renderProcess.computePipeline, renderProcess.computePipelineLayout);
            recordComputeCommandBuffer();
            renderer.EndRecordComputeCommandBuffer();

            renderer.StartRecordGraphicsCommandBuffer(
                renderProcess.renderPass, 
                swapchain.swapChainFramebuffers,swapchain.swapChainExtent, 
                renderProcess.clearValues);
            recordGraphicsCommandBuffer();
            renderer.EndRecordGraphicsCommandBuffer();
            
            renderer.SubmitCompute(); 
            renderer.SubmitGraphics(); 

            renderer.PresentSwapchainImage(swapchain); 
        break;
        default:
        break;
    }

    //renderer.RecordCompute();
    //recordComputeCommandBuffer();
    //renderer.RecordGraphics();
    //recordGraphicsCommandBuffer();

    //renderer.AquireSwapchainImage(swapchain);
    //renderer.SubmitCompute();
    //renderer.SubmitGraphics();
    //renderer.PresentSwapchainImage(swapchain);     

    //if(renderProcess.bCreateGraphicsPipeline){
        //renderer.preRecordGraphicsCommandBuffer(swapchain);
        //recordGraphicsCommandBuffer();
        //renderer.postRecordGraphicsCommandBuffer(swapchain);
    //}

    //if(renderProcess.bCreateComputePipeline){
        //recordComputeCommandBuffer();
        //renderer.preRecordComputeCommandBuffer(swapchain);
        //renderer.postRecordComputeCommandBuffer(swapchain);
   //}

    postUpdate();

    renderer.Update(); //update currentFrame    
}


#ifndef ANDROID
void CApplication::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
#endif

void CApplication::CleanUp(){
    swapchain.CleanUp();
    renderProcess.Cleanup();
   //for(int i = 0; i < descriptors.size(); i++)
    //    descriptors[i].DestroyAndFree();
    graphicsDescriptorManager.DestroyAndFree();
    computeDescriptorManager.DestroyAndFree();
    //textureDescriptor.DestroyAndFree();
    //for(int i = 0; i < textureImages.size(); i++) textureImages[i].Destroy();
    //for(int i = 0; i < textureImages1.size(); i++) textureImages1[i].Destroy();
    //for(int i = 0; i < textureImages2.size(); i++) textureImages2[i].Destroy();
    textureManager.Destroy();
    renderer.Destroy();

    vkDestroyDevice(CContext::GetHandle().GetLogicalDevice(), nullptr);

#ifndef ANDROID
    if (enableValidationLayers) 
        DestroyDebugUtilsMessengerEXT(instance->getHandle(), instance->debugMessenger, nullptr);
#endif

    vkDestroySurfaceKHR(instance->getHandle(), surface, nullptr);
    vkDestroyInstance(instance->getHandle(), nullptr);
    
    CContext::Quit();
}

CApplication::~CApplication(){
    CleanUp();
}

/*************
 * Helper Functions
 *******/
void CApplication::Dispatch(int numWorkGroupsX, int numWorkGroupsY, int numWorkGroupsZ){
    CSupervisor::Dispatch(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
}
