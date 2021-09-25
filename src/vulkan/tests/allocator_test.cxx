#include "sys.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "../DequeAllocator.h"
#include "utils/NodeMemoryPool.h"
#include <deque>
#include <list>

struct Foo
{
  char data[37] = {};
  Foo() = default;
  Foo(int x) { ASSERT(x == 42); }
};

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  utils::NodeMemoryPool pool(32);
  utils::Allocator<Foo, utils::NodeMemoryPool> alloc(pool);
  std::shared_ptr<Foo> ptr = std::allocate_shared<Foo>(alloc, 42);
  std::list<Foo, decltype(alloc)> v(3, alloc);

  for (int i = 0; i < 100; ++i)
    v.emplace_back(42);

  for (int i = 0; i < 100; ++i)
    v.pop_back();

  AIMemoryPagePool mpp;
  utils::NodeMemoryResource nmr(AIMemoryPagePool::instance());
  DequeAllocator<Foo> alloc2(nmr);
  std::deque<Foo, decltype(alloc2)> d(3, alloc2);

  for (int i = 0; i < 100; ++i)
    d.emplace_back(42);

  for (int i = 0; i < 100; ++i)
    d.pop_back();

  Dout(dc::notice, "Leaving main()");
}
