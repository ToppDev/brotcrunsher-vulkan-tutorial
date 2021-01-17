//#include "stdafx.h"
#include "VulkanUtils.h"
//GLFW automatically includes Vulkan (#include "vulkan/vulkan.h")
#include <iostream>
#include <cstring>
#include <vector>
#include <fstream>
//#define GLM_FORCE_CTOR_INIT // Needed so that mat4() produces not zero matrix, but identity matrix
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include "EasyImage.h"
#include "DepthImage.h"
#include "Vertex.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "MeshHelper.h"

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK __builtin_trap()
#endif


VkInstance instance;
std::vector<VkPhysicalDevice> physicalDevices;
VkSurfaceKHR surface;
VkDevice device;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkImageView *imageViews;
VkFramebuffer *framebuffers;
VkShaderModule shaderModuleVert;
VkShaderModule shaderModuleFrag;
VkRenderPass renderPass;
Pipeline pipeline;
Pipeline pipelineWireframe;
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers;
VkSemaphore semaphoreImageAvailable;
VkSemaphore semaphoreRenderingDone;
VkQueue queue;
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferDeviceMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferDeviceMemory;
VkBuffer uniformBuffer;
VkDeviceMemory uniformBufferMemory;
uint32_t amountOfImagesInSwapchain = 0;
GLFWwindow *window;

uint32_t width = 1800;
uint32_t height = 900;
const VkFormat vulkanFormat = VK_FORMAT_B8G8R8A8_UNORM; //TODO: Check if valid

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 lightPosition;
};

UniformBufferObject ubo;

VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;

EasyImage elfImage;
EasyImage dirtTexture;
EasyImage dirtNormalTexture;
DepthImage depthImage;
Mesh dragonMesh;


std::vector<Vertex> vertices = {
    /*Vertex({-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
    Vertex({ 0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),
    Vertex({-0.5f,  0.5f,  0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}),
    Vertex({ 0.5f, -0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),

    Vertex({-0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
    Vertex({ 0.5f,  0.5f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),
    Vertex({-0.5f,  0.5f, -1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}),
    Vertex({ 0.5f, -0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),*/
};

std::vector<uint32_t> indices = {
    /* 0, 1, 2, 0, 3, 1,
    4, 5, 6, 4, 7, 5*/
};

void printStats(VkPhysicalDevice &device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    std::cout << "Name:                     " << properties.deviceName << std::endl;
    uint32_t apiVer = properties.apiVersion;
    std::cout << "API Version:              " << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;
    std::cout << "Driver Version:           " << properties.driverVersion << std::endl;
    std::cout << "Vendor ID:                " << properties.vendorID << std::endl;
    std::cout << "Device ID:                " << properties.deviceID << std::endl;
    std::cout << "Device Type:              " << properties.deviceType << std::endl; // OTHER = 0, INTEGRATED_GPU = 1, DISCRETE_GPU = 2, VIRTUAL_GPU = 3, CPU = 4
    std::cout << "discreteQueuePriorities:  " << properties.limits.discreteQueuePriorities << std::endl;

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    std::cout << "Geometry Shader:          " << features.geometryShader << std::endl;
    std::cout << "FillModeNonSolid:         " << features.fillModeNonSolid << std::endl;

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(device, &memProp);

    uint32_t amountOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, nullptr);
    VkQueueFamilyProperties *familyProperties = new VkQueueFamilyProperties[amountOfQueueFamilies];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, familyProperties);

    std::cout << "Amount of Queue Families: " << amountOfQueueFamilies << std::endl;

    for (uint32_t i = 0; i < amountOfQueueFamilies; i++) {
        std::cout << std::endl;
        std::cout << "Queue Family #" << i << std::endl;
        std::cout << "VK_QUEUE_GRAPHICS_BIT       " << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT        " << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT       " << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_SPARSE_BINDING_BIT " << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;
        std::cout << "Queue Count: " << familyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << familyProperties[i].timestampValidBits << std::endl;
        uint32_t width = familyProperties[i].minImageTransferGranularity.width;
        uint32_t height = familyProperties[i].minImageTransferGranularity.height;
        uint32_t depth = familyProperties[i].minImageTransferGranularity.depth;
        std::cout << "Min Image Timestamp Granularity: " << width << ", " << height << ", " << depth << std::endl;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilites);
    ASSERT_VULKAN(result);

    std::cout << std::endl;
    std::cout << "Surface capabilites: " << std::endl;
    std::cout << "\tminImageCount: " << surfaceCapabilites.minImageCount << std::endl;
    std::cout << "\tmaxImageCount: " << surfaceCapabilites.maxImageCount << std::endl;
    std::cout << "\tcurrentExtent: " << surfaceCapabilites.currentExtent.width << "/" << surfaceCapabilites.currentExtent.height << std::endl;
    std::cout << "\tminImageExtent: " << surfaceCapabilites.minImageExtent.width << "/" << surfaceCapabilites.minImageExtent.height << std::endl;
    std::cout << "\tmaxImageExtent: " << surfaceCapabilites.maxImageExtent.width << "/" << surfaceCapabilites.maxImageExtent.height << std::endl;
    std::cout << "\tmaxImageArrayLayers: " << surfaceCapabilites.maxImageArrayLayers << std::endl;
    std::cout << "\tsupportedTransforms: " << surfaceCapabilites.supportedTransforms << std::endl;
    std::cout << "\tcurrentTransform: " << surfaceCapabilites.currentTransform << std::endl;
    std::cout << "\tsupportedCompositeAlpha: " << surfaceCapabilites.supportedCompositeAlpha << std::endl;
    std::cout << "\tsupportedUsageFlags: " << surfaceCapabilites.supportedUsageFlags << std::endl;

    uint32_t amountOfFormats = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &amountOfFormats, nullptr);
    ASSERT_VULKAN(result);
    VkSurfaceFormatKHR *surfaceFormats = new VkSurfaceFormatKHR[amountOfFormats];
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &amountOfFormats, surfaceFormats);
    ASSERT_VULKAN(result);

    std::cout << std::endl;
    std::cout << "Amount of Formats: " << amountOfFormats << std::endl;
    for (uint32_t i = 0; i < amountOfFormats; i++) {
        std::cout << "\tFormat: " << surfaceFormats[i].format << std::endl;
    }

    uint32_t amountOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &amountOfPresentationModes, nullptr);
    VkPresentModeKHR *presentModes = new VkPresentModeKHR[amountOfPresentationModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &amountOfPresentationModes, presentModes);

    std::cout << std::endl;
    std::cout << "Amount of Presentation Modes: " << amountOfPresentationModes << std::endl;
    for (uint32_t i = 0; i < amountOfPresentationModes; i++) {
        std::cout << "\tSupported mode: " << presentModes[i] << std::endl;
    }

    std::cout << std::endl;

    delete[] presentModes;
    delete[] surfaceFormats;
    delete[] familyProperties;
}

std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file) {
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> fileBuffer(fileSize);
        file.seekg(0);
        file.read(fileBuffer.data(), fileSize);
        file.close();
        return fileBuffer;
    }
    else {
        DEBUG_BREAK;
        throw std::runtime_error("Failed to open file!!!");
    }
}

void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo shaderCreateInfo;
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.pNext = nullptr;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.codeSize = code.size();
    shaderCreateInfo.pCode = (uint32_t*)code.data();

    VkResult result = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, shaderModule);
    ASSERT_VULKAN(result);
}

void createInstance() {
    std::cout << "Vulkan Header Version: " << VK_HEADER_VERSION << std::endl;

    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Vulkan Tutorial";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.pEngineName = "Vulkan Test Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    const std::vector<const char*> usedLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    uint32_t amountOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&amountOfGlfwExtensions);

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = usedLayers.size();
    instanceInfo.ppEnabledLayerNames = usedLayers.data();
    instanceInfo.enabledExtensionCount = amountOfGlfwExtensions;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    ASSERT_VULKAN(result);
}

void printInstanceLayers() {
    uint32_t amountOfLayers = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&amountOfLayers, nullptr);
    ASSERT_VULKAN(result);
    VkLayerProperties *layers = new VkLayerProperties[amountOfLayers];
    result = vkEnumerateInstanceLayerProperties(&amountOfLayers, layers);
    ASSERT_VULKAN(result);

    std::cout << "Amount of Instace Layers: " << amountOfLayers << std::endl;
    for (uint32_t i = 0; i < amountOfLayers; i++) {
        std::cout << std::endl;
        std::cout << "Name:         " << layers[i].layerName << std::endl;
        std::cout << "Spec Version: " << layers[i].specVersion << std::endl;
        std::cout << "Impl Version: " << layers[i].implementationVersion << std::endl;
        std::cout << "Description:  " << layers[i].description << std::endl;
    }
    delete[] layers;
}

void printInstanceExtensions() {
    uint32_t amountOfExtensions = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, nullptr);
    ASSERT_VULKAN(result);
    VkExtensionProperties *extensions = new VkExtensionProperties[amountOfExtensions];
    result = vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, extensions);
    ASSERT_VULKAN(result);

    std::cout << std::endl;
    std::cout << "Amount of Extensions: " << amountOfExtensions << std::endl;
    for (uint32_t i = 0; i < amountOfExtensions; i++) {
        std::cout << std::endl;
        std::cout << "Name:         " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version: " << extensions[i].specVersion << std::endl;
    }
    delete[] extensions;
}

void createGlfwWindowSurface() {
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    ASSERT_VULKAN(result);
}

std::vector<VkPhysicalDevice> getAllPhysicalDevices() {
    uint32_t amountOfPhysicalDevices = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &amountOfPhysicalDevices, nullptr);
    ASSERT_VULKAN(result);
    std::vector<VkPhysicalDevice> physicalDevices(amountOfPhysicalDevices);

    result = vkEnumeratePhysicalDevices(instance, &amountOfPhysicalDevices, physicalDevices.data());
    ASSERT_VULKAN(result);

    return physicalDevices;
}

void printStatsOfAllPhysicalDevices() {
    std::cout << std::endl;
    for (uint32_t i = 0; i < physicalDevices.size(); i++) {
        printStats(physicalDevices[i]);
    }
}

void createLogicalDevice() {
    float queuePrios[] = {1.0f, 1.0f, 1.0f, 1.0f};

    VkDeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = 0;    // TODO: Choose correct family index
    deviceQueueCreateInfo.queueCount = 1;          // TODO: Check if this amount is valid
    deviceQueueCreateInfo.pQueuePriorities = queuePrios;

    VkPhysicalDeviceFeatures usedFeatures = {};
    usedFeatures.samplerAnisotropy = VK_TRUE;
    usedFeatures.fillModeNonSolid = VK_TRUE; // Check if graphics card lets us do this

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &usedFeatures;

    // TODO: pick "best device" instead of first device
    VkResult result = vkCreateDevice(physicalDevices[0], &deviceCreateInfo, nullptr, &device);
    ASSERT_VULKAN(result);
}

void createQueue() {
    vkGetDeviceQueue(device, 0, 0, &queue);
}

void checkSurfaceSupport() {
    VkBool32 surfaceSupport = false;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], 0, surface, &surfaceSupport);
    ASSERT_VULKAN(result);

    if (!surfaceSupport) {
        std::cerr << "Surface not supported!" << std::endl;
        DEBUG_BREAK;
    }
}

void createSwapchain() {
    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = 3; //TODO: Check if valid
    swapchainCreateInfo.imageFormat = vulkanFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; //TODO: Check if valid
    swapchainCreateInfo.imageExtent = VkExtent2D{ width, height };
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //TODO: Check if valid
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //TODO: Check if valid or if better available
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = swapchain;

    VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
    ASSERT_VULKAN(result);
}

void createImageViews() {
    VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &amountOfImagesInSwapchain, nullptr);
    ASSERT_VULKAN(result);
    VkImage *swapchainImages = new VkImage[amountOfImagesInSwapchain];
    result = vkGetSwapchainImagesKHR(device, swapchain, &amountOfImagesInSwapchain, swapchainImages);
    ASSERT_VULKAN(result);

    imageViews = new VkImageView[amountOfImagesInSwapchain];
    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        createImageView(device, swapchainImages[i], vulkanFormat, VK_IMAGE_ASPECT_COLOR_BIT, imageViews[i]);
    }   
    delete[] swapchainImages;
}

void createRenderPass() {
    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = vulkanFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = DepthImage::getDepthAttachment(physicalDevices[0]);

    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;

    std::vector<VkAttachmentDescription> attachments;
    attachments.push_back(attachmentDescription);
    attachments.push_back(depthAttachment);

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    ASSERT_VULKAN(result);
}

void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding;
    samplerDescriptorSetLayoutBinding.binding = 1;
    samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorSetLayoutBinding.descriptorCount = 1;
    samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerNormalDescriptorSetLayoutBinding;
    samplerNormalDescriptorSetLayoutBinding.binding = 2;
    samplerNormalDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerNormalDescriptorSetLayoutBinding.descriptorCount = 1;
    samplerNormalDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerNormalDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> descriptorSets;
    descriptorSets.push_back(descriptorSetLayoutBinding);
    descriptorSets.push_back(samplerDescriptorSetLayoutBinding);
    descriptorSets.push_back(samplerNormalDescriptorSetLayoutBinding);

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();

    VkResult result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
    ASSERT_VULKAN(result);
}

void createPipeline() {
    auto shaderCodeVert = readFile("shader/vert.spv");
    auto shaderCodeFrag = readFile("shader/frag.spv");

    createShaderModule(shaderCodeVert, &shaderModuleVert);
    createShaderModule(shaderCodeFrag, &shaderModuleFrag);

    pipeline.ini(shaderModuleVert, shaderModuleFrag, width, height);
    pipeline.create(device, renderPass, descriptorSetLayout);

    pipelineWireframe.ini(shaderModuleVert, shaderModuleFrag, width, height);
    pipelineWireframe.setPolygonMode(VK_POLYGON_MODE_LINE);
    pipelineWireframe.create(device, renderPass, descriptorSetLayout);
}

void createFramebuffers() {
    framebuffers = new VkFramebuffer[amountOfImagesInSwapchain];
    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        std::vector<VkImageView> attachmentViews;
        attachmentViews.push_back(imageViews[i]);
        attachmentViews.push_back(depthImage.getImageView());

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = attachmentViews.size();
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = width;
        framebufferCreateInfo.height = height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &(framebuffers[i]));
        ASSERT_VULKAN(result);
    }
}

void createCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = 0; // TODO: Check if valid, needs VK_QUEUE_GRAPHICS_BIT

    VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    ASSERT_VULKAN(result);
}

void createDepthImage()
{
    depthImage.create(device, physicalDevices[0], commandPool, queue, width, height);
}

void createCommandBuffers() {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = amountOfImagesInSwapchain;

    commandBuffers = new VkCommandBuffer[amountOfImagesInSwapchain];
    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers);
    ASSERT_VULKAN(result);
}

void loadTexture() {
    elfImage.load("images/world-of-warcraft-blood-elf.jpg");

    std::cout << "Loading image: " << std::endl;
    std::cout << "\tResolution: " << elfImage.getWidth() << "/" << elfImage.getHeight() << std::endl;
    std::cout << "\tSize in Bytes: " << elfImage.getSizeInBytes() << std::endl;

    elfImage.upload(device, physicalDevices[0], commandPool, queue);

    dirtTexture.load("images/dirt.jpg");
    dirtTexture.upload(device, physicalDevices[0], commandPool, queue);

    dirtNormalTexture.load("images/dirt_normal.jpg");
    dirtNormalTexture.upload(device, physicalDevices[0], commandPool, queue);
}

void loadMesh()
{
    //dragonMesh.create("meshes/dragon.obj");
    //vertices = dragonMesh.getVertices();
    //indices = dragonMesh.getIndices();
    vertices = getQuadVertices();
    indices = getQuadIndices();
}

void createVertexBuffer() {
    createAndUploadBuffer(device, physicalDevices[0], queue, commandPool, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferDeviceMemory);
}

void createIndexBuffer() {
    createAndUploadBuffer(device, physicalDevices[0], queue, commandPool, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferDeviceMemory);
}

void createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(ubo);    
    createBuffer(device, physicalDevices[0], bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferMemory);
}

void createDescriptorPool() {
    VkDescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = 1;

    VkDescriptorPoolSize samplerPoolSize;
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = 2;

    std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    descriptorPoolSizes.push_back(descriptorPoolSize);
    descriptorPoolSizes.push_back(samplerPoolSize);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
    ASSERT_VULKAN(result);
}

void createDescriptorSet() {
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet);
    ASSERT_VULKAN(result);

    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(ubo);

    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pBufferInfo = &descriptorBufferInfo;
    descriptorWrite.pTexelBufferView = nullptr;

    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = dirtTexture.getSampler();
    descriptorImageInfo.imageView = dirtTexture.getImageView();
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo descriptorImageNormalInfo;
    descriptorImageNormalInfo.sampler = dirtNormalTexture.getSampler();
    descriptorImageNormalInfo.imageView = dirtNormalTexture.getImageView();
    descriptorImageNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::vector<VkDescriptorImageInfo> descriptorImageInfos = {descriptorImageInfo, descriptorImageNormalInfo};

    VkWriteDescriptorSet descriptorSampler;
    descriptorSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSampler.pNext = nullptr;
    descriptorSampler.dstSet = descriptorSet;
    descriptorSampler.dstBinding = 1;
    descriptorSampler.dstArrayElement = 0;
    descriptorSampler.descriptorCount = descriptorImageInfos.size();
    descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSampler.pImageInfo = descriptorImageInfos.data();
    descriptorSampler.pBufferInfo = nullptr;
    descriptorSampler.pTexelBufferView = nullptr;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    writeDescriptorSets.push_back(descriptorWrite);
    writeDescriptorSets.push_back(descriptorSampler);

    vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void recordCommandBuffers() {
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffers[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { width, height };
        VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depthClearValue = { 1.0f, 0.0f};

        std::vector<VkClearValue> clearValues;
        clearValues.push_back(clearValue);
        clearValues.push_back(depthClearValue);

        renderPassBeginInfo.clearValueCount = clearValues.size();
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkBool32 usePhong = VK_TRUE;
        vkCmdPushConstants(commandBuffers[i], pipeline.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(usePhong), &usePhong),

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());

        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = width / 2;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset = { 0, 0 };
        scissor.extent = { width, height };
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 1, &descriptorSet, 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], indices.size(), 1, 0, 0, 0);

        // Splitscreen
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineWireframe.getPipeline());
        viewport.x = width / 2;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);
        usePhong = VK_FALSE;
        vkCmdPushConstants(commandBuffers[i], pipelineWireframe.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(usePhong), &usePhong),
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineWireframe.getLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdDrawIndexed(commandBuffers[i], indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        result = vkEndCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result);
    }
}

void createSemaphores() {
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreImageAvailable);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreRenderingDone);
    ASSERT_VULKAN(result);
}

void startVulkan() {
    createInstance();
    physicalDevices = getAllPhysicalDevices();
    printInstanceLayers();
    printInstanceExtensions();
    createGlfwWindowSurface();
    printStatsOfAllPhysicalDevices();
    createLogicalDevice();
    createQueue();
    checkSurfaceSupport();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createPipeline();
    createCommandPool();
    createDepthImage();
    createFramebuffers();
    createCommandBuffers();
    loadTexture();
    loadMesh();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    recordCommandBuffers();
    createSemaphores();
}

void recreateSwapchain();

void onWindowResized(GLFWwindow *window, int w, int h) {
    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[0], surface, &surfaceCapabilites);
    ASSERT_VULKAN(result);

    if (w > surfaceCapabilites.maxImageExtent.width) w = surfaceCapabilites.maxImageExtent.width;
    if (h > surfaceCapabilites.maxImageExtent.height) h = surfaceCapabilites.maxImageExtent.height;

    if (w == 0 || h == 0) return; // Do nothing!

    width = w;
    height = h;
    recreateSwapchain();
}

void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);

    glfwSetWindowSizeCallback(window, onWindowResized);
}

void recreateSwapchain() {
    vkDeviceWaitIdle(device);

    depthImage.destroy();

    vkFreeCommandBuffers(device, commandPool, amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;

    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
    delete[] framebuffers;
    
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        vkDestroyImageView(device, imageViews[i], nullptr);
    }
    delete[] imageViews;

    VkSwapchainKHR oldSwapchain = swapchain;

    createSwapchain();
    createImageViews();
    createRenderPass();
    createDepthImage();
    createFramebuffers();
    createCommandBuffers();
    recordCommandBuffers();

    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
}

void drawFrame() {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);
    ASSERT_VULKAN(result);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphoreImageAvailable;
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphoreRenderingDone;

    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result);

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(queue, &presentInfo);
    ASSERT_VULKAN(result);
}

void updateMVP() {
    static auto gameStartTime = std::chrono::high_resolution_clock::now();
    auto frameTime = std::chrono::high_resolution_clock::now();

    // in seconds
    float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime - gameStartTime).count() / 1000.0f;

    glm::vec3 offset = glm::vec3(timeSinceStart * 1.0f, 0.0f, 0.0f);

    // Translate -> Scale -> Rotate
    glm::mat4 model = glm::mat4(1.0);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.2f));
    model = glm::translate(model, offset);
    //model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
    model = glm::rotate(model, timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f) + offset, glm::vec3(0.0f, 0.0f, 0.0f) + offset, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width * 0.5f / (float)height, 0.01f, 10.0f);
    projection[1][1] *= -1; // Vulkan and OpenGL y-Axis different orientation

    ubo.lightPosition = glm::vec4(offset, 0.0) + glm::rotate(glm::mat4(1.0), timeSinceStart * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0, 3, 1, 0);
    ubo.model = model;
    ubo.view = view;
    ubo.projection = projection;

    void* data;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBufferMemory);
}

void gameLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        updateMVP();

        drawFrame();
    }
}

void shutdownVulkan() {
    vkDeviceWaitIdle(device);

    depthImage.destroy();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);
    vkDestroyBuffer(device, uniformBuffer, nullptr);    

    vkFreeMemory(device, indexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);

    vkFreeMemory(device, vertexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);

    elfImage.destroy();
    dirtTexture.destroy();
    dirtNormalTexture.destroy();

    vkDestroySemaphore(device, semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(device, commandPool, amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;

    vkDestroyCommandPool(device, commandPool, nullptr);
    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
    delete[] framebuffers;

    pipeline.destroy();
    pipelineWireframe.destroy();
    
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (uint32_t i = 0; i < amountOfImagesInSwapchain; i++) {
        vkDestroyImageView(device, imageViews[i], nullptr);
    }
    delete[] imageViews;
    
    vkDestroyShaderModule(device, shaderModuleVert, nullptr);
    vkDestroyShaderModule(device, shaderModuleFrag, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void shutdownGlfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main()
{
    startGlfw();
    startVulkan();

    gameLoop();

    shutdownVulkan();
    shutdownGlfw();

    return 0;
}