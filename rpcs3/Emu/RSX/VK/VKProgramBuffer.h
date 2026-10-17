#pragma once
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKPipelineCompiler.h"
#include "../Program/ProgramStateCache.h"

#ifdef RPCS3_UWP
extern bool g_uwp_deferred_shader_compile; // VKProgramPipeline.cpp
#endif

namespace vk
{
	struct VKTraits
	{
		using vertex_program_type = VKVertexProgram;
		using fragment_program_type = VKFragmentProgram;
		using pipeline_type = vk::glsl::program;
		using pipeline_storage_type = std::unique_ptr<vk::glsl::program>;
		using pipeline_properties = vk::pipeline_props;

		static
			void recompile_fragment_program(const RSXFragmentProgram& RSXFP, fragment_program_type& fragmentProgramData, usz ID)
		{
			fragmentProgramData.Decompile(RSXFP);
			fragmentProgramData.id = static_cast<u32>(ID);
#ifdef RPCS3_UWP
			if (g_uwp_deferred_shader_compile)
				return; // compiled by build_pipeline's compiler thread
#endif
			fragmentProgramData.Compile();
		}

		static
			void recompile_vertex_program(const RSXVertexProgram& RSXVP, vertex_program_type& vertexProgramData, usz ID)
		{
			vertexProgramData.Decompile(RSXVP);
			vertexProgramData.id = static_cast<u32>(ID);
#ifdef RPCS3_UWP
			if (g_uwp_deferred_shader_compile)
				return;
#endif
			vertexProgramData.Compile();
		}

		static
			void validate_pipeline_properties(const VKVertexProgram&, const VKFragmentProgram& fp, vk::pipeline_props& properties)
		{
			//Explicitly disable writing to undefined registers
			properties.state.att_state[0].colorWriteMask &= fp.output_color_masks[0];
			properties.state.att_state[1].colorWriteMask &= fp.output_color_masks[1];
			properties.state.att_state[2].colorWriteMask &= fp.output_color_masks[2];
			properties.state.att_state[3].colorWriteMask &= fp.output_color_masks[3];
		}

		static
			pipeline_type* build_pipeline(
				const vertex_program_type& vertexProgramData,
				const fragment_program_type& fragmentProgramData,
				const vk::pipeline_props& pipelineProperties,
				bool compile_async,
				std::function<pipeline_type*(pipeline_storage_type&)> callback)
		{
			vk::pipe_compiler::op_flags compiler_flags = compile_async ? vk::pipe_compiler::COMPILE_DEFERRED : vk::pipe_compiler::COMPILE_INLINE;
			compiler_flags |= vk::pipe_compiler::SEPARATE_SHADER_OBJECTS;

			if (vertexProgramData.Flags() & RSX_SHADER_CONTROL_FLAT_SHADING)
			{
				compiler_flags |= vk::pipe_compiler::USE_LAST_PROVOKING_VERTEX;
			}

			auto compiler = vk::get_pipe_compiler();
#ifdef RPCS3_UWP
			if (!vertexProgramData.handle || !fragmentProgramData.handle)
			{
				// Deferred SPIR-V compile; the program objects live in the program cache.
				auto result = compiler->compile(
					pipelineProperties,
					&const_cast<vertex_program_type&>(vertexProgramData).shader,
					&const_cast<fragment_program_type&>(fragmentProgramData).shader,
					compiler_flags, callback,
					vertexProgramData.uniforms,
					fragmentProgramData.uniforms);

				return callback(result);
			}
#endif
			auto result = compiler->compile(
				pipelineProperties,
				vertexProgramData.handle,
				fragmentProgramData.handle,
				compiler_flags, callback,
				vertexProgramData.uniforms,
				fragmentProgramData.uniforms);

			return callback(result);
		}
	};

	struct program_cache : public program_state_cache<VKTraits>
	{
		program_cache(decompiler_callback_t callback)
		{
			notify_pipeline_compiled = callback;
		}

		u64 get_hash(const vk::pipeline_props& props)
		{
			return rpcs3::hash_struct<vk::pipeline_props>(props);
		}

		u64 get_hash(const RSXVertexProgram& prog)
		{
			return program_hash_util::vertex_program_utils::get_vertex_program_ucode_hash(prog);
		}

		u64 get_hash(const RSXFragmentProgram& prog)
		{
			return program_hash_util::fragment_program_utils::get_fragment_program_ucode_hash(prog);
		}

		template <typename... Args>
		void add_pipeline_entry(RSXVertexProgram& vp, RSXFragmentProgram& fp, vk::pipeline_props& props, Args&& ...args)
		{
			get_graphics_pipeline(nullptr, vp, fp, props, false, false, std::forward<Args>(args)...);
		}

		void preload_programs(rsx::program_cache_hint_t* cache_hint, const RSXVertexProgram& vp, const RSXFragmentProgram& fp)
		{
			search_vertex_program(cache_hint, vp);
			search_fragment_program(cache_hint, fp);
		}

		bool check_cache_missed() const
		{
			return m_cache_miss_flag;
		}
	};
}
