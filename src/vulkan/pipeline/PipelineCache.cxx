#include "sys.h"
#include "PipelineCache.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "utils/nearest_multiple_of_power_of_two.h"
#include "utils/ulong_to_base.h"
#include "debug.h"
#include <boost/serialization/binary_object.hpp>

#ifdef CWDEBUG
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>

namespace vk {

std::ostream& operator<<(std::ostream& os, vk::PipelineCacheHeaderVersionOne const& header)
{
  os << '{';
  os << "headerSize:" << header.headerSize <<
      ", headerVersion:" << to_string(header.headerVersion) <<
      ", vendorID:" << header.vendorID <<
      ", deviceID:" << header.deviceID;
  boost::uuids::uuid uuid;
  std::copy(header.pipelineCacheUUID.begin(), header.pipelineCacheUUID.end(), uuid.begin());
  os << ", pipelineCacheUUID:" << uuid;
  return os << '}';
}

} // namespace vk
#endif

namespace task {

char const* PipelineCache::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    // A complete listing of my_task_state_type.
    AI_CASE_RETURN(PipelineCache_initialize);
    AI_CASE_RETURN(PipelineCache_load_from_disk);
    AI_CASE_RETURN(PipelineCache_ready);
    AI_CASE_RETURN(PipelineCache_save_to_disk);
    AI_CASE_RETURN(PipelineCache_done);
  }
  AI_NEVER_REACHED
}

std::filesystem::path PipelineCache::get_filename() const
{
  return vulkan::Application::instance().path_of(vulkan::Directory::cache) / to_string(m_logical_device->get_UUID()) / "pipeline_cache";
}

void PipelineCache::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case PipelineCache_initialize:
    {
      if (!std::filesystem::exists(get_filename()))
      {
        set_state(PipelineCache_ready);
        break;
      }
      set_state(PipelineCache_load_from_disk);
      [[fallthrough]];
    }
    case PipelineCache_load_from_disk:
    {
      std::ifstream file(get_filename());
      Debug(m_load_ambifix = vulkan::Ambifix("PipelineCache", "[" + utils::ulong_to_base(reinterpret_cast<uint64_t>(this), "0123456789abcdef") + "]"));
      boost::archive::binary_iarchive ia(file);
      ia & *this;
      set_state(PipelineCache_ready);
      [[fallthrough]];
    }
    case PipelineCache_ready:
      finish();
      break;
#if 0
      set_state(PipelineCache_save_to_disk);
      wait(condition_flush_to_disk);
      break;
#endif
    case PipelineCache_save_to_disk:
    {
      std::ofstream file(get_filename());
      boost::archive::binary_oarchive oa(file);
      oa & *this;
      set_state(PipelineCache_done);
      [[fallthrough]];
    }
    case PipelineCache_done:
      clear_cache();
      finish();
      break;
  }
}

void PipelineCache::clear_cache()
{
  DoutEntering(dc::vulkan, "PipelineCache::clear_cache() [" << this << "]");
  m_pipeline_cache.reset();
}

void PipelineCache::load(boost::archive::binary_iarchive& archive, unsigned int const version)
{
  size_t size;
  archive & size;
  void* tmp_storage = std::aligned_alloc(
      alignof(vk::PipelineCacheHeaderVersionOne),
      utils::nearest_multiple_of_power_of_two(size, alignof(vk::PipelineCacheHeaderVersionOne)));
  archive & make_nvp("pipeline_cache", boost::serialization::make_binary_object(tmp_storage, size));
  vk::PipelineCacheHeaderVersionOne const* header = static_cast<vk::PipelineCacheHeaderVersionOne const*>(tmp_storage);
  Dout(dc::vulkan, "Read " << size << " bytes from pipeline cache, with header: " << *header);
  vk::PipelineCacheCreateInfo pipeline_cache_create_info = {
    .flags = m_logical_device->supports_cache_control() ? vk::PipelineCacheCreateFlagBits::eExternallySynchronized : vk::PipelineCacheCreateFlagBits{0},
    .initialDataSize = size,
    .pInitialData = tmp_storage
  };
  m_pipeline_cache = m_logical_device->create_pipeline_cache(pipeline_cache_create_info
      COMMA_CWDEBUG_ONLY(".m_pipeline_cache" + m_load_ambifix));
  free(tmp_storage);
}

void PipelineCache::save(boost::archive::binary_oarchive& archive, unsigned int const version) const
{
  size_t size;
  vk::PipelineCache pipeline_cache = *m_pipeline_cache;
  // This can happen when the cache was cleared with clear_cache().
  if (!pipeline_cache)
    return;
  size = m_logical_device->get_pipeline_cache_size(pipeline_cache);
  void* tmp_storage = std::aligned_alloc(
      alignof(vk::PipelineCacheHeaderVersionOne),
      utils::nearest_multiple_of_power_of_two(size, alignof(vk::PipelineCacheHeaderVersionOne)));
  m_logical_device->get_pipeline_cache_data(pipeline_cache, size, tmp_storage);
  archive & size;
  archive & make_nvp("pipeline_cache", boost::serialization::make_binary_object(tmp_storage, size));
  vk::PipelineCacheHeaderVersionOne const* header = static_cast<vk::PipelineCacheHeaderVersionOne const*>(tmp_storage);
  Dout(dc::vulkan, "Wrote " << size << " bytes to pipeline cache, with header: " << *header);
  free(tmp_storage);
}

} // namespace task
