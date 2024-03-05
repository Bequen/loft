//
// Created by martin on 12/8/23.
//

#include "Camera.h"
#include "resources/GpuAllocator.h"

Camera::Camera(Gpu *pGpu):
        m_pGpu(pGpu), m_resource("camera", VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {

    glm_mat4_identity(data.view);
    glm_mat4_identity(data.projection);
    vec3 eye = {0.0f, 1.0f, 0.0f};
    vec3 center = {0.0f, 1.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat(eye, center, up, data.view);
    glm_perspective(glm_rad(60.0f), 1.0f, 0.1f, 1000.0f, data.projection);

	MemoryAllocationInfo allocInfo = {
		.usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
		.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};

    BufferCreateInfo bufferInfo = {
            .size = sizeof(data),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true
    };

    m_pGpu->memory()->create_buffer(&bufferInfo, &allocInfo, &m_buffer);

    void *pData = nullptr;
    m_pGpu->memory()->map(m_buffer.allocation, &pData);

    memcpy(pData, &data, sizeof(data));

    m_pGpu->memory()->unmap(m_buffer.allocation);

    m_resource.set_offset(0)
            .set_size(sizeof(data));

    glm_quat(m_rotation, 0, 0.0f, 0.0f, 1.0f);
    glm_vec3_copy(center, m_position);
    glm_vec3_copy(center, data.position);
}

void Camera::update() {
    glm_mat4_identity(data.view);

    glm_quat_rotate(data.view, m_rotation, data.view);
    glm_translate(data.view, m_position);

    void *pData;
    m_pGpu->memory()->map(m_buffer.allocation, &pData);
    memcpy(pData, &data, sizeof(data));
    m_pGpu->memory()->unmap(m_buffer.allocation);
}

void Camera::move(vec3 velocity)  {
    vec3 right = {data.view[0][2], data.view[1][2], data.view[2][2]};
    glm_vec3_scale(right, velocity[1] * 0.1f, right);

    vec3 forward = {data.view[0][0], data.view[1][0], data.view[2][0]};
    glm_vec3_scale(forward, velocity[0] * 0.1f, forward);

    vec3 up = {0.0f, 0.0f, 1.0f};
    glm_vec3_scale(up, velocity[2] * 0.1f, up);

    glm_vec3_add(m_position, right, m_position);
    glm_vec3_add(m_position, forward, m_position);
    glm_vec3_add(m_position, up, m_position);

    glm_vec3_copy(m_position, data.position);
}

void Camera::rotate(float dx, float dy) {
    vec4 rotZ = {}, rotX = {}, rot = {};
    glm_quat(rotZ, glm_rad(dx), 0.0f, 1.0f, 0.0f);

    glm_quat(rotX, glm_rad(dy), data.view[0][0], data.view[1][0], data.view[2][0]);
    glm_quat_mul(rotX, rotZ, rot);

    glm_quat_mul(m_rotation, rot, m_rotation);
    // glm_quat_rotate(data.view, rot, data.view);
}
