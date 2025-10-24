//
// Created by hrach on 10/23/25.
//

#include "texture_renderer.h"

void init(){

    for(int i = 0; i < M0; i++){
        for(int j = i + 1; j < M0; j++){
            graph[i].edge_list[j] = 1;
            graph[j].edge_list[i] = 1;
            vertex_pool[i]++;
            vertex_pool[j]++;
            total_degree += 2;
        }
    }
}

void add_node(int i, int j){
    graph[i].edge_list[j] = 1;
    graph[j].edge_list[i] = 1;
    vertex_pool[i]++;
    vertex_pool[j]++;
    total_degree += 2;
}

void generate(){
    for(int i = M0; i < N; i++){
        int edges_added = 0;
        while (edges_added < M){
            double p = (double)rand() / RAND_MAX;
            double cumulative = 0.0;
            for(int j = 0; j < i; j++){
                cumulative += (double)vertex_pool[j] / total_degree;
                if(p < cumulative){
                    if(graph[j].edge_list[i] == 0){
                        add_node(j, i);
                        edges_added++;
                        break;
                    }
                }
            }
        }
    }
}

void print_graph(){
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            printf("%d ", graph[i].edge_list[j]);
        }
        printf("\n");
    }
}

void textureRendererInit(TextureRenderer* renderer, VkDevice device, VkPhysicalDevice physicalDevice,
                        VkCommandPool commandPool, VkQueue graphicsQueue) {
    memset(renderer, 0, sizeof(TextureRenderer));
    renderer->device = device;
    renderer->physicalDevice = physicalDevice;
    renderer->commandPool = commandPool;
    renderer->graphicsQueue = graphicsQueue;
}

int textureRendererCreateTexture(TextureRenderer* renderer, const uint8_t* pixels, uint32_t width, uint32_t height) {
    renderer->textureWidth = width;
    renderer->textureHeight = height;
    VkDeviceSize imageSize = width * height * 4;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(renderer, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer, &stagingMemory);

    void* data;
    vkMapMemory(renderer->device, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(renderer->device, stagingMemory);

    // Create texture image
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(renderer->device, &imageInfo, NULL, &renderer->textureImage) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture image\n");
        return 0;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(renderer->device, renderer->textureImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(renderer->physicalDevice, memReqs.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(renderer->device, &allocInfo, NULL, &renderer->textureMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate texture memory\n");
        return 0;
    }

    vkBindImageMemory(renderer->device, renderer->textureImage, renderer->textureMemory, 0);

    // Transition and copy
    transitionImageLayout(renderer, renderer->textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(renderer, stagingBuffer, renderer->textureImage, width, height);
    transitionImageLayout(renderer, renderer->textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(renderer->device, stagingBuffer, NULL);
    vkFreeMemory(renderer->device, stagingMemory, NULL);

    // Create image view
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = renderer->textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(renderer->device, &viewInfo, NULL, &renderer->textureImageView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture image view\n");
        return 0;
    }

    // Create sampler
    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(renderer->device, &samplerInfo, NULL, &renderer->textureSampler) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture sampler\n");
        return 0;
    }

    return 1;
}

// Create descriptor set layout
int textureRendererCreateDescriptorSetLayout(TextureRenderer* renderer) {
    VkDescriptorSetLayoutBinding samplerBinding = {0};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(renderer->device, &layoutInfo, NULL, &renderer->descriptorSetLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        return 0;
    }

    return 1;
}

// Create descriptor set
int textureRendererCreateDescriptorSet(TextureRenderer* renderer, VkDescriptorPool descriptorPool) {
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &renderer->descriptorSetLayout;

    if (vkAllocateDescriptorSets(renderer->device, &allocInfo, &renderer->descriptorSet) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate descriptor set\n");
        return 0;
    }

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = renderer->textureImageView;
    imageInfo.sampler = renderer->textureSampler;

    VkWriteDescriptorSet descriptorWrite = {0};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = renderer->descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(renderer->device, 1, &descriptorWrite, 0, NULL);

    return 1;
}

// Create vertex buffer for fullscreen quad
int textureRendererCreateVertexBuffer(TextureRenderer* renderer) {
    Vertex vertices[] = {
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 1.0f}}
    };

    uint16_t indices[] = {0, 1, 2, 2, 3, 0};
    renderer->indexCount = 6;

    VkDeviceSize vertexBufferSize = sizeof(vertices);
    VkDeviceSize indexBufferSize = sizeof(indices);

    // Create vertex buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(renderer, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer, &stagingMemory);

    void* data;
    vkMapMemory(renderer->device, stagingMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices, vertexBufferSize);
    vkUnmapMemory(renderer->device, stagingMemory);

    createBuffer(renderer, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->vertexBuffer, &renderer->vertexBufferMemory);

    copyBuffer(renderer, stagingBuffer, renderer->vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(renderer->device, stagingBuffer, NULL);
    vkFreeMemory(renderer->device, stagingMemory, NULL);

    // Create index buffer
    createBuffer(renderer, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer, &stagingMemory);

    vkMapMemory(renderer->device, stagingMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices, indexBufferSize);
    vkUnmapMemory(renderer->device, stagingMemory);

    createBuffer(renderer, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->indexBuffer, &renderer->indexBufferMemory);

    copyBuffer(renderer, stagingBuffer, renderer->indexBuffer, indexBufferSize);

    vkDestroyBuffer(renderer->device, stagingBuffer, NULL);
    vkFreeMemory(renderer->device, stagingMemory, NULL);

    return 1;
}

// Render the texture
void textureRendererRender(TextureRenderer* renderer, VkCommandBuffer commandBuffer,
                          VkPipeline pipeline, VkPipelineLayout pipelineLayout) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkBuffer vertexBuffers[] = {renderer->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, renderer->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout, 0, 1, &renderer->descriptorSet, 0, NULL);

    vkCmdDrawIndexed(commandBuffer, renderer->indexCount, 1, 0, 0, 0);
}

// Get descriptor set layout
VkDescriptorSetLayout textureRendererGetDescriptorSetLayout(TextureRenderer* renderer) {
    return renderer->descriptorSetLayout;
}

// Cleanup
void textureRendererDestroy(TextureRenderer* renderer) {
    if (renderer->textureSampler)
        vkDestroySampler(renderer->device, renderer->textureSampler, NULL);
    if (renderer->textureImageView)
        vkDestroyImageView(renderer->device, renderer->textureImageView, NULL);
    if (renderer->textureImage)
        vkDestroyImage(renderer->device, renderer->textureImage, NULL);
    if (renderer->textureMemory)
        vkFreeMemory(renderer->device, renderer->textureMemory, NULL);
    if (renderer->vertexBuffer)
        vkDestroyBuffer(renderer->device, renderer->vertexBuffer, NULL);
    if (renderer->vertexBufferMemory)
        vkFreeMemory(renderer->device, renderer->vertexBufferMemory, NULL);
    if (renderer->indexBuffer)
        vkDestroyBuffer(renderer->device, renderer->indexBuffer, NULL);
    if (renderer->indexBufferMemory)
        vkFreeMemory(renderer->device, renderer->indexBufferMemory, NULL);
    if (renderer->descriptorSetLayout)
        vkDestroyDescriptorSetLayout(renderer->device, renderer->descriptorSetLayout, NULL);
}

// Helper functions

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type\n");
    return 0;
}

static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

static void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void createBuffer(TextureRenderer* renderer, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* memory) {
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(renderer->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create buffer\n");
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(renderer->device, *buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(renderer->physicalDevice, memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(renderer->device, &allocInfo, NULL, memory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate buffer memory\n");
        return;
    }

    vkBindBufferMemory(renderer->device, *buffer, *memory, 0);
}

static void copyBuffer(TextureRenderer* renderer, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderer->device, renderer->commandPool);

    VkBufferCopy copyRegion = {0};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    endSingleTimeCommands(renderer->device, renderer->commandPool, renderer->graphicsQueue, commandBuffer);
}

static void copyBufferToImage(TextureRenderer* renderer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderer->device, renderer->commandPool);

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(renderer->device, renderer->commandPool, renderer->graphicsQueue, commandBuffer);
}

static void transitionImageLayout(TextureRenderer* renderer, VkImage image, VkFormat format,
                                 VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderer->device, renderer->commandPool);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        fprintf(stderr, "Unsupported layout transition\n");
        return;
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);

    endSingleTimeCommands(renderer->device, renderer->commandPool, renderer->graphicsQueue, commandBuffer);
}

// Vertex input binding description
VkVertexInputBindingDescription getVertexBindingDescription(void) {
    VkVertexInputBindingDescription bindingDescription = {0};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

// Vertex input attribute descriptions
void getVertexAttributeDescriptions(VkVertexInputAttributeDescription* attributeDescriptions) {
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, tex_coord);
}