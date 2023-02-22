#include "sys.h"
#include "PipelineCache.h"
#include "PipelineFactory.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "SynchronousWindow.h"
#include "utils/nearest_multiple_of_power_of_two.h"
#include "utils/u8string_to_filename.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <boost/serialization/binary_object.hpp>
#include <fstream>

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

namespace vulkan::task {

PipelineCache::PipelineCache(PipelineFactory* factory COMMA_CWDEBUG_ONLY(bool debug)) : direct_base_type(CWDEBUG_ONLY(debug)), m_owning_factory(factory)
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineCache(" << factory << ") [" << this << "]");
  // We depend on the owning window, but should not be aborted at program termination.
  // Note: increment() can, theoretically, throw -- but that should never happen in
  // this case: a PipelineCache is created in PipelineFactory_start, but all PipelineFactory
  // tasks are aborted before the owning window enters m_task_counter_gate.wait().
  m_owning_factory->owning_window()->m_task_counter_gate.increment();
}

PipelineCache::~PipelineCache()
{
  DoutEntering(dc::statefultask(mSMDebug), "~PipelineCache() [" << this << "]");
  m_owning_factory->owning_window()->m_task_counter_gate.decrement();
}

char const* PipelineCache::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(condition_flush_to_disk);
    AI_CASE_RETURN(factory_finished);
  }
  return direct_base_type::condition_str_impl(condition);
}

char const* PipelineCache::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    // A complete listing of my_task_state_type.
    AI_CASE_RETURN(PipelineCache_initialize);
    AI_CASE_RETURN(PipelineCache_load_from_disk);
    AI_CASE_RETURN(PipelineCache_ready);
    AI_CASE_RETURN(PipelineCache_factory_finished);
    AI_CASE_RETURN(PipelineCache_factory_merge);
    AI_CASE_RETURN(PipelineCache_save_to_disk);
    AI_CASE_RETURN(PipelineCache_done);
  }
  AI_NEVER_REACHED
}

char const* PipelineCache::task_name_impl() const
{
  return "PipelineCache";
}

std::filesystem::path PipelineCache::get_filename() const
{
  return Application::instance().path_of(Directory::cache) / utils::u8string_to_filename(u8"pipeline_cache of " + m_owning_factory->owning_window()->pipeline_cache_name());
}

void PipelineCache::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case PipelineCache_initialize:
    {
      if (!std::filesystem::exists(get_filename()))
      {
        vulkan::LogicalDevice const* logical_device(m_owning_factory->owning_window()->logical_device());
        vk::PipelineCacheCreateInfo pipeline_cache_create_info = {
          .flags = logical_device->supports_cache_control() ? vk::PipelineCacheCreateFlagBits::eExternallySynchronized : vk::PipelineCacheCreateFlagBits{0},
          .initialDataSize = 0
        };
        m_pipeline_cache = logical_device->create_pipeline_cache(pipeline_cache_create_info
            COMMA_CWDEBUG_ONLY({"PipelineCache_initialize::m_pipeline_cache", as_postfix(this)}));
        set_state(PipelineCache_ready);
        break;
      }
      set_state(PipelineCache_load_from_disk);
      Dout(dc::statefultask(mSMDebug), "Falling through to PipelineCache_load_from_disk [" << this << "]");
      [[fallthrough]];
    }
    case PipelineCache_load_from_disk:
    {
      std::ifstream file(get_filename());
      ASSERT(file);
      try
      {
        boost::archive::binary_iarchive ia(file);
        //Dout(dc::always|flush_cf, "CALLING ia & *this");
        ia & *this;
      }
      catch (boost::archive::archive_exception const& error)
      {
        Dout(dc::warning, "Caught boost::archive::archive_exception \"" << error.what() << "\" while trying to load " << get_filename() << ". Removing.");
        std::error_code ec;
        int exists = std::filesystem::remove(get_filename(), ec);
        if (ec)
          THROW_ALERTC(ec, "Failed to load (invalid?) pipeline cache file [FILENAME] ([ARERR]) and then failed to remove that file! Please remove it yourself",
              AIArgs("[FILENAME]", get_filename())("[ARERR]", error.what()));
        // Paranoia check: we should never get in state PipelineCache_load_from_disk when it doesn't exist?!
        ASSERT(exists);
        yield();        // Must yield to avoid an assert because PipelineCache_initialize did fallthrough to this state.
        set_state(PipelineCache_initialize);
        break;
      }
      set_state(PipelineCache_ready);
      Dout(dc::statefultask(mSMDebug), "Falling through to PipelineCache_ready [" << this << "]");
      [[fallthrough]];
    }
    case PipelineCache_ready:
      // Paranoia check: ready means we have a pipeline cache.
      ASSERT(m_pipeline_cache);
      m_owning_factory->signal(PipelineFactory::pipeline_cache_set_up);
      set_state(PipelineCache_factory_finished);
      wait(factory_finished);
      break;
    case PipelineCache_factory_finished:
      if (!m_is_merger)
      {
        set_state(PipelineCache_done);
        break;
      }
      set_state(PipelineCache_factory_merge);
      Dout(dc::statefultask(mSMDebug), "Falling through to PipelineCache_merge [" << this << "]");
      [[fallthrough]];
    case PipelineCache_factory_merge:
      {
        std::vector<vk::UniquePipelineCache> srcs;
        std::vector<vk::PipelineCache> vhv_srcs;
        // Copy over all pipeline caches. Also preserve the unique handle to keep the pipeline cache alive
        // until the end of scope (after which it was merged).
        flush_new_data([&](vk::UniquePipelineCache&& pipeline_cache){
          vhv_srcs.push_back(*pipeline_cache);
          srcs.push_back(std::move(pipeline_cache));
        });
        if (!vhv_srcs.empty())
        {
          Dout(dc::vulkan, "Merging " << vhv_srcs << " into " << *m_pipeline_cache);
          vulkan::LogicalDevice const* logical_device(m_owning_factory->owning_window()->logical_device());
          logical_device->merge_pipeline_caches(*m_pipeline_cache, vhv_srcs);
        }
      }
      if (producer_not_finished())
        break;
      set_state(PipelineCache_save_to_disk);
      Dout(dc::statefultask(mSMDebug), "Falling through to PipelineCache_save_to_disk [" << this << "]");
      [[fallthrough]];
    case PipelineCache_save_to_disk:
    {
      if (m_pipeline_cache)     // Should always be true - but see ASSERT in PipelineCache::save.
      {
        std::ofstream file(get_filename());
        boost::archive::binary_oarchive oa(file);
        oa & *this;
      }
      else
        Dout(dc::warning, "Not saving pipeline cache because m_pipeline_cache is nul?!");
      set_state(PipelineCache_done);
      Dout(dc::statefultask(mSMDebug), "Falling through to PipelineCache_done [" << this << "]");
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
  DoutEntering(dc::vulkan, "PipelineCache::load(archive, " << version << ") [" << this << "]");
  size_t size;
  archive & size;
  void* tmp_storage = std::aligned_alloc(
      alignof(vk::PipelineCacheHeaderVersionOne),
      utils::nearest_multiple_of_power_of_two(size, alignof(vk::PipelineCacheHeaderVersionOne)));
  archive & make_nvp("pipeline_cache", boost::serialization::make_binary_object(tmp_storage, size));
  vk::PipelineCacheHeaderVersionOne const* header = static_cast<vk::PipelineCacheHeaderVersionOne const*>(tmp_storage);
  Dout(dc::vulkan, "Read " << size << " bytes from pipeline cache, with header: " << *header);
  vulkan::LogicalDevice const* logical_device(m_owning_factory->owning_window()->logical_device());
  vk::PipelineCacheCreateInfo pipeline_cache_create_info = {
    .flags = logical_device->supports_cache_control() ? vk::PipelineCacheCreateFlagBits::eExternallySynchronized : vk::PipelineCacheCreateFlagBits{0},
    .initialDataSize = size,
    .pInitialData = tmp_storage
  };
  m_pipeline_cache = logical_device->create_pipeline_cache(pipeline_cache_create_info
      COMMA_CWDEBUG_ONLY({"PipelineCache::m_pipeline_cache", as_postfix(this)}));
  free(tmp_storage);
}

void PipelineCache::save(boost::archive::binary_oarchive& archive, unsigned int const version) const
{
  DoutEntering(dc::vulkan, "PipelineCache::save(archive, " << version << ") [" << this << "]");
  size_t size;
  vk::PipelineCache vh_pipeline_cache = *m_pipeline_cache;
  // Don't call `oa & *this` (state PipelineCache_save_to_disk) when we don't have a handle:
  // that would truncate the cache file since we can't write to it and worse, make it unloadable.
  // This assert is put before the throw because it is a program error and should be fixed before a Release.
  ASSERT(vh_pipeline_cache);
  // However - a release doesn't have asserts. In this case I want to do something better than just crash
  // below; if this inadvertently would still happen.
  if (!vh_pipeline_cache)
    THROW_FALERT("The pipeline cache handle is nul.");
  vulkan::LogicalDevice const* logical_device(m_owning_factory->owning_window()->logical_device());
  size = logical_device->get_pipeline_cache_size(vh_pipeline_cache);
  void* tmp_storage = std::aligned_alloc(
      alignof(vk::PipelineCacheHeaderVersionOne),
      utils::nearest_multiple_of_power_of_two(size, alignof(vk::PipelineCacheHeaderVersionOne)));
  logical_device->get_pipeline_cache_data(vh_pipeline_cache, size, tmp_storage);
  archive & size;
  archive & make_nvp("pipeline_cache", boost::serialization::make_binary_object(tmp_storage, size));
  vk::PipelineCacheHeaderVersionOne const* header = static_cast<vk::PipelineCacheHeaderVersionOne const*>(tmp_storage);
  Dout(dc::vulkan, "Wrote " << size << " bytes to pipeline cache, with header: " << *header);
  free(tmp_storage);
}

} // namespace vulkan::task
