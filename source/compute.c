
#include "compute.h"

#include <stdio.h>

static VkPipeline pipeline;
static VkShaderModule shader_module;

static int CreateShader() {
  FILE* f = fopen(filename.data(), "rb");

  if (!f) {
    return -1;
  }

  fseek(f, 0, SEEK_END);
  uint32_t file_size = ftell(f);
  fseek(f, 0, SEEK_SET);
  // keep alignment
  if (file_size % 4) file_size += 4 - (file_size % 4);

  uint32_t* shaderCode = malloc(file_size);
  fread(shaderCode, fileSize, 1, f);
  fclose(f);

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pCode = "";
  create_info.codeSize = 0;

  free(shaderCode);
}
static int CreatePipeline() {}

static void DestroyPipeline() {}
static void DestroyShader() {}

int CreateComputePipeline() {
  if (-1 == CreateShader()) {
    fprintf(stderr, "failed to create the shader \n");
    return -1;
  }
  if (-1 == CreatePipeline()) {
    fprintf(stderr, "failed to create the pipeline \n");
    return -1;
  }
  return 0;

  VkComputePipelineCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  create_info.layout = layout;

  VkSpecializationMapEntry map_entries[2];

  VkSpecializationInfo spec_info = {};
  spec_info.dataSize = 0;
  spec_info.pData = 0;

  spec_info.mapEntryCount = sizeof(map_entries) / sizeof(map_entries[0]);
  spec_info.pMapEntries = map_entries;

  create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  create_info.stage.pName = "main";
  create_info.stage.pSpecializationInfo = &spec_info;
  create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

  return 0;
}
