//
// Created by hrach on 10/23/25.
//

#ifndef CS226FINALPROJECT_TEXTURE_RENDERER_H
#define CS226FINALPROJECT_TEXTURE_RENDERER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#define N 15
#define M0 4
#define M 4

static int vertex_pool[N] = {0};

typedef struct {
    int edge_list[N];
    float tex_coord[2];
    float pos[2];
} Vertex;

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    VkImage textureImage;
    VkDeviceMemory textureMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    uint32_t textureWidth;
    uint32_t textureHeight;
    uint32_t indexCount;
} TextureRenderer;

static Vertex graph[N];
static int total_degree = 0;

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
static void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);
static void createBuffer(TextureRenderer* renderer, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* memory);
static void copyBuffer(TextureRenderer* renderer, VkBuffer src, VkBuffer dst, VkDeviceSize size);
static void copyBufferToImage(TextureRenderer* renderer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
static void transitionImageLayout(TextureRenderer* renderer, VkImage image, VkFormat format,
                                 VkImageLayout oldLayout, VkImageLayout newLayout);
void textureRendererInit(TextureRenderer* renderer, VkDevice device, VkPhysicalDevice physicalDevice,
                        VkCommandPool commandPool, VkQueue graphicsQueue);
int textureRendererCreateTexture(TextureRenderer* renderer, const uint8_t* pixels, uint32_t width, uint32_t height);
int textureRendererCreateDescriptorSetLayout(TextureRenderer* renderer);
int textureRendererCreateDescriptorSet(TextureRenderer* renderer, VkDescriptorPool descriptorPool);
int textureRendererCreateVertexBuffer(TextureRenderer* renderer);
void textureRendererRender(TextureRenderer* renderer, VkCommandBuffer commandBuffer,
                          VkPipeline pipeline, VkPipelineLayout pipelineLayout);
VkDescriptorSetLayout textureRendererGetDescriptorSetLayout(TextureRenderer* renderer);
void textureRendererDestroy(TextureRenderer* renderer);
static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void getVertexAttributeDescriptions(VkVertexInputAttributeDescription* attributeDescriptions);


void init();
void add_Vertex(int i, int j);
void generate();
void print_graph();
void createTexture(const uint8_t* pixels, uint32_t width, uint32_t height);

#endif //CS226FINALPROJECT_TEXTURE_RENDERER_H