
#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>

#include <volk/volk.h>

#include "Instance.hpp"
#include "RenderPass.hpp"
#include "Swapchain.hpp"
#include "SDLWindow.h"
#include "RenderGraphBuilder.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "cglm/types.h"
#include "io/gltfSceneLoader.hpp"
#include "io/path.hpp"
#include "mesh/runtime/Scene.h"
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

struct ImGuiContext {
    VkRenderPass rp;
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
            .buffer(0, camera.buffer(), 0, sizeof(mat4) * 2 + sizeof(vec4))
            .build(gpu, global_input_set_layout);

	std::vector<VkDescriptorSet> input_sets(1);

    auto sceneData = GltfSceneLoader().from_file("/home/martin/packages/glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf");
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


	lft::rg::Builder builder("swapchain");

	GBufferContext* context = new GBufferContext();
	context->global_input_set = &global_input_set;
	context->scene = &scene;

	auto task1 = std::make_shared<lft::rg::LambdaGpuTask<GBufferContext>>(
			"task1", context,
			[&](const lft::rg::RenderPassBuildInfo& info,
				GBufferContext* context) {
		// Build the render pass here
		auto input_layout = ShaderInputSetLayoutBuilder(0)
			.build(info.gpu());

		for(uint32_t i = 0; i < info.num_buffers(); i++) {
			input_sets[i] = ShaderInputSetBuilder(0)
				.build(info.gpu(), input_layout);
		}

		auto scene_input_set_layout = Scene::input_layout()
			.build(info.gpu());

		context->layout = PipelineLayoutBuilder()
            .push_constant_range(0, sizeof(uint32_t) * 2, 
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.input_set(0, global_input_set_layout)
			.input_set(1, scene_input_set_layout)
			.input_set(2, scene.input_layout().build(gpu))
			.build(info.gpu());

		context->pipeline = PipelineBuilder(info.gpu(), info.viewport(),
				context->layout, info.renderpass(),
				4, vert, frag)
            .set_vertex_input_info(Vertex::bindings(), Vertex::attributes())
			.build();
		
	}, [](const lft::rg::RenderPassRecordInfo& info, GBufferContext* context) {
		// Record the render pass here
		vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline());
		vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->layout,
				0, 1, context->global_input_set, 0, nullptr);

		context->scene->draw(info.command_buffer(), context->layout);
	});

    task1->add_color_output("col_gbuf", VK_FORMAT_R8G8B8A8_SRGB, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1->add_color_output("norm_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1->add_color_output("pos_gbuf", VK_FORMAT_R16G16B16A16_SFLOAT, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1->add_color_output("pbr_gbuf", VK_FORMAT_R8G8B8A8_UNORM, swapchain.extent(), { 0.0f, 0.0f, 0.0f, 1.0f });
    task1->set_depth_output("depth_gbuf", VK_FORMAT_D32_SFLOAT_S8_UINT, swapchain.extent(), {1.0f, 0});
	builder.add_render_pass(task1);


	auto shading_context = new ShadingContext();
	shading_context->global_input_set = &global_input_set;
	shading_context->sampler = sampler;
	auto shading_task = std::make_shared<lft::rg::LambdaGpuTask<ShadingContext>>(
			"shading", shading_context,
			[&](const lft::rg::RenderPassBuildInfo& info,
				ShadingContext* context) {
			context->shading_set_layout = ShaderInputSetLayoutBuilder(5)
				.binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
				.binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
				.binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
				.binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
				.binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
				.build(info.gpu());

			for(uint32_t i = 0; i < info.num_buffers(); i++) {
				context->input_sets.push_back(ShaderInputSetBuilder(5)
                    .buffer(0, light_buffer, 0, sizeof(Light))
					.image(1, info.get_resource("col_gbuf", i).image_view, context->sampler)
					.image(2, info.get_resource("norm_gbuf", i).image_view, context->sampler)
					.image(3, info.get_resource("pos_gbuf", i).image_view, context->sampler)
					.image(4, info.get_resource("pbr_gbuf", i).image_view, context->sampler)
					.build(info.gpu(), context->shading_set_layout));
				std::cout << "Done building" << std::endl;
			}

			auto layout = PipelineLayoutBuilder()
				.input_set(0, global_input_set_layout)
				.input_set(1, context->shading_set_layout)
				.build(info.gpu());

			context->pipeline = PipelineBuilder(info.gpu(), info.viewport(),
					layout, info.renderpass(),
					1, offscr, shade)
				.set_vertex_input_info({}, {})
				.build();
			},
			[&](const lft::rg::RenderPassRecordInfo& info, ShadingContext* context) {
				vkCmdBindPipeline(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline());
				vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline_layout(),
						0, 1, &global_input_set, 0, nullptr);

				vkCmdBindDescriptorSets(info.command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.pipeline_layout(),
						1, 1, &context->input_sets[info.buffer_idx()], 0, nullptr);

				vkCmdDraw(info.command_buffer(), 3, 1, 0, 0);

			});

	shading_task->add_color_output("swapchain", swapchain.format().format, swapchain.extent(),
			{ 0.0f, 0.0f, 0.0f, 1.0f });
	shading_task->add_dependency("col_gbuf");
	shading_task->add_dependency("norm_gbuf");
	shading_task->add_dependency("pos_gbuf");
	shading_task->add_dependency("pbr_gbuf");

	builder.add_render_pass(shading_task);

    auto ins = instance->instance();
    ImGui_ImplVulkan_LoadFunctions([](const char *function_name, void *vulkan_instance) {
        return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance *>(vulkan_instance)), function_name);
    }, &ins);

	auto imgui_context = new ImGuiContext();
	auto imgui_task = std::make_shared<lft::rg::LambdaGpuTask<ImGuiContext>>(
			"imgui", imgui_context,
			[&](const lft::rg::RenderPassBuildInfo& info,
				ImGuiContext* context) {
			ImGui::CreateContext();
			// ImGui_ImplVulkan_LoadFunctions();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			std::cout << "Loading font: " << io::path::asset("fonts/ProggyClean.ttf") << std::endl;
			io.Fonts->AddFontFromFileTTF(io::path::asset("fonts/ProggyClean.ttf").c_str(), 14.0f);

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
		}, [&](const lft::rg::RenderPassRecordInfo& info, ImGuiContext* context) {
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(draw_data, info.command_buffer());
	});

	imgui_task->add_color_output("swapchain", swapchain.format().format, swapchain.extent(), {0.0f, 0.0f, 0.0f, 1.0f});
	imgui_task->add_dependency("shading");
	builder.add_render_pass(imgui_task);

	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	/**
	 * Builds the render graph.
	 */
	auto render_graph = builder.build(gpu, ImageChain::from_swapchain(swapchain), 1);
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "ns" << std::endl;

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

	VkFence get_image_fence = VK_NULL_HANDLE;
    vkCreateFence(gpu->dev(), &fenceInfo, nullptr, &get_image_fence);


	bool is_open = true;

	vec3 velocity = {0.0f, 0.0f, 0.0f};
	/**
	 * Main loop
	 */
	//for(uint32_t i = 0; i < 3; i++)
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

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		SDL_Event event;
		while(window->poll_event(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
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
		swapchain.present({ render_graph.final_semaphore(0, imageIdx) }, imageIdx);

		camera.move(velocity);
		camera.update();
	}

	return 0;
}
