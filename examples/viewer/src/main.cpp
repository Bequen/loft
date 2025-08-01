
#include <random>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>

#include <volk.h>

#include "Instance.hpp"
#include "RenderPass.hpp"
#include "Swapchain.hpp"
#include "SDLWindow.h"
#include "RenderGraphBuilder.hpp"

#include "cglm/vec3.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl2.h"
#include "cglm/types.h"
#include "io/gltfSceneLoader.hpp"
#include "io/path.hpp"
#include "mesh/runtime/Scene.h"
#include "resources/GpuAllocation.h"
#include "resources/GpuAllocator.h"
#include "runtime/Camera.h"
#include "scene/Light.h"
#include "shaders/Pipeline.hpp"
#include "shaders/PipelineBuilder.h"
#include "shaders/ShaderInputSetLayoutBuilder.hpp"
#include "shaders/SpirvShaderBuilder.hpp"

void lft_dbg_callback(lft::dbg::LogMessageSeverity severity,
                      lft::dbg::LogMessageType type,
                      const char *__restrict format,
                      va_list args) {
    const char* titles[3] = {
            "\033[0;34m[info]:",
            "\033[0;33m[warn]:",
            "\033[0;31m[fail]:"
    };
    fwrite(titles[severity], 14, 1, stdout);

    if(args != nullptr) {
        vfprintf(stdout, format, args);
    } else {
        fprintf(stdout, format);
    }

    fwrite("\033[0m", 4, 1, stdout);

    printf("\n");
}

struct GBufferContext {
	VkPipelineLayout layout;
	Pipeline pipeline;

	VkDescriptorSetLayout input_layout;
	VkDescriptorSetLayout scene_input_layout;
	VkDescriptorSet* global_input_set;

	Scene *scene;

	GBufferContext() :
		layout(VK_NULL_HANDLE),
		pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE) {
	}
};

struct ShadingContext {
	VkDescriptorSetLayout shading_set_layout;
	VkPipelineLayout layout;
	Pipeline pipeline;

	VkSampler sampler;

	VkDescriptorSet* global_input_set;
	std::vector<VkDescriptorSet> input_sets;

	ShadingContext() :
		layout(VK_NULL_HANDLE),
		pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE) {
	}
};

struct ParticleContext {
	VkDescriptorSetLayout compute_input_set_layout;
	VkPipelineLayout compute_pipeline_layout;
	VkPipeline compute_pipeline;

	VkDescriptorSetLayout draw_input_set_layout;
	VkPipelineLayout draw_pipeline_layout;
	VkPipeline draw_pipeline;

	VkDescriptorSet* global_input_set;
	std::vector<VkDescriptorSet> compute_input_sets;
	std::vector<VkDescriptorSet> draw_input_sets;

	ParticleContext() :
        draw_pipeline(VK_NULL_HANDLE) {
	}
};

struct ImGuiContext {
    VkRenderPass rp = VK_NULL_HANDLE;
    bool is_initialized = false;
};

int main(int argc, char** argv) {
	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    /**
     * Sets the executable path to find shaders and assets
     */
    io::path::setup_exe_path(argv[0]);

    /**
     * Opens up a window
     */
    std::shared_ptr<Window> window = std::make_shared<SDLWindow>(SDLWindow("loft", {
            0, 0,
            extent.width, extent.height
    }));

    /**
     * Different platforms has different extension needs.
     */
    uint32_t count = 0;
    window->get_required_extensions(&count, nullptr);
    std::vector<const char*> extensions(count);
    window->get_required_extensions(&count, extensions.data());

    /**
     * Instance initializes a connection with Vulkan driver
     */
    auto instance = std::make_shared<const Instance>(
			"loft", "loft",
			extensions,
			std::vector<const char*>(),
			lft_dbg_callback);

    volkLoadInstance(instance->instance());


    auto gpus = instance->list_physical_devices();
    for(auto gpu : gpus) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(gpu, &props);
        std::cout << "GPU: " << props.deviceName << std::endl;
	}

    /*
     * Surface is a way to tell window:
     * Hey, I am going to render to you from GPU. I need some surface to render to.
     */
    auto surface = window->create_surface(instance->instance());

    /**
     * Gpu manages stuff around rendering. Needed for most graphics operations.
     */
    auto gpu = std::make_shared<Gpu>(instance, surface);
    Camera camera(gpu, extent.width / extent.height);

    /**
     * Swapchain is a queue storage of images to render to for the Window we opened.
     */
    Swapchain swapchain = Swapchain(gpu, extent, surface);


    auto global_input_set_layout = ShaderInputSetLayoutBuilder(1)
            .uniform_buffer(0)
            .build(gpu);

    auto global_input_set = ShaderInputSetBuilder(1)
            .buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, camera.buffer(), 0, sizeof(mat4) * 2 + sizeof(vec4))
            .build(gpu, global_input_set_layout);

	std::vector<VkDescriptorSet> input_sets(1);

    auto sceneData = GltfSceneLoader().from_file(argv[1]);
	Scene scene(gpu, &sceneData);

    vec3 position = {20.0f, 250.0f, 50.0f};
    vec3 direction = {0.0f, 0.0f, 0.0f};
    vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<Light> lights = {
            Light::directional(1.0f, 1000.0f, position, direction, color)
    };

    MemoryAllocationInfo memoryInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    BufferCreateInfo lightInfoBuffer = {
            .size = sizeof(Light),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };

    Buffer light_buffer;
    gpu->memory()->create_buffer(&lightInfoBuffer, &memoryInfo, &light_buffer);

    void *pPtr;
    gpu->memory()->map(light_buffer.allocation, &pPtr);
    memcpy(pPtr, lights.data(), lightInfoBuffer.size);
    gpu->memory()->unmap(light_buffer.allocation);

    SpirvShaderBuilder shaderBuilder(gpu);
    auto vert = shaderBuilder.from_file(io::path::shader("Opaque.vert.spirv"));
    auto frag = shaderBuilder.from_file(io::path::shader("Opaque.frag.spirv"));

    auto offscr = shaderBuilder.from_file(io::path::shader("Offscreen.vert.spirv"));
    auto shade = shaderBuilder.from_file(io::path::shader("Shading.frag.spirv"));

	auto particle_shader = shaderBuilder.from_file(io::path::shader("Particles.comp.spirv"));
	auto particle_vert = shaderBuilder.from_file(io::path::shader("ParticleDraw.vert.spirv"));
	auto particle_frag = shaderBuilder.from_file(io::path::shader("ParticleDraw.frag.spirv"));

	VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
    };

    VkSampler sampler = VK_NULL_HANDLE;
    if(vkCreateSampler(gpu->dev(), &samplerInfo, nullptr, &sampler)) {
        throw std::runtime_error("failed to create sampler");
    }


	lft::rg::Builder builder(gpu, ImageChain::from_swapchain(swapchain), "swapchain");

	// keeps all the drawn images -> to be available for debuggers like RenderDoc
	builder.store_all_images();

	GBufferContext* context = new GBufferContext();
	context->global_input_set = &global_input_set;
	context->scene = &scene;
	context->input_layout = ShaderInputSetLayoutBuilder(0).build(gpu);
	context->scene_input_layout = scene.input_layout()
		.build(gpu);

	auto task1 = lft::rg::render_task<GBufferContext>(
			"task1", context,
			[&](const lft::rg::TaskBuildInfo& info,
				GBufferContext* context) {

		input_sets.resize(info.num_buffers());
		std::cout << "Updating buffer idx:" << info.buffer_idx();
		input_sets[info.buffer_idx()] = ShaderInputSetBuilder(0)
			.build(info.gpu(), context->input_layout);

		if(context->pipeline.pipeline() == VK_NULL_HANDLE) {
    		context->layout = PipelineLayoutBuilder()
                .push_constant_range(0, sizeof(uint32_t) * 2,
    					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
    			.input_set(0, global_input_set_layout)
    			.input_set(1, context->input_layout)
    			.input_set(2, context->scene_input_layout)
    			.build(info.gpu());

    		context->pipeline = PipelineBuilder(info.gpu(), info.viewport(),
    				context->layout, info.renderpass(),
    				4, vert, frag)
                .set_vertex_input_info(Vertex::bindings(), Vertex::attributes())
    			.build();
		}

	}, [](const lft::rg::TaskRecordInfo& info, GBufferContext* context) {
		// Record the render pass here
		vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline());
		vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->layout,
				0, 1, context->global_input_set, 0, nullptr);

		context->scene->draw(info.command_buffer(), context->layout);
	});

    task1.add_color_output("col_gbuf", VK_FORMAT_R8G8B8A8_SRGB, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1.add_color_output("norm_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1.add_color_output("pos_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1.add_color_output("pbr_gbuf", VK_FORMAT_R8G8B8A8_UNORM, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1.set_depth_output("depth_gbuf", VK_FORMAT_D32_SFLOAT_S8_UINT, swapchain.extent(), {1.0f, 0});
	builder.add_task(task1.build());


	auto shading_context = new ShadingContext();
	shading_context->global_input_set = &global_input_set;
	shading_context->sampler = sampler;
	shading_context->shading_set_layout = ShaderInputSetLayoutBuilder(5)
		.binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(gpu);

	auto shading_task = lft::rg::render_task<ShadingContext>(
			"shading", shading_context,
			[&](const lft::rg::TaskBuildInfo& info,
				ShadingContext* context) {
				context->input_sets.resize(info.num_buffers());
				context->input_sets[info.buffer_idx()] = ShaderInputSetBuilder(5)
					.buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, light_buffer, 0, sizeof(Light))
					.image(1, info.get_resource("col_gbuf").image_view, context->sampler)
					.image(2, info.get_resource("norm_gbuf").image_view, context->sampler)
					.image(3, info.get_resource("pos_gbuf").image_view, context->sampler)
					.image(4, info.get_resource("pbr_gbuf").image_view, context->sampler)
					.build(info.gpu(), context->shading_set_layout);

				if(context->pipeline.pipeline() == VK_NULL_HANDLE) {
    				auto layout = PipelineLayoutBuilder()
    					.input_set(0, global_input_set_layout)
    					.input_set(1, context->shading_set_layout)
    					.build(info.gpu());

    				context->pipeline = PipelineBuilder(info.gpu(), info.viewport(),
    						layout, info.renderpass(),
    						1, offscr, shade)
    					.set_vertex_input_info({}, {})
    					.build();
				}
			},
			[&](const lft::rg::TaskRecordInfo& info, ShadingContext* context) {
				vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline());
				vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline_layout(),
						0, 1, &global_input_set, 0, nullptr);

				vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline_layout(),
						1, 1, &context->input_sets[info.buffer_idx()], 0, nullptr);

				vkCmdDraw(info.command_buffer(), 3, 1, 0, 0);
			});

	shading_task.add_color_output("swapchain", swapchain.format().format, swapchain.extent(),
			{ 0.0f, 0.0f, 0.0f, 1.0f });
	shading_task.add_dependency("col_gbuf");
	shading_task.add_dependency("norm_gbuf");
	shading_task.add_dependency("pos_gbuf");
	shading_task.add_dependency("pbr_gbuf");

	builder.add_task(shading_task.build());

	auto particle_context = new ParticleContext();
	struct Particle {
	    vec4 position;
		vec4 velocity;
		vec4 color;
	};

	BufferCreateInfo particle_buffer_info = {
	    .size = 1000 * sizeof(Particle),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.isExclusive = true,
	};

	MemoryAllocationInfo memory_info2 = {
		.usage = MEMORY_USAGE_AUTO_PREFER_HOST
	};

	Buffer particle_buffer;
	gpu->memory()->create_buffer(&particle_buffer_info, &memory_info2, &particle_buffer);

	std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(1000);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
        float x = r * cos(theta) * 100 / 100;
        float y = r * sin(theta);
        particle.position[0] = x;
        particle.position[1] = y;
        particle.position[2] = 0.0f;
        particle.position[3] = 1.0f;

        glm_normalize_to(particle.position, particle.velocity);
        glm_vec3_scale(particle.velocity, 0.00025f, particle.velocity);
        particle.color[0] = rndDist(rndEngine);
        particle.color[1] = rndDist(rndEngine);
        particle.color[2] = rndDist(rndEngine);
        particle.color[3] = 1.0f;
        // particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
        // particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

	auto particle_task = lft::rg::compute_task<ParticleContext>(
			"particle", particle_context,
			[&](const lft::rg::TaskBuildInfo& info, ParticleContext* context) {
				context->compute_input_set_layout = ShaderInputSetLayoutBuilder(1)
					.binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
					.build(info.gpu());

				for(uint32_t i = 0; i < info.num_buffers(); i++) {
					context->compute_input_sets.push_back(ShaderInputSetBuilder(1)
						.buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, particle_buffer, 0, 1000 * sizeof(Particle))
						.build(info.gpu(), context->compute_input_set_layout));
				}

				VkPipelineLayoutCreateInfo pipeline_layout_info = {
				    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					.setLayoutCount = 1,
					.pSetLayouts = &context->compute_input_set_layout
				};

				if (vkCreatePipelineLayout(info.gpu()->dev(), &pipeline_layout_info, nullptr, &context->compute_pipeline_layout) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create compute pipeline layout!");
                }

                VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
                computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                computeShaderStageInfo.module = particle_shader.module();
                computeShaderStageInfo.pName = "main";

				VkComputePipelineCreateInfo pipeline_info = {
				    .sType =VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
					.stage = computeShaderStageInfo,
					.layout = context->compute_pipeline_layout,
				};

				if (vkCreateComputePipelines(info.gpu()->dev(),
			        VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
					&context->compute_pipeline) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create compute pipeline!");
                }

                std::cout << "Created compute pipeline " << context->compute_pipeline << std::endl;

			}, [&](const lft::rg::TaskRecordInfo& info, ParticleContext* context) {
                vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, context->compute_pipeline);
                vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, context->compute_pipeline_layout, 0, 1, &context->compute_input_sets[0], 0, 0);
                vkCmdDispatch(info.command_buffer(), 1000 / 256, 1, 1);
			}).add_buffer_output("particle_buffer", 0)
           	.build();

	std::vector<Buffer> buffers;
	buffers.push_back(particle_buffer);
	builder.add_buffer_resource("particle_buffer", buffers, 1000 * sizeof(Particle));
	builder.add_task(particle_task);

	auto particle_draw_task = lft::rg::render_task<ParticleContext>(
	    "particle_draw", particle_context,
		[&](const lft::rg::TaskBuildInfo& info, ParticleContext* context) {
				if(context->draw_pipeline == VK_NULL_HANDLE) {
    				context->draw_pipeline_layout = PipelineLayoutBuilder()
    					.input_set(0, global_input_set_layout)
    					.build(info.gpu());

    				context->draw_pipeline = PipelineBuilder(info.gpu(), info.viewport(),
    						context->draw_pipeline_layout, info.renderpass(),
    						1, particle_vert, particle_frag)
    					.set_vertex_input_info({
                            {
                                .binding = 0,
                                .stride = sizeof(Particle),
                                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                            }
                        }, {
                            { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, position) },
                            { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color) }
                        })
                        .topology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
    					.build().pipeline();
				}

				context->draw_input_sets.resize(info.num_buffers());
				/* context->draw_input_sets[info.buffer_idx()] = ShaderInputSetBuilder(1)
    				.buffer(0, particle_buffer, 0, 1000 * sizeof(Particle))
    				.build(info.gpu(), context->draw_input_set_layout); */
		}, [&](const lft::rg::TaskRecordInfo& info, ParticleContext* context) {
		    vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->draw_pipeline);
			vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
			        context->draw_pipeline_layout,
					0, 1, &global_input_set, 0, nullptr);
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(info.command_buffer(), 0, 1, &particle_buffer.buf, offsets);

			/* vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
			        context->draw_pipeline_layout,
					1, 1, &context->draw_input_sets[info.buffer_idx()], 0, nullptr); */

			vkCmdDraw(info.command_buffer(), 10, 1, 0, 0);
		}).add_dependency("particle_buffer")
	    .add_dependency("shading")
    	.add_color_output("swapchain", swapchain.format().format, extent, {0.0f, 0.0f, 0.0f, 0.0f})
    	.build();

	builder.add_task(particle_draw_task);


    auto ins = instance->instance();
    ImGui_ImplVulkan_LoadFunctions([](const char *function_name, void *vulkan_instance) {
        return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance *>(vulkan_instance)), function_name);
    }, &ins);

	auto imgui_context = new ImGuiContext();
	auto imgui_task = lft::rg::render_task<ImGuiContext>(
			"imgui", imgui_context,
			[&](const lft::rg::TaskBuildInfo& info,
				ImGuiContext* context) {
			if(!context->is_initialized) {
    			ImGui::CreateContext();
    			// ImGui_ImplVulkan_LoadFunctions();
    			ImGuiIO& io = ImGui::GetIO(); (void)io;
    			std::cout << "Loading font: " << io::path::asset("fonts/ProggyClean.ttf") << std::endl;
    			io.Fonts->AddFontFromFileTTF(io::path::asset("fonts/ProggyClean.ttf").c_str(), 20.0f);

    			std::cout << "Initializing ImGui SDL2 for Vulkan..." << std::endl;
    			ImGui_ImplSDL2_InitForVulkan(((SDLWindow*)window.get())->get_handle());
    			std::cout << "Initialized ImGui SDL2 for Vulkan" << std::endl;
    			ImGui_ImplVulkan_InitInfo init_info = {
    				.Instance = instance->instance(),
    				.PhysicalDevice = gpu->gpu(),
    				.Device = gpu->dev(),
    				.QueueFamily = 0,
    				.Queue = gpu->graphics_queue(),
    				.PipelineCache = nullptr,
    				.DescriptorPool = gpu->descriptor_pool(),
    				.Subpass = 0,
    				.MinImageCount = 2,
    				.ImageCount = 2,
    				.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
    				.Allocator = nullptr,
    				.CheckVkResultFn = nullptr,
    			};
    			std::cout << "Initializing ImGui for Vulkan..." << std::endl;
    			ImGui_ImplVulkan_Init(&init_info, info.renderpass());
    			std::cout << "Initialized ImGui for Vulkan" << std::endl;
                context->is_initialized = true;
			}
		}, [&](const lft::rg::TaskRecordInfo& info, ImGuiContext* context) {
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(draw_data, info.command_buffer());
	});

	imgui_task.add_color_output("swapchain", swapchain.format().format, swapchain.extent(), {0.0f, 0.0f, 0.0f, 1.0f});
	imgui_task.add_dependency("shading");
	imgui_task.add_dependency("particle_draw");
	auto imgui = imgui_task.build();
	builder.add_task(imgui_task.build());

	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	/**
	 * Builds the render graph.
	 */
	auto render_graph = builder.build();
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "ns" << std::endl;

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

	VkFence get_image_fence = VK_NULL_HANDLE;
    vkCreateFence(gpu->dev(), &fenceInfo, nullptr, &get_image_fence);


	bool is_open = true;
	bool is_imgui = true;

	vec3 velocity = {0.0f, 0.0f, 0.0f};
	/**
	 * Main loop
	 */
	// for(uint32_t i = 0; i < 3; i++) {
	while(is_open) {
        uint32_t imageIdx = 0;
		auto result = swapchain.get_next_image_idx(nullptr, get_image_fence, &imageIdx);


		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkWaitForFences(gpu->dev(), 1, &get_image_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(gpu->dev(), 1, &get_image_fence);

        if(is_imgui) {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

    		ImGui::ShowDemoWindow();
        }

		SDL_Event event;
		bool change_imgui = false;
		while(window->poll_event(&event)) {
		    if(is_imgui) {
                ImGui_ImplSDL2_ProcessEvent(&event);
			}
			switch(event.type) {
				case SDL_QUIT:
					is_open = false;
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
					    case SDLK_p:
							change_imgui = !event.key.state;
    						break;
						case SDLK_w:
							velocity[1] = (float)event.key.state;
							break;
						case SDLK_s:
							velocity[1] = -(float)event.key.state;
							break;
						case SDLK_a:
							velocity[0] = (float)event.key.state;
							break;
						case SDLK_d:
							velocity[0] = -(float)event.key.state;
							break;
					}
			}
		}

		render_graph.run(imageIdx);
		swapchain.present({ render_graph.buffer(0).final_signal(imageIdx) }, imageIdx);

		camera.move(velocity);
		camera.update();

		if(change_imgui) {
		    if(!is_imgui) {
				builder.add_task(imgui);
			} else {
			    builder.remove_task(imgui.name());
			}

			render_graph = builder.build();
			is_imgui = !is_imgui;
		}
	}

	return 0;
}
