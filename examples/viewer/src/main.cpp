
#include <random>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>

#include <volk.h>

#include "Instance.hpp"
#include "RenderPass.hpp"
#include "SamplerBuilder.hpp"
#include "ShaderManager.hpp"
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
#include "resources/GpuAllocator.h"
#include "runtime/Camera.h"
#include "scene/Light.h"
#include "shaders/ComputePipelineBuilder.hpp"
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
	Pipeline compute_pipeline;

	VkDescriptorSetLayout draw_input_set_layout;
	Pipeline draw_pipeline;

	VkDescriptorSet* global_input_set;
	std::vector<VkDescriptorSet> compute_input_sets;
	std::vector<VkDescriptorSet> draw_input_sets;

	ParticleContext() :
        compute_pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE),
        draw_pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE) {
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

    const std::string engine_name = "loft";
    const std::string application_name = "loft";

    /**
     * Opens up a window
     */
    std::unique_ptr<Window> window = std::make_unique<SDLWindow>(application_name, (VkRect2D){
            0, 0,
            extent.width, extent.height
    });

    /**
     * Different platforms has different extension needs.
     */
    std::vector<std::string> required_extensions = window->get_required_extensions();

    std::vector<std::string> required_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    /**
     * Instance initializes a connection with Vulkan driver
     */
    auto instance = std::make_unique<const Instance>(
			application_name, engine_name,
			required_extensions,
			required_layers,
			lft_dbg_callback);

    /*
     * Surface is a way to tell window:
     * Hey, I am going to render to you from GPU. I need some surface to render to.
     */
    auto surface = window->create_surface(instance->instance());

    /**
     * Gpu manages stuff around rendering. Needed for most graphics operations.
     */
    auto gpu = std::make_unique<Gpu>(std::move(instance), surface);

    /**
     * Create camera for scene
     */
    Camera camera(gpu.get(), (float)extent.width / extent.height);

    /**
     * Swapchain is a queue storage of images to render to for the Window we opened.
     */
    Swapchain swapchain = Swapchain(gpu.get(), extent, surface);


    auto global_input_set_layout = ShaderInputSetLayoutBuilder()
            .uniform_buffer(0)
            .build(gpu.get());

    auto global_input_set = ShaderInputSetBuilder()
            .buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, camera.buffer(), 0, sizeof(mat4) * 2 + sizeof(vec4))
            .build(gpu.get(), global_input_set_layout);

	std::vector<VkDescriptorSet> input_sets(1);

    auto sceneData = GltfSceneLoader().from_file(argv[1]);
	Scene scene(gpu.get(), &sceneData);

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

    ShaderManager shader_manager(gpu.get(), io::path::shader(""));

    VkSampler sampler = lft::SamplerBuilder()
        .address_mode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
        .filter(VK_FILTER_LINEAR)
        .mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
        .border_color(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
        .build(gpu.get());

	lft::rg::Builder builder(gpu.get(), ImageChain::from_swapchain(swapchain), "swapchain");

	// keeps all the drawn images -> to be available for debuggers like RenderDoc
	builder.store_all_images();

	GBufferContext* context = new GBufferContext();
	context->global_input_set = &global_input_set;
	context->scene = &scene;
	context->input_layout = ShaderInputSetLayoutBuilder().build(gpu.get());
	context->scene_input_layout = scene.input_layout()
		.build(gpu.get());

	auto task1 = lft::rg::render_task<GBufferContext>(
			"task1", context,
			[&](const lft::rg::TaskBuildInfo& info,
				GBufferContext* context) {

		input_sets.resize(info.num_buffers());
		input_sets[info.buffer_idx()] = ShaderInputSetBuilder()
			.build(info.gpu(), context->input_layout);

		if(context->pipeline.pipeline() == VK_NULL_HANDLE) {
    		context->layout = PipelineLayoutBuilder()
                .push_constant_range(0, sizeof(uint32_t) * 2,
    					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
    			.input_set(0, global_input_set_layout)
    			.input_set(1, context->input_layout)
    			.input_set(2, context->scene_input_layout)
    			.build(info.gpu());

            auto vertex_shader = shader_manager.get("Opaque.vert.spirv");
            auto fragment_shader = shader_manager.get("Opaque.frag.spirv");

    		context->pipeline = PipelineBuilder(info.gpu(), info.viewport(),
    				context->layout, info.renderpass(),
    				4, // number of attachments, because each needs to have a blending
                       // TODO: How to do this automatically
                    vertex_shader, fragment_shader)
                .set_vertex_input_info(Vertex::bindings(), Vertex::attributes())
    			.build();

            context->pipeline.set_debug_name(gpu.get(), "offscreen_pipeline");
		}

	}, [](const lft::rg::TaskRecordInfo& info, GBufferContext* context) {
        lft::RecordingBindPoint pipeline_bind = info.recording()
            .bind_graphics_pipeline(context->pipeline)
                .bind_descriptor_set(0, *context->global_input_set);

		context->scene->draw(info.recording(), pipeline_bind);
	});

    task1.add_color_output("col_gbuf", VK_FORMAT_R8G8B8A8_SRGB);
    task1.add_color_output("norm_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT);
    task1.add_color_output("pos_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT);
    task1.add_color_output("pbr_gbuf", VK_FORMAT_R8G8B8A8_UNORM);
    task1.set_depth_output("depth_gbuf", VK_FORMAT_D32_SFLOAT_S8_UINT);
	builder.add_task(task1.build());


	auto shading_context = new ShadingContext();
	shading_context->global_input_set = &global_input_set;
	shading_context->sampler = sampler;
	shading_context->shading_set_layout = ShaderInputSetLayoutBuilder()
		.binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.image(1, VK_SHADER_STAGE_FRAGMENT_BIT)
		.image(2, VK_SHADER_STAGE_FRAGMENT_BIT)
		.image(3, VK_SHADER_STAGE_FRAGMENT_BIT)
		.image(4, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(gpu.get());

	auto shading_task = lft::rg::render_task<ShadingContext>(
			"shading", shading_context,
			[&](const lft::rg::TaskBuildInfo& info,
				ShadingContext* context) {
				context->input_sets.resize(info.num_buffers());
				context->input_sets[info.buffer_idx()] = ShaderInputSetBuilder()
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

                    const Shader* offscreen_vertex = shader_manager.get("Offscreen.vert.spirv");
                    const Shader* shading_fragment = shader_manager.get("Shading.frag.spirv");

    				context->pipeline = PipelineBuilder(info.gpu(), info.viewport(),
    						layout, info.renderpass(),
    						1, offscreen_vertex, shading_fragment)
    					.set_vertex_input_info({}, {})
    					.build();
                    
                    context->pipeline.set_debug_name(gpu.get(), "shading_pipeline");
				}
			},
			[&](const lft::rg::TaskRecordInfo& info, ShadingContext* context) {
                info.recording().bind_graphics_pipeline(context->pipeline)
                    .bind_descriptor_set(0, global_input_set)
                    .bind_descriptor_set(1, context->input_sets[info.buffer_idx()]);

                info.recording().draw(3, 1, 0, 0);
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
				context->compute_input_set_layout = ShaderInputSetLayoutBuilder()
					.binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
					.build(info.gpu());

				for(uint32_t i = 0; i < info.num_buffers(); i++) {
					context->compute_input_sets.push_back(ShaderInputSetBuilder()
						.buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, particle_buffer, 0, 1000 * sizeof(Particle))
						.build(info.gpu(), context->compute_input_set_layout));
				}

                const Shader* particle_move_shader = shader_manager.get("Particles.comp.spirv");

                VkPipelineLayout pipeline_layout = PipelineLayoutBuilder()
                    .input_set(0, context->compute_input_set_layout)
                    .build(info.gpu());

                context->compute_pipeline = lft::ComputePipelineBuilder(particle_move_shader, pipeline_layout)
                    .build(info.gpu());
			}, [&](const lft::rg::TaskRecordInfo& info, ParticleContext* context) {
                info.recording()
                    .bind_compute_pipeline(context->compute_pipeline)
                        .bind_descriptor_set(0, context->compute_input_sets[0]);
                info.recording().dispatch(1000 / 256, 1, 1);
			}).add_buffer_output("particle_buffer", 0)
           	.build();

	std::vector<Buffer> buffers;
	buffers.push_back(particle_buffer);
	builder.add_buffer_resource("particle_buffer", buffers, 1000 * sizeof(Particle));
	builder.add_task(particle_task);

	auto particle_draw_task = lft::rg::render_task<ParticleContext>(
	    "particle_draw", particle_context,
		[&](const lft::rg::TaskBuildInfo& info, ParticleContext* context) {
				if(context->draw_pipeline.pipeline() == VK_NULL_HANDLE) {
    				VkPipelineLayout draw_pipeline_layout = PipelineLayoutBuilder()
    					.input_set(0, global_input_set_layout)
    					.build(info.gpu());
                        
                    const Shader* particle_draw_vertex = shader_manager.get("ParticleDraw.vert.spirv");
                    const Shader* particle_draw_fragment = shader_manager.get("ParticleDraw.frag.spirv");

    				context->draw_pipeline = PipelineBuilder(info.gpu(), info.viewport(),
    						draw_pipeline_layout, info.renderpass(),
    						1, particle_draw_vertex, particle_draw_fragment)
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
    					.build();
				}

				context->draw_input_sets.resize(info.num_buffers());
				/* context->draw_input_sets[info.buffer_idx()] = ShaderInputSetBuilder(1)
    				.buffer(0, particle_buffer, 0, 1000 * sizeof(Particle))
    				.build(info.gpu(), context->draw_input_set_layout); */
		}, [&](const lft::rg::TaskRecordInfo& info, ParticleContext* context) {
            info.recording().bind_graphics_pipeline(context->draw_pipeline)
                .bind_descriptor_set(0, global_input_set);

            info.recording().bind_vertex_buffers({particle_buffer}, {0});
            info.recording().draw(10, 1, 0, 0);
		}).add_dependency("particle_buffer")
	    .add_dependency("shading")
    	.add_color_output("swapchain", swapchain.format().format, extent, {0.0f, 0.0f, 0.0f, 0.0f})
    	.build();

	builder.add_task(particle_draw_task);


    auto ins = gpu->instance()->instance();
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
    			ImGuiIO& io = ImGui::GetIO(); (void)io;
    			io.Fonts->AddFontFromFileTTF(io::path::asset("fonts/ProggyClean.ttf").c_str(), 32.0f);

    			ImGui_ImplSDL2_InitForVulkan(((SDLWindow*)window.get())->get_handle());
    			ImGui_ImplVulkan_InitInfo init_info = {
    				.Instance = info.gpu()->instance()->instance(),
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
    			ImGui_ImplVulkan_Init(&init_info, info.renderpass());
                context->is_initialized = true;
			}
		}, [&](const lft::rg::TaskRecordInfo& info, ImGuiContext* context) {
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(draw_data, info.recording().cmdbuf());
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

    VkSemaphore wait_on_image_semaphore = gpu->create_semaphore();

    VkFence wait_on_image_fence = gpu->create_fence(false);

	bool is_open = true;
	bool is_imgui = true;

	vec3 velocity = {0.0f, 0.0f, 0.0f};

	/**
	 * Main loop
	 */
	while(is_open) {
        uint32_t imageIdx = 0;
		auto result = swapchain.get_next_image_idx(VK_NULL_HANDLE, wait_on_image_fence, &imageIdx);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

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

		render_graph.run(imageIdx, VK_NULL_HANDLE, wait_on_image_fence);
		swapchain.present({ render_graph.buffer(0).final_signal(imageIdx) }, imageIdx);

		camera.move(velocity);
		camera.update();

        // Implement turning ImGui task on and off
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
