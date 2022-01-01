#include "sys.h"
#include "get_binary_file_contents.h"
#include "utils/AIAlert.h"
#include <fstream>
#include <vector>
#include "debug.h"

namespace vk_utils {

// Function reading binary contents of a file.
std::vector<std::byte> get_binary_file_contents(std::filesystem::path const& filename)
{
  std::ifstream ifs(filename, std::ios::binary | std::ios::ate);

  if (ifs.fail())
    THROW_ALERT("Could not open [FILENAME] file!", AIArgs("[FILENAME]", filename));

  std::streampos const file_size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  std::vector<std::byte> result(file_size);
  ifs.read(reinterpret_cast<char*>(result.data()), file_size);

  if (!ifs)
    THROW_ALERT("Reading [SIZE] bytes from [FILENAME] failed!", AIArgs("[SIZE]", file_size)("[FILENAME]", filename));

  ASSERT(ifs.gcount() == file_size);
  ASSERT(ifs.good());

  return result;
}

} // namespace vk_utils
