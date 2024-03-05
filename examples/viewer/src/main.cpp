#include <stdio.h>
#include <vulkan/vulkan_core.h>
#include <iostream>

#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "mesh/Vertex.hpp"
#include "SDLWindow.h"
#include "Swapchain.hpp"
#include "resources/DefaultAllocator.h"
#include "shaders/SpirvShaderBuilder.hpp"
#include "shaders/ShaderInputSetLayoutBuilder.hpp"

#include "mesh/DrawMeshInfo.hpp"
#include "shaders/SpirvShaderBuilder.hpp"
#include "io/gltfSceneLoader.hpp"
#include "mesh/SceneBuffer.hpp"
#include "io/path.hpp"
#include "shaders/Pipeline.hpp"
#include "shaders/PipelineBuilder.h"
#include "runtime/Camera.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "RenderGraph.hpp"
#include "RenderPass.hpp"
#include "RenderGraphBuilder.h"
#include "FramebufferBuilder.hpp"
#include "RenderGraph.hpp"
#include "scene/Light.h"
#include "runtime/FrameLock.h"

struct MeshInfo {
  uint32_t materialIdx;
  uint32_t modelIdx;
};

struct ShaderBucket {
  VkPipeline pipeline;
  std::vector<DrawMeshInfo> meshes;
};

struct MeshPassContext {
  std::vector<ShaderBucket> shaderBuckets;

  std::vector<MeshInfo> meshInfo;
  std::vector<DrawMeshInfo> meshes;

  Pipeline pipeline;
  Pipeline blendPipeline;

  SceneBuffer *pBuffer;
  VkDescriptorSet sceneSet;

  /*
   * Creates new mesh pass context with null pipeline.
   * Pipeline must be set afterward manually
   */
  MeshPassContext() :
	  pipeline(nullptr, nullptr), blendPipeline(nullptr, nullptr) {

  }
};


struct ShadowPassContext {
  Pipeline pipeline;
  SceneBuffer *pBuffer;
  VkDescriptorSet sceneSet;

  ShadowPassContext() :
          pipeline(nullptr, nullptr) {

  }
};


struct ShadingPassContext {
  Pipeline pipeline;
  VkDescriptorSet sceneSet;

  VkDescriptorSetLayout perPassInputSetLayout;
  std::vector<VkDescriptorSet> descriptorSets;

  ShadingPassContext() :
          pipeline(nullptr, nullptr) {

  }
};



struct GeometryContext {
    std::vector<VkDescriptorSet> inputSets;

    Pipeline pipeline;
    SceneBuffer *pSceneBuffer;

    VkDescriptorSet sceneInputSet;
    VkPipelineLayout layout;

    GeometryContext() :
        pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE),
        pSceneBuffer(nullptr) {

    }
};

struct ImGuiContext {
    VkRenderPass rp;
};

struct DownsampleContext {
    std::vector<VkDescriptorSet> inputSets;
    std::string attachmentName;

    Pipeline pipeline;

    VkExtent2D extent;

    std::string source;

    DownsampleContext() :
    pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE) {

    }
};

struct UpsampleContext {
    std::vector<VkDescriptorSet> inputSets;
    std::string attachmentName;

    Pipeline pipeline;

    VkExtent2D extent;

    std::string source;
    std::string blur;

    UpsampleContext() :
            pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE) {

    }
};

struct CompositionContext {
    std::vector<VkDescriptorSet> inputSets;
    Pipeline pipeline;

    CompositionContext() :
    pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE) {

    }
};

int
main(int32_t argc, char** argv) {
    /**
     * The program needs some glTF file to render
     */
    if(argc <= 1) {
        fprintf(stderr, "Specify path to glTF file");
        return 0;
    }

    VkExtent2D extent = {
            .width = 1200,
            .height = 800
    };

    /**
     * Sets the executable path to find shaders and assets
     */
    io::path::setup_exe_path(argv[0]);

    /**
     * Opens up a window
     */
    Window *window = new SDLWindow("loft", (VkRect2D) {
            0, 0,
            extent.width, extent.height
    });

    /**
     * Different platforms has different extension needs.
     */
    uint32_t count = 0;
    window->get_required_extensions(&count, nullptr);
    char** extensions = new char*[count];
    window->get_required_extensions(&count, extensions);

    /**
     * Instance initializes a connection with Vulkan driver
     */
    Instance instance("loft", "loft", count, extensions);
    delete [] extensions;

    /*
     * Surface is a way to tell window:
     * Hey, I am going to render to you from GPU. I need some surface to render to.
     */
    auto surface = window->create_surface(instance.instance());

    /**
     * Gpu manages stuff around rendering. Needed for most graphics operations.
     */
    Gpu gpu = Gpu(instance, surface);
    Camera camera(&gpu);

    /**
     * Swapchain is a queue storage of images to render to for the Window we opened.
     */
    Swapchain swapchain = Swapchain(&gpu, surface);


    /**
     * Creates new frame graph.
     * Frame graph manages renderpass dependencies and synchronization.
     */
    RenderGraphBuilder renderGraphBuilder(extent);


	auto scene = GltfSceneLoader().from_file(argv[1]);
	auto sceneBuffer = new SceneBuffer(&gpu, &scene);

	/*
	 * Create pipelines
	 * Load and compile shaders
	 */
	SpirvShaderBuilder shaderBuilder(&gpu);
	auto vert = shaderBuilder.from_file(io::path::shader("Opaque.vert.spirv"));
	auto frag = shaderBuilder.from_file(io::path::shader("Opaque.frag.spirv"));
	auto blendFrag = shaderBuilder.from_file(io::path::shader("Transparent.frag.spirv"));

	auto offscr = shaderBuilder.from_file(io::path::shader("Offscreen.vert.spirv"));
	auto shade = shaderBuilder.from_file(io::path::shader("Shading.frag.spirv"));

	auto shadowVert = shaderBuilder.from_file(io::path::shader("Shadow.vert.spirv"));
	auto shadowFrag = shaderBuilder.from_file(io::path::shader("Shadow.frag.spirv"));

	auto globalInputSetLayout = ShaderInputSetLayoutBuilder(1)
            .uniform_buffer(0)
			.build(&gpu);

    auto globalInputSet = ShaderInputSetBuilder(1)
            .buffer(0, camera.buffer(), 0, sizeof(mat4) * 2 + sizeof(vec4))
            .build(&gpu, globalInputSetLayout);

    auto geometryContext = new GeometryContext();
    geometryContext->pSceneBuffer = sceneBuffer;

    /*
     * Geometry Pass
     * Draws information about screen space geometry into GBuffer for post processing.
     */
    auto geometry = LambdaRenderPass<GeometryContext>("geometry", geometryContext,
        [&](GeometryContext* pContext, RenderPassBuildInfo info) {
            /* create layout */
            auto perPassInputLayout = ShaderInputSetLayoutBuilder(0)
                    .build(info.gpu());

            pContext->inputSets.resize(info.num_images_in_flights());
            /* create input set */
            for(uint32_t i = 0; i < info.num_images_in_flights(); i++) {
                pContext->inputSets[i] = ShaderInputSetBuilder(0)
                    .build(info.gpu(), perPassInputLayout);
            }

            auto sceneInputSetLayout = ShaderInputSetLayoutBuilder(4)
                    .binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    /* color texture */
                    .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    /* normal texture */
                    .binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    /* pbr texture */
                    .binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(info.gpu());

            pContext->sceneInputSet = ShaderInputSetBuilder(4)
                    .buffer(0, pContext->pSceneBuffer->m_materialBuffer.m_materials, 0, pContext->pSceneBuffer->m_materialBuffer.material_buffer_size())
                    .image(1, pContext->pSceneBuffer->m_materialBuffer.color_texture(), pContext->pSceneBuffer->m_materialBuffer.sampler())
                    .image(2, pContext->pSceneBuffer->m_materialBuffer.normal_texture(), pContext->pSceneBuffer->m_materialBuffer.sampler())
                    .image(3, pContext->pSceneBuffer->m_materialBuffer.pbr_texture(), pContext->pSceneBuffer->m_materialBuffer.sampler())
                    .build(info.gpu(), sceneInputSetLayout);

            pContext->layout = PipelineLayoutBuilder()
                    .push_constant_range(0, sizeof(uint32_t) * 2, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                    .input_set(0, globalInputSetLayout)
                    .input_set(1, perPassInputLayout)
                    .input_set(2, sceneInputSetLayout)
                    .build(info.gpu());

            pContext->pipeline = PipelineBuilder(info.gpu(), info.output().viewport(),
                                                  pContext->layout, info.output().renderpass(),
                                                  4, vert, frag)
                    .set_vertex_input_info(Vertex::bindings(), Vertex::attributes())
                    .build()
                    .value();
        },
        [&](GeometryContext* pContext, RenderPassRecordInfo info) {
                vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipeline.pipeline());
                vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipeline.pipeline_layout(),
                                        0, 1, &globalInputSet, 0, nullptr);

                vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipeline.pipeline_layout(),
                                        2, 1, &pContext->sceneInputSet, 0, nullptr);

                pContext->pSceneBuffer->draw_opaque(info.command_buffer(), pContext->layout);
        });
    geometry.add_color_output("col_gbuf", VK_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.0f, 0.0f, 1.0f })
            .add_color_output("norm_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT, { 0.0f, 0.0f, 0.0f, 1.0f })
            .add_color_output("pos_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT, { 0.0f, 0.0f, 0.0f, 1.0f })
            .add_color_output("pbr_gbuf", VK_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.0f, 0.0f, 1.0f })
            .set_depth_output("depth_gbuf", VK_FORMAT_D32_SFLOAT_S8_UINT, {1.0f, 0});




    /**
     * Shading.
     * Takes gbuffer and lights as inputs and calculates how it interacts with the scene.
     * Deferred approach.
     */
    auto shadingContext = new ShadingPassContext();
    auto shading = LambdaRenderPass<ShadingPassContext>("shading", shadingContext,
                                                        [&](ShadingPassContext* pContext, RenderPassBuildInfo info) {
            pContext->perPassInputSetLayout = ShaderInputSetLayoutBuilder(4)
                    .binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(info.gpu());

            pContext->descriptorSets.resize(info.num_images_in_flights());
            /* create input set */
            for(uint32_t i = 0; i < info.num_images_in_flights(); i++) {
                pContext->descriptorSets[i] = ShaderInputSetBuilder(4)
                        .image(0, info.get_image("col_gbuf", i)->view, info.sampler())
                        .image(1, info.get_image("norm_gbuf", i)->view, info.sampler())
                        .image(2, info.get_image("pos_gbuf", i)->view, info.sampler())
                        .image(3, info.get_image("pbr_gbuf", i)->view, info.sampler())
                        .build(info.gpu(), pContext->perPassInputSetLayout);
            }

            auto layout = PipelineLayoutBuilder()
                    .input_set(0, globalInputSetLayout)
                    .input_set(1, pContext->perPassInputSetLayout)
                    .build(info.gpu());

            pContext->pipeline = PipelineBuilder(info.gpu(), info.output().viewport(),
                                                 layout, info.output().renderpass(),
                                                 2, offscr, shade)
                    .set_vertex_input_info({}, {})
                    .build()
                    .value();
    }, [&](ShadingPassContext* pContext, RenderPassRecordInfo recordInfo) {
            vkCmdBindPipeline(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipeline.pipeline());
            vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipeline.pipeline_layout(),
                                    0, 1, &globalInputSet, 0, nullptr);

            vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipeline.pipeline_layout(),
                                    1, 1, &pContext->descriptorSets[recordInfo.image_idx()], 0, nullptr);

            vkCmdDraw(recordInfo.command_buffer(), 3, 1, 0, 0);
    });
    shading.add_color_output("hdr", VK_FORMAT_R32G32B32A32_SFLOAT)
           .add_color_output("bloom_threshold", VK_FORMAT_R32G32B32A32_SFLOAT)
           .add_input("col_gbuf")
           .add_input("norm_gbuf");



    uint32_t numIters = 6;
    assert(numIters > 0);

    std::vector<LambdaRenderPass<DownsampleContext>*> downsamples(numIters);
    auto downsamplePerPassInputSetLayout = ShaderInputSetLayoutBuilder(4)
            .binding(0,
                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     1, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(&gpu);

    std::vector<LambdaRenderPass<UpsampleContext>*> upsamples(numIters);
    auto upsamplePerPassInputSetLayout = ShaderInputSetLayoutBuilder(4)
            .binding(0,
                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     1, VK_SHADER_STAGE_FRAGMENT_BIT) /* blur */
            .binding(1,
                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     1, VK_SHADER_STAGE_FRAGMENT_BIT) /* upsampling */
            .build(&gpu);

    auto bloomShader = shaderBuilder.from_file(io::path::shader("Bloom.frag.spirv"));
    auto upscaleShader = shaderBuilder.from_file(io::path::shader("Upsample.frag.spirv"));
    std::string previous = "bloom_threshold";

    for(uint32_t i = 0; i < numIters; i++) {
        /* create downscale pass */
        {
            auto downsampleContext = new DownsampleContext();
            downsampleContext->extent.width = extent.width >> i;
            downsampleContext->extent.height = extent.height >> i;

            downsampleContext->attachmentName = std::string("downsample_").append(std::to_string(i));
            downsampleContext->source = std::string(previous);

            downsamples[i] = new LambdaRenderPass<DownsampleContext>(std::string("downscale_").append(std::to_string(i)), downsampleContext,
                                                                         [&](DownsampleContext *pContext, RenderPassBuildInfo info) {
                                                                             pContext->inputSets.resize(info.num_images_in_flights());

                                                                             /* create input set */
                                                                             for (uint32_t i = 0; i < info.num_images_in_flights(); i++) {
                                                                                 pContext->inputSets[i] = ShaderInputSetBuilder(1)
                                                                                         .image(0, info.get_image(pContext->source, i)->view, info.sampler())
                                                                                         .build(info.gpu(), downsamplePerPassInputSetLayout);
                                                                             }

                                                                             auto layout = PipelineLayoutBuilder()
                                                                                     .input_set(0, globalInputSetLayout)
                                                                                     .input_set(1, downsamplePerPassInputSetLayout)
                                                                                     .push_constant_range(0, sizeof(vec2), VK_SHADER_STAGE_FRAGMENT_BIT)
                                                                                     .build(info.gpu());

                                                                             pContext->pipeline = PipelineBuilder(info.gpu(),
                                                                                                                  info.output().viewport(),
                                                                                                                  layout,
                                                                                                                  info.output().renderpass(),
                                                                                                                  1, offscr, bloomShader)
                                                                                     .set_vertex_input_info({}, {})
                                                                                     .build()
                                                                                     .value();
                                                                         },
                                                                         [&](DownsampleContext *pContext, RenderPassRecordInfo recordInfo) {
                                                                             vkCmdBindPipeline(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                               pContext->pipeline.pipeline());
                                                                             vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                                     pContext->pipeline.pipeline_layout(),
                                                                                                     0, 1, &globalInputSet, 0, nullptr);

                                                                             vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                                     pContext->pipeline.pipeline_layout(),
                                                                                                     1, 1, &pContext->inputSets[recordInfo.image_idx()], 0, nullptr);

                                                                             vkCmdPushConstants(recordInfo.command_buffer(), pContext->pipeline.pipeline_layout(), VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                                                0, sizeof(VkExtent2D), &pContext->extent);

                                                                             vkCmdDraw(recordInfo.command_buffer(), 3, 1, 0, 0);
                                                                         });

            downsamples[i]->add_color_output(downsampleContext->attachmentName, VK_FORMAT_R32G32B32A32_SFLOAT)
                    .add_input(previous)
                    .set_extent(downsampleContext->extent);

            previous = downsampleContext->attachmentName;

            renderGraphBuilder.add_graphics_pass(downsamples[i]);
        }
    }

    for(uint32_t i = 0; i < numIters - 1; i++) {
        auto upsampleContext = new UpsampleContext();
        upsampleContext->extent.width = extent.width >> (numIters - i - 1);
        upsampleContext->extent.height = extent.height >> (numIters - i - 1);
        upsampleContext->attachmentName = std::string("upsample_").append(std::to_string(i));
        upsampleContext->source = std::string("downsample_").append(std::to_string(numIters - i - 2));
        upsampleContext->blur = std::string(previous);

        upsamples[i] = new LambdaRenderPass<UpsampleContext>(std::string("upscale_").append(std::to_string(i)), upsampleContext,
                                                             [&](UpsampleContext *pContext, RenderPassBuildInfo info) {
                                                                 pContext->inputSets.resize(info.num_images_in_flights());

                                                                 /* create input set */
                                                                 for (uint32_t i = 0; i < info.num_images_in_flights(); i++) {
                                                                     pContext->inputSets[i] = ShaderInputSetBuilder(2)
                                                                             .image(0, info.get_image(pContext->source, i)->view, info.sampler())
                                                                             .image(1, info.get_image(pContext->blur, i)->view, info.sampler())
                                                                             .build(info.gpu(), upsamplePerPassInputSetLayout);
                                                                 }

                                                                 auto layout = PipelineLayoutBuilder()
                                                                         .input_set(0, globalInputSetLayout)
                                                                         .input_set(1, upsamplePerPassInputSetLayout)
                                                                         .push_constant_range(0, sizeof(vec2), VK_SHADER_STAGE_FRAGMENT_BIT)
                                                                         .build(info.gpu());

                                                                 pContext->pipeline = PipelineBuilder(info.gpu(),
                                                                                                      info.output().viewport(),
                                                                                                      layout,
                                                                                                      info.output().renderpass(),
                                                                                                      1, offscr, upscaleShader)
                                                                         .set_vertex_input_info({}, {})
                                                                         .build()
                                                                         .value();
                                                             },
                                                             [&](UpsampleContext *pContext, RenderPassRecordInfo recordInfo) {
                                                                 vkCmdBindPipeline(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                   pContext->pipeline.pipeline());
                                                                 vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                         pContext->pipeline.pipeline_layout(),
                                                                                         0, 1, &globalInputSet, 0, nullptr);

                                                                 vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                                         pContext->pipeline.pipeline_layout(),
                                                                                         1, 1, &pContext->inputSets[recordInfo.image_idx()], 0, nullptr);

                                                                 vkCmdPushConstants(recordInfo.command_buffer(), pContext->pipeline.pipeline_layout(), VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                                    0, sizeof(VkExtent2D), &pContext->extent);

                                                                 vkCmdDraw(recordInfo.command_buffer(), 3, 1, 0, 0);
                                                             });

        upsamples[i]->add_color_output(upsampleContext->attachmentName, VK_FORMAT_R32G32B32A32_SFLOAT)
                .add_input(upsampleContext->blur)
                .add_input(upsampleContext->source)
                .set_extent(upsampleContext->extent);

        previous = upsampleContext->attachmentName;

        renderGraphBuilder.add_graphics_pass(upsamples[i]);
    }


    /*
     * Composition
     */
    auto compositeShader = shaderBuilder.from_file(io::path::shader("Composite.frag.spirv"));

    auto compositionContext = CompositionContext();
    auto compositionPass = LambdaRenderPass<CompositionContext>("composition", &compositionContext,
        [&](CompositionContext* pContext, RenderPassBuildInfo info) {
            pContext->inputSets.resize(info.num_images_in_flights());

            /* create input set */
            for (uint32_t i = 0; i < info.num_images_in_flights(); i++) {
                pContext->inputSets[i] = ShaderInputSetBuilder(2)
                        .image(0, info.get_image("hdr", i)->view, info.sampler())
                        .image(1, info.get_image(previous, i)->view, info.sampler())
                        .build(info.gpu(), upsamplePerPassInputSetLayout);
            }

            auto layout = PipelineLayoutBuilder()
                    .input_set(0, globalInputSetLayout)
                    .input_set(1, upsamplePerPassInputSetLayout)
                    .push_constant_range(0, sizeof(vec2), VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(info.gpu());

            pContext->pipeline = PipelineBuilder(info.gpu(),
                                                 info.output().viewport(),
                                                 layout,
                                                 info.output().renderpass(),
                                                 1, offscr, compositeShader)
                    .set_vertex_input_info({}, {})
                    .build()
                    .value();
        },
        [&](CompositionContext* pContext, RenderPassRecordInfo recordInfo) {
            vkCmdBindPipeline(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pContext->pipeline.pipeline());
            vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pContext->pipeline.pipeline_layout(),
                                    0, 1, &globalInputSet, 0, nullptr);

            vkCmdBindDescriptorSets(recordInfo.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pContext->pipeline.pipeline_layout(),
                                    1, 1, &pContext->inputSets[recordInfo.image_idx()], 0, nullptr);

            vkCmdDraw(recordInfo.command_buffer(), 3, 1, 0, 0);
        }
    );
    compositionPass
        .add_input("hdr")
        .add_input(previous)
        .add_color_output("swapchain", swapchain.format().format, {0.0f, 0.0f, 0.0f, 0.0f});

    renderGraphBuilder.add_graphics_pass(&compositionPass);

    auto imguiPassContext = ImGuiContext();
    auto imguiPass = LambdaRenderPass<ImGuiContext>("shading", &imguiPassContext,
        [&](ImGuiContext* pContext, RenderPassBuildInfo info) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.Fonts->AddFontFromFileTTF(io::path::asset("fonts/ProggyClean.ttf").c_str(), 16.0f);

            ImGui_ImplSDL2_InitForVulkan(((SDLWindow*)window)->get_handle());
            ImGui_ImplVulkan_InitInfo init_info = {
                    .Instance = instance.instance(),
                    .PhysicalDevice = gpu.gpu(),
                    .Device = gpu.dev(),
                    .QueueFamily = 0,
                    .Queue = gpu.graphics_queue(),
                    .PipelineCache = nullptr,
                    .DescriptorPool = gpu.descriptor_pool(),
                    .Subpass = 0,
                    .MinImageCount = 2,
                    .ImageCount = 2,
                    .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
                    .Allocator = nullptr,
                    .CheckVkResultFn = nullptr,
            };
            ImGui_ImplVulkan_Init(&init_info, info.output().renderpass());
        },
        [&](ImGuiContext* pContext, RenderPassRecordInfo recordInfo) {
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(draw_data, recordInfo.command_buffer());
        }
    );
    imguiPass.add_color_output("imgui", swapchain.format().format, {0.0f, 0.0f, 0.0f, 0.0f});

    auto renderGraph = renderGraphBuilder
            .add_graphics_pass(&geometry)
            .add_graphics_pass(&shading)
            .add_graphics_pass(&imguiPass)
            .build(&gpu, &swapchain);

    renderGraph.print_dot(stdout);

    /* Create lights for the scene */
    vec3 position = {20.0f, 200.0f, -30.0f};
    vec3 direction = {10.0f, 10.0f, 10.0f};
    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<Light> lights = {
            Light::directional(1.0f, 1000.0f, position, direction, color)
    };

    BufferCreateInfo lightInfoBuffer = {
            .size = sizeof(Light),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };
    MemoryAllocationInfo memoryInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_HOST
    };
    Buffer lightBuffer;
    gpu.memory()->create_buffer(&lightInfoBuffer, &memoryInfo, &lightBuffer);


    void *pPtr;
    gpu.memory()->map(lightBuffer.allocation, &pPtr);
    memcpy(pPtr, lights.data(), lightInfoBuffer.size);
    gpu.memory()->unmap(lightBuffer.allocation);


    /* Prepare ImGui */



    bool isOpen = true;
    vec3 velocity = {0.0f, 0.0f, 0.0f};

    FrameLock frameLock(60);
    while(isOpen) {
        frameLock.update();

        SDL_Event event;

        /* ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Frame stats");
        ImGui::Text("DeltaTime in ns: %ld", deltaTime);
        ImGui::Text("FPS: %ld", 1000000000UL / deltaTime);
        ImGui::End(); */


        while(window->poll_event(&event)) {
            // ImGui_ImplSDL2_ProcessEvent(&event);
            switch(event.type) {
            case SDL_QUIT:
                isOpen = false;
                break;
            case SDL_FINGERMOTION:
                camera.rotate(event.tfinger.dx * 10.0f, event.tfinger.dy * 10.0f);
                break;
            case SDL_MOUSEMOTION:
                camera.rotate(event.motion.xrel * 0.2f, event.motion.yrel * 0.2f);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                switch(event.key.keysym.sym) {
                case SDLK_w:
                    velocity[1] = event.key.state;
                    break;
                case SDLK_s:
                    velocity[1] = -event.key.state;
                    break;
                case SDLK_a:
                    velocity[0] = event.key.state;
                    break;
                case SDLK_d:
                    velocity[0] = -event.key.state;
                    break;
                }
            }
        }

        renderGraph.run();

        camera.move(velocity);
        camera.update();
    }

    return 0;
}
