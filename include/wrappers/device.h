//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/** Implements a wrapper for a Vulkan device. Implemented in order to:
 *
 *  - manage life-time of device instances.
 *  - encapsulate all logic required to manipulate devices.
 *  - let ObjectTracker detect leaking device instances.
 *
 *  The wrapper is thread-safe (on an opt-in basis).
 **/
#ifndef WRAPPERS_DEVICE_H
#define WRAPPERS_DEVICE_H

#include "misc/debug.h"
#include "misc/mt_safety.h"
#include "misc/types.h"
#include <algorithm>

namespace Anvil
{
    class BaseDevice : public Anvil::MTSafetySupportProvider
    {
    public:
        /* Public functions */

        /* Constructor */
        BaseDevice(const Anvil::Instance* in_parent_instance_ptr,
                   bool                   in_mt_safe);

        virtual ~BaseDevice();

        /** Retrieves a command pool, created for the specified queue family index.
         *
         *  @param in_vk_queue_family_index Vulkan index of the queue family to return the command pool for.
         *
         *  @return As per description
         **/
        Anvil::CommandPool* get_command_pool_for_queue_family_index(uint32_t in_vk_queue_family_index) const
        {
            return m_command_pool_ptr_per_vk_queue_fam.at(in_vk_queue_family_index).get();
        }

        /** Retrieves a compute pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        Anvil::ComputePipelineManager* get_compute_pipeline_manager() const
        {
            return m_compute_pipeline_manager_ptr.get();
        }

        /** Returns a Queue instance, corresponding to a compute queue at index @param in_n_queue
         *
         *  @param in_n_queue Index of the compute queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_compute_queue(uint32_t in_n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_compute_queues.size() > in_n_queue)
            {
                result_ptr = m_compute_queues.at(in_n_queue);
            }

            return result_ptr;
        }

        Anvil::DescriptorSetLayoutManager* get_descriptor_set_layout_manager() const
        {
            return m_descriptor_set_layout_manager_ptr.get();
        }

        /** Retrieves a raw Vulkan handle for this device.
         *
         *  @return As per description
         **/
        VkDevice get_device_vk() const
        {
            return m_device;
        }

        /** Retrieves a DescriptorSet instance which defines 0 descriptors.
         *
         *  Do NOT release. This object is owned by Device and will be released at object tear-down time.
         **/
        const Anvil::DescriptorSet* get_dummy_descriptor_set() const;

        /** Retrieves a DescriptorSetLayout instance, which encapsulates a single descriptor set layout,
         *  which holds 1 descriptor set holding 0 descriptors.
         *
         *  Do NOT release. This object is owned by Device and will be released at object tear-down time.
         **/
        Anvil::DescriptorSetLayout* get_dummy_descriptor_set_layout() const;

        /** Returns a container with entry-points to functions introduced by VK_AMD_draw_indirect_count  extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionAMDDrawIndirectCountEntrypoints& get_extension_amd_draw_indirect_count_entrypoints() const
        {
            anvil_assert(m_amd_draw_indirect_count_enabled);

            return m_amd_draw_indirect_count_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_AMD_shader_info extension.
        *
        *  Will fire an assertion failure if the extension was not requested at device creation time.
        **/
        const ExtensionAMDShaderInfoEntrypoints& get_extension_amd_shader_info_entrypoints() const
        {
            anvil_assert(m_amd_shader_info_enabled);

            return m_amd_shader_info_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_EXT_debug_marker extension.
         *
         *  Will fire an assertion failure if the extension is not supported.
         **/
        const ExtensionEXTDebugMarkerEntrypoints& get_extension_ext_debug_marker_entrypoints() const
        {
            anvil_assert(m_ext_debug_marker_enabled);

            return m_ext_debug_marker_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_bind_memory2 extension. **/
        const ExtensionKHRBindMemory2Entrypoints& get_extension_khr_bind_memory2_entrypoints() const
        {
            anvil_assert(m_khr_bind_memory2_enabled);

            return m_khr_bind_memory2_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_descriptor_update_template extension. **/
        const ExtensionKHRDescriptorUpdateTemplateEntrypoints& get_extension_khr_descriptor_update_template_entrypoints() const
        {
            anvil_assert(m_khr_descriptor_update_template_enabled);

            return m_khr_descriptor_update_template_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_maintenance1 extension. **/
        const ExtensionKHRMaintenance1Entrypoints& get_extension_khr_maintenance1_entrypoints() const
        {
            anvil_assert(m_khr_maintenance1_enabled);

            return m_khr_maintenance1_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_maintenance3 extension. **/
        const ExtensionKHRMaintenance3Entrypoints& get_extension_khr_maintenance3_entrypoints() const
        {
            anvil_assert(m_khr_maintenance3_enabled);

            return m_khr_maintenance3_extension_entrypoints;
        }

        /** Returns a container with entry-points to functions introduced by VK_KHR_swapchain extension.
         *
         *  Will fire an assertion failure if the extension was not requested at device creation time.
         **/
        const ExtensionKHRSwapchainEntrypoints& get_extension_khr_swapchain_entrypoints() const
        {
            anvil_assert(m_khr_swapchain_enabled);

            return m_khr_swapchain_extension_entrypoints;
        }

        /** Retrieves a graphics pipeline manager, created for this device instance.
         *
         *  @return As per description
         **/
        Anvil::GraphicsPipelineManager* get_graphics_pipeline_manager() const
        {
            return m_graphics_pipeline_manager_ptr.get();
        }

        /** Returns the number of compute queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_compute_queues() const
        {
            return static_cast<uint32_t>(m_compute_queues.size() );
        }

        /** Returns the number of queues available for the specified queue family index */
        uint32_t get_n_queues(uint32_t in_n_queue_family) const;

        /** Returns the number of queues available for the specified queue family type
         *
         *  @param in_family_type Queue family type to use for the query.
         *
         *  @return As per description.
         **/
        uint32_t get_n_queues(QueueFamilyType in_family_type) const
        {
            uint32_t result = 0;

            switch (in_family_type)
            {
                case Anvil::QueueFamilyType::COMPUTE:   result = static_cast<uint32_t>(m_compute_queues.size() );   break;
                case Anvil::QueueFamilyType::TRANSFER:  result = static_cast<uint32_t>(m_transfer_queues.size() );  break;
                case Anvil::QueueFamilyType::UNIVERSAL: result = static_cast<uint32_t>(m_universal_queues.size() ); break;

                default:
                {
                    /* Invalid queue family type */
                    anvil_assert_fail();
                }
            }

            return result;
        }

        /** Returns the number of sparse binding queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_sparse_binding_queues() const
        {
            return static_cast<uint32_t>(m_sparse_binding_queues.size() );
        }

        /** Returns the number of transfer queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_transfer_queues() const
        {
            return static_cast<uint32_t>(m_transfer_queues.size() );
        }

        /** Returns the number of universal queues supported by this device.
         *
         *  @return As per description
         **/
        uint32_t get_n_universal_queues() const
        {
            return static_cast<uint32_t>(m_universal_queues.size() );
        }

        /** Returns Vulkan instance wrapper used to create this device. */
        const Anvil::Instance* get_parent_instance() const
        {
            return m_parent_instance_ptr;
        }

        /** Returns features supported by physical device(s) which have been used to instantiate
         *  this logical device instance.
         **/
        virtual const Anvil::PhysicalDeviceFeatures& get_physical_device_features() const = 0;

        /** Returns format properties, as reported by physical device(s) which have been used to
         *  instantiate this logical device instance.
         *
         *  @param in_format Format to use for the query.
         */
        virtual const Anvil::FormatProperties& get_physical_device_format_properties(VkFormat in_format) const = 0;

        /** Returns image format properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         *
         *  @param in_format  Meaning as per Vulkan spec.
         *  @param in_type    Meaning as per Vulkan spec.
         *  @param in_tiling  Meaning as per Vulkan spec.
         *  @param in_usage   Meaning as per Vulkan spec.
         *  @param in_flags   Meaning as per Vulkan spec.
         *  @param out_result If the function returns true, the requested information will be stored
         *                    at this location.
         *
         *  @return true if successful, false otherwise.
         **/
        virtual bool get_physical_device_image_format_properties(VkFormat                 in_format,
                                                                 VkImageType              in_type,
                                                                 VkImageTiling            in_tiling,
                                                                 VkImageUsageFlags        in_usage,
                                                                 VkImageCreateFlags       in_flags,
                                                                 VkImageFormatProperties& out_result) const = 0;

        /** Returns memory properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         **/
        virtual const Anvil::MemoryProperties& get_physical_device_memory_properties() const = 0;

        /** Returns general physical device properties, as reported by physical device(s) which have been used
         *  to instantiate this logical device instance.
         **/
        virtual const Anvil::PhysicalDeviceProperties& get_physical_device_properties() const = 0;

        /** Returns queue families available for the physical device(s) used to instantiate this
         *  logical device instance.
         **/
        virtual const QueueFamilyInfoItems& get_physical_device_queue_families() const = 0;

        /** Returns sparse image format properties, as reported by physical device(s) which have been
         *  used to instantiate this logical device instance.
         *
         *  @param in_format       Meaning as per Vulkan spec.
         *  @param in_type         Meaning as per Vulkan spec.
         *  @param in_sample_count Meaning as per Vulkan spec.
         *  @param in_usage        Meaning as per Vulkan spec.
         *  @param in_tiling       Meaning as per Vulkan spec.
         *  @param out_result      If the function returns true, the requested information will be stored
         *                         at this location.
         *
         *  @return true if successful, false otherwise.
         **/
        virtual bool get_physical_device_sparse_image_format_properties(VkFormat                                    in_format,
                                                                        VkImageType                                 in_type,
                                                                        VkSampleCountFlagBits                       in_sample_count,
                                                                        VkImageUsageFlags                           in_usage,
                                                                        VkImageTiling                               in_tiling,
                                                                        std::vector<VkSparseImageFormatProperties>& out_result) const = 0;


        /** Returns surface capabilities, as reported for physical device(s) which have been used to instantiate
         *  this logical device instance.
         **/
        virtual bool get_physical_device_surface_capabilities(Anvil::RenderingSurface*  in_surface_ptr,
                                                              VkSurfaceCapabilitiesKHR* out_result_ptr) const = 0;

        /** Returns a pipeline cache, created specifically for this device.
         *
         *  @return As per description
         **/
        Anvil::PipelineCache* get_pipeline_cache() const
        {
            return m_pipeline_cache_ptr.get();
        }

        /** Returns a pipeline layout manager, created specifically for this device.
         *
         *  @return As per description
         **/
        Anvil::PipelineLayoutManager* get_pipeline_layout_manager() const
        {
            return m_pipeline_layout_manager_ptr.get();
        }

        /** Calls the device-specific implementaton of vkGetDeviceProcAddr(), using this device
         *  instance and user-specified arguments.
         *
         *  @param in_name Func name to use for the query. Must not be nullptr.
         *
         *  @return As per description
         **/
        PFN_vkVoidFunction get_proc_address(const char* in_name) const
        {
            return vkGetDeviceProcAddr(m_device,
                                       in_name);
        }

        PFN_vkVoidFunction get_proc_address(const std::string& in_name) const
        {
            return get_proc_address(in_name.c_str() );
        }

        /** TODO */
        Anvil::Queue* get_queue(const Anvil::QueueFamilyType& in_queue_family_type,
                                uint32_t                      in_n_queue) const;

        /* Returns a Queue instance representing a Vulkan queue from queue family @param in_n_queue_family
         * at index @param in_n_queue.
         *
         * @param in_n_queue_family See @brief.
         * @param in_n_queue        See @brief.
         *
         * @return Requested Queue instance OR nullptr if either of the parameters is invalid.
         */
        Anvil::Queue* get_queue_for_queue_family_index(uint32_t in_n_queue_family,
                                                       uint32_t in_n_queue) const;

        /** TODO
         *
         *  NOTE: A single Anvil queue family MAY map onto more than one Vulkan queue family type. The same is not true the other way around.
         */
        bool get_queue_family_indices_for_queue_family_type(const Anvil::QueueFamilyType& in_queue_family_type,
                                                            uint32_t*                     out_opt_n_queue_family_indices_ptr,
                                                            const uint32_t**              out_opt_queue_family_indices_ptr_ptr) const;

        /** TODO.
         *
         *  NOTE: Anvil's queue family may map onto more than one Vulkan queue family. The same is not true the other way around.
         */
        Anvil::QueueFamilyType get_queue_family_type(uint32_t in_queue_family_index) const;

        /** Returns detailed queue family information for a queue family at index @param in_queue_family_index . */
        virtual const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t in_queue_family_index) const = 0;

        /** Returns sample locations used by the physical device for the specified sample count.
         *
         *  NOTE: This function will only report success if the physical device supports standard sample locations.
         *
         *  @param in_sample_count Sample count to use for the query.
         *  @param out_result_ptr  Must not be null. Deref will be cleared and filled with sample locations
         *                         for the specified sample count.
         *
         *  @return true if successful, false otherwise.
         */
        bool get_sample_locations(VkSampleCountFlagBits               in_sample_count,
                                  std::vector<Anvil::SampleLocation>* out_result_ptr) const;

        /** Returns shader module cache instance */
        Anvil::ShaderModuleCache* get_shader_module_cache() const
        {
            return m_shader_module_cache_ptr.get();
        }

        /** Returns a Queue instance, corresponding to a sparse binding-capable queue at index @param in_n_queue,
         *  which supports queue family capabilities specified with @param opt_required_queue_flags.
         *
         *  @param in_n_queue                  Index of the queue to retrieve the wrapper instance for.
         *  @param in_opt_required_queue_flags Additional queue family bits the returned queue needs to support.
         *                                     0 by default.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_sparse_binding_queue(uint32_t     in_n_queue,
                                               VkQueueFlags in_opt_required_queue_flags = 0) const;

        /** Returns a Queue instance, corresponding to a transfer queue at index @param in_n_queue
         *
         *  @param in_n_queue Index of the transfer queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_transfer_queue(uint32_t in_n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_transfer_queues.size() > in_n_queue)
            {
                result_ptr = m_transfer_queues.at(in_n_queue);
            }

            return result_ptr;
        }

        /** Returns a Queue instance, corresponding to a universal queue at index @param in_n_queue
         *
         *  @param in_n_queue Index of the universal queue to retrieve the wrapper instance for.
         *
         *  @return As per description
         **/
        Anvil::Queue* get_universal_queue(uint32_t in_n_queue) const
        {
            Anvil::Queue* result_ptr = nullptr;

            if (m_universal_queues.size() > in_n_queue)
            {
                result_ptr = m_universal_queues.at(in_n_queue);
            }

            return result_ptr;
        }

        /* Tells what type this device instance is */
        virtual DeviceType get_type() const = 0;

        bool is_amd_draw_indirect_count_extension_enabled() const
        {
            return m_amd_draw_indirect_count_enabled;
        }

        bool is_amd_gcn_shader_extension_enabled() const
        {
            return m_amd_gcn_shader_enabled;
        }

        bool is_amd_gpu_shader_half_float_extension_enabled() const
        {
            return m_amd_gpu_shader_half_float_enabled;
        }

        bool is_amd_gpu_shader_int16_extension_enabled() const
        {
            return m_amd_gpu_shader_int16_enabled;
        }

        bool is_amd_negative_viewport_height_extension_enabled() const
        {
            return m_amd_negative_viewport_height_enabled;
        }

        bool is_amd_rasterization_order_extension_enabled() const
        {
            return m_amd_rasterization_order_enabled;
        }

        bool is_amd_shader_ballot_extension_enabled() const
        {
            return m_amd_shader_ballot_enabled;
        }

        bool is_amd_shader_explicit_vertex_parameter_extension_enabled() const
        {
            return m_amd_shader_explicit_vertex_parameter_enabled;
        }

        bool is_amd_shader_fragment_mask_extension_enabled() const
        {
            return m_amd_shader_fragment_mask_enabled;
        }

        bool is_amd_shader_image_load_store_lod_enabled() const
        {
            return m_amd_shader_image_load_store_lod_enabled;
        }

        bool is_amd_shader_info_extension_enabled() const
        {
            return m_amd_shader_info_enabled;
        }

        bool is_amd_shader_trinary_minmax_extension_enabled() const
        {
            return m_amd_shader_trinary_minmax_enabled;
        }

        bool is_amd_texture_gather_bias_lod_extension_enabled() const
        {
            return m_amd_texture_gather_bias_lod_enabled;
        }

        bool is_compute_queue_family_index(const uint32_t& in_queue_family_index) const;

        bool is_ext_shader_stencil_export_extension_enabled() const
        {
            return m_ext_shader_stencil_export_enabled;
        }

        /* Tells whether the device has been created with the specified extension enabled.
         *
         * @param in_extension_name Null-terminated name of the extension. Must not be null.
         *
         * @return true if the device has been created with the extension enabled, false otherwise.
         **/
        bool is_extension_enabled(const std::string& in_extension_name) const
        {
            return std::find(m_enabled_extensions.begin(),
                             m_enabled_extensions.end(),
                             in_extension_name) != m_enabled_extensions.end();
        }

        bool is_ext_debug_marker_extension_enabled() const
        {
            return m_ext_debug_marker_enabled;
        }

        bool is_ext_shader_subgroup_ballot_extension_enabled() const
        {
            return m_ext_shader_subgroup_ballot_enabled;
        }

        bool is_ext_shader_subgroup_vote_extension_enabled() const
        {
            return m_ext_shader_subgroup_vote_enabled;
        }

        bool is_khr_16bit_storage_extension_enabled() const
        {
            return m_khr_16bit_storage_enabled;
        }

        bool is_khr_descriptor_update_template_extension_enabled() const
        {
            return m_khr_descriptor_update_template_enabled;
        }

        bool is_khr_bind_memory2_extension_enabled() const
        {
            return m_khr_bind_memory2_enabled;
        }

        bool is_khr_maintenance1_extension_enabled() const
        {
            return m_khr_maintenance1_enabled;
        }

        bool is_khr_maintenance3_extension_enabled() const
        {
            return m_khr_maintenance3_enabled;
        }

        bool is_khr_storage_buffer_storage_class_enabled() const
        {
            return m_khr_storage_buffer_storage_class_enabled;
        }

        bool is_khr_surface_extension_enabled() const
        {
            return m_khr_surface_enabled;
        }

        bool is_khr_swapchain_extension_enabled() const
        {
            return m_khr_swapchain_enabled;
        }

        bool is_transfer_queue_family_index(const uint32_t& in_queue_family_index) const;

        bool is_universal_queue_family_index(const uint32_t& in_queue_family_index) const;

        bool wait_idle() const;

    protected:
        /* Protected type definitions */

        typedef struct DeviceQueueFamilyMemberInfo
        {
            uint32_t family_index;
            uint32_t n_queues;

            DeviceQueueFamilyMemberInfo()
                :family_index(UINT32_MAX),
                 n_queues    (UINT32_MAX)
            {
                /* Stub */
            }

            explicit DeviceQueueFamilyMemberInfo(uint32_t in_family_index,
                                                 uint32_t in_n_queues)
                :family_index(in_family_index),
                 n_queues    (in_n_queues)
            {
                /* Stub */
            }

            bool operator==(const uint32_t& in_family_index) const
            {
                return family_index == in_family_index;
            }
        } DeviceQueueFamilyMemberInfo;

        typedef struct
        {
            uint32_t                                                                    n_total_queues_per_family[static_cast<uint32_t>(Anvil::QueueFamilyType::COUNT)];
            std::map<Anvil::QueueFamilyType, std::vector<DeviceQueueFamilyMemberInfo> > queue_families;
        } DeviceQueueFamilyInfo;

        /* Protected functions */
        std::vector<float> get_queue_priorities(const QueueFamilyInfo*              in_queue_family_info_ptr) const;
        void               init                (const DeviceExtensionConfiguration& in_extensions,
                                                const std::vector<std::string>&     in_layers,
                                                bool                                in_transient_command_buffer_allocs_only,
                                                bool                                in_support_resettable_command_buffer_allocs,
                                                bool                                in_enable_shader_module_cache);

        BaseDevice& operator=(const BaseDevice&);
        BaseDevice           (const BaseDevice&);

        /** Retrieves family indices of compute, DMA, graphics, transfer queue families for the specified physical device.
         *
         *  @param in_physical_device_ptr           Physical device to use for the query.
         *  @param out_device_queue_family_info_ptr Deref will be used to store the result info. Must not be null.
         *
         **/
        virtual void get_queue_family_indices_for_physical_device(const Anvil::PhysicalDevice* in_physical_device_ptr,
                                                                  DeviceQueueFamilyInfo*       out_device_queue_family_info_ptr) const;

        virtual void create_device                         (const std::vector<const char*>& in_extensions,
                                                            const std::vector<const char*>& in_layers,
                                                            const VkPhysicalDeviceFeatures& in_features,
                                                            DeviceQueueFamilyInfo*          out_queue_families_ptr)       = 0;
        virtual void init_device                           ()                                                             = 0;
        virtual bool is_layer_supported                    (const std::string&              in_layer_name)          const = 0;
        virtual bool is_physical_device_extension_supported(const std::string&              in_extension_name)      const = 0;

        /* Protected variables */
        std::vector<Anvil::Queue*> m_compute_queues;
        DeviceQueueFamilyInfo      m_device_queue_families;
        std::vector<Anvil::Queue*> m_sparse_binding_queues;
        std::vector<Anvil::Queue*> m_transfer_queues;
        std::vector<Anvil::Queue*> m_universal_queues;

        std::vector<std::unique_ptr<Anvil::Queue> > m_owned_queues;

        std::map<uint32_t, Anvil::QueueFamilyType>                                      m_queue_family_index_to_type;
        std::map<Anvil::QueueFamilyType, std::vector<uint32_t> >                        m_queue_family_type_to_queue_family_indices;
        std::map<uint32_t /* Vulkan queue family index */, std::vector<Anvil::Queue*> > m_queue_ptrs_per_vk_queue_fam;

        /* Protected variables */
        VkDevice m_device;

        ExtensionAMDDrawIndirectCountEntrypoints          m_amd_draw_indirect_count_extension_entrypoints;
        ExtensionAMDShaderInfoEntrypoints                 m_amd_shader_info_extension_entrypoints;
        ExtensionEXTDebugMarkerEntrypoints                m_ext_debug_marker_extension_entrypoints;
        ExtensionKHRBindMemory2Entrypoints                m_khr_bind_memory2_extension_entrypoints;
        ExtensionKHRDescriptorUpdateTemplateEntrypoints   m_khr_descriptor_update_template_extension_entrypoints;
        ExtensionKHRMaintenance1Entrypoints               m_khr_maintenance1_extension_entrypoints;
        ExtensionKHRMaintenance3Entrypoints               m_khr_maintenance3_extension_entrypoints;
        ExtensionKHRSurfaceEntrypoints                    m_khr_surface_extension_entrypoints;
        ExtensionKHRSwapchainEntrypoints                  m_khr_swapchain_extension_entrypoints;

    private:
        /* Private variables */
        bool m_amd_draw_indirect_count_enabled;
        bool m_amd_gcn_shader_enabled;
        bool m_amd_gpu_shader_half_float_enabled;
        bool m_amd_gpu_shader_int16_enabled;
        bool m_amd_negative_viewport_height_enabled;
        bool m_amd_rasterization_order_enabled;
        bool m_amd_shader_ballot_enabled;
        bool m_amd_shader_explicit_vertex_parameter_enabled;
        bool m_amd_shader_fragment_mask_enabled;
        bool m_amd_shader_image_load_store_lod_enabled;
        bool m_amd_shader_info_enabled;
        bool m_amd_shader_trinary_minmax_enabled;
        bool m_amd_texture_gather_bias_lod_enabled;
        bool m_ext_debug_marker_enabled;
        bool m_ext_shader_stencil_export_enabled;
        bool m_ext_shader_subgroup_ballot_enabled;
        bool m_ext_shader_subgroup_vote_enabled;
        bool m_khr_16bit_storage_enabled;
        bool m_khr_bind_memory2_enabled;
        bool m_khr_descriptor_update_template_enabled;
        bool m_khr_maintenance1_enabled;
        bool m_khr_maintenance3_enabled;
        bool m_khr_storage_buffer_storage_class_enabled;
        bool m_khr_surface_enabled;
        bool m_khr_swapchain_enabled;


        std::unique_ptr<Anvil::ComputePipelineManager> m_compute_pipeline_manager_ptr;
        DescriptorSetLayoutManagerUniquePtr            m_descriptor_set_layout_manager_ptr;
        Anvil::DescriptorSetGroupUniquePtr             m_dummy_dsg_ptr;
        std::vector<std::string>                       m_enabled_extensions;
        GraphicsPipelineManagerUniquePtr               m_graphics_pipeline_manager_ptr;
        const Anvil::Instance*                         m_parent_instance_ptr;
        PipelineCacheUniquePtr                         m_pipeline_cache_ptr;
        PipelineLayoutManagerUniquePtr                 m_pipeline_layout_manager_ptr;
        Anvil::ShaderModuleCacheUniquePtr              m_shader_module_cache_ptr;

        std::vector<CommandPoolUniquePtr> m_command_pool_ptr_per_vk_queue_fam;

        friend struct DeviceDeleter;
    };

    /* Implements a logical device wrapper, created from a single physical device */
    class SGPUDevice : public BaseDevice
    {
    public:
        /* Public functions */

        virtual ~SGPUDevice();

        /** Creates a new Vulkan Device instance using a user-specified physical device instance.
         *
         *  @param in_physical_device_ptr                      Physical device to create this device from. Must not be nullptr.
         *  @param in_extensions                               Tells which extensions must/should be specified at creation time.
         *  @param in_layers                                   A vector of layer names to be used when creating the device.
         *                                                     Can be empty.
         *  @param in_transient_command_buffer_allocs_only     True if the command pools, which are going to be initialized after
         *                                                     the device is created, should be set up for transient command buffer
         *                                                     support.
         *  @param in_support_resettable_command_buffer_allocs True if the command pools should be configured for resettable command
         *                                                     buffer support.
         *  @param in_enable_shader_module_cache               True if all spawned shader modules should be cached throughout instance lifetime.
         *                                                     False if they should be released as soon as all shared pointers go out of scope.
         *  @param in_mt_safe                                  True if command buffer creation and queue submissions should be automatically serialized.
         *                                                     Set to false if your app is never going to use more than one thread at a time for
         *                                                     command buffer creation or submission.
         *
         *  @return A new Device instance.
         **/
        static SGPUDeviceUniquePtr create(const Anvil::PhysicalDevice*         in_physical_device_ptr,
                                          bool                                 in_enable_shader_module_cache,
                                          const DeviceExtensionConfiguration&  in_extensions,
                                          const std::vector<std::string>&      in_layers,
                                          bool                                 in_transient_command_buffer_allocs_only,
                                          bool                                 in_support_resettable_command_buffer_allocs,
                                          bool                                 in_mt_safe = false);

        /** Creates a new swapchain instance for the device.
         *
         *  @param in_parent_surface_ptr Rendering surface to create the swapchain for. Must not be nullptr.
         *  @param in_window_ptr         current window to create the swapchain for. Must not be nullptr.
         *  @param in_image_format       Format which the swap-chain should use.
         *  @param in_present_mode       Presentation mode which the swap-chain should use.
         *  @param in_usage              Image usage flags describing how the swap-chain is going to be used.
         *  @param in_n_swapchain_images Number of images the swap-chain should use.
         *
         *  @return A new Swapchain instance.
         **/
        Anvil::SwapchainUniquePtr create_swapchain(Anvil::RenderingSurface* in_parent_surface_ptr,
                                                   Anvil::Window*           in_window_ptr,
                                                   VkFormat                 in_image_format,
                                                   VkPresentModeKHR         in_present_mode,
                                                   VkImageUsageFlags        in_usage,
                                                   uint32_t                 in_n_swapchain_images);

        /** Retrieves a PhysicalDevice instance, from which this device instance was created.
         *
         *  @return As per description
         **/
        const Anvil::PhysicalDevice* get_physical_device() const
        {
            return m_parent_physical_device_ptr;
        }

        /** See documentation in BaseDevice for more details */
        const Anvil::PhysicalDeviceFeatures& get_physical_device_features() const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::FormatProperties& get_physical_device_format_properties(VkFormat in_format) const override;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_image_format_properties(VkFormat                 in_format,
                                                         VkImageType              in_type,
                                                         VkImageTiling            in_tiling,
                                                         VkImageUsageFlags        in_usage,
                                                         VkImageCreateFlags       in_flags,
                                                         VkImageFormatProperties& out_result) const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::MemoryProperties& get_physical_device_memory_properties() const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::PhysicalDeviceProperties& get_physical_device_properties() const override;

        /** See documentation in BaseDevice for more details */
        const QueueFamilyInfoItems& get_physical_device_queue_families() const override;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_sparse_image_format_properties(VkFormat                                    in_format,
                                                                VkImageType                                 in_type,
                                                                VkSampleCountFlagBits                       in_sample_count,
                                                                VkImageUsageFlags                           in_usage,
                                                                VkImageTiling                               in_tiling,
                                                                std::vector<VkSparseImageFormatProperties>& out_result) const override;

        /** See documentation in BaseDevice for more details */
        bool get_physical_device_surface_capabilities(Anvil::RenderingSurface*  in_surface_ptr,
                                                      VkSurfaceCapabilitiesKHR* out_result_ptr) const override;

        /** See documentation in BaseDevice for more details */
        const Anvil::QueueFamilyInfo* get_queue_family_info(uint32_t in_queue_family_index) const override;

        /* Tells what type this device instance is */
        DeviceType get_type() const override
        {
            return DEVICE_TYPE_SINGLE_GPU;
        }

    protected:
        /* Protected functions */
        void create_device                         (const std::vector<const char*>& in_extensions,
                                                    const std::vector<const char*>& in_layers,
                                                    const VkPhysicalDeviceFeatures& in_features,
                                                    DeviceQueueFamilyInfo*          out_queue_families_ptr) override;
        void get_queue_family_indices              (DeviceQueueFamilyInfo*          out_device_queue_family_info_ptr) const;
        void init_device                           () override;
        bool is_layer_supported                    (const std::string&              in_layer_name)     const override;
        bool is_physical_device_extension_supported(const std::string&              in_extension_name) const override;

    private:
        /* Private type definitions */

        /* Private functions */

        /** Private constructor. Please use create() instead. */
        SGPUDevice(const Anvil::PhysicalDevice* in_physical_device_ptr,
                   bool                         in_mt_safe);

        /* Private variables */
        const Anvil::PhysicalDevice* m_parent_physical_device_ptr;
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_DEVICE_H */
