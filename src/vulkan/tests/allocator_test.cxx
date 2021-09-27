#include "sys.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "utils/DequeAllocator.h"
#include "utils/NodeMemoryPool.h"
#include "utils/log2.h"
#include <deque>
#include <list>

struct Foo
{
  char data[13] = {};
  int id;
  Foo() = default;
  Foo(int x) : id(x) { }
  void print_on(std::ostream& os) const
  {
    os << id;
  }
};

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

#if 0
  // std::deque allocates tables.
  // The minimum table size is Access::initial_map_size() pointers.
  Dout(dc::notice, "initial_map_size = " << Access::initial_map_size() << " pointers.");
  Dout(dc::notice, "map_size(1000) = " << Access::map_size(8000) << " nodes.");
#endif

#if 0
  utils::NodeMemoryPool pool(32);
  utils::Allocator<Foo, utils::NodeMemoryPool> alloc(pool);
  std::shared_ptr<Foo> ptr = std::allocate_shared<Foo>(alloc, 42);
  std::list<Foo, decltype(alloc)> v(3, alloc);

  for (int i = 0; i < 100; ++i)
    v.emplace_back(42);

  for (int i = 0; i < 100; ++i)
    v.pop_back();
#endif

#if 1
  AIMemoryPagePool mpp;
  utils::DequeMemoryResource::Initialization dmri(mpp.instance());      // For the internal tables of all std::deque<T, utils::Allocator<T>>'s.
  utils::NodeMemoryResource nmr1(AIMemoryPagePool::instance());         // For the fixed size element allocation.
  utils::NodeMemoryResource nmr2(AIMemoryPagePool::instance());         // For the fixed size element allocation.
  utils::DequeAllocator<Foo> alloc1(nmr1);
  utils::DequeAllocator<Foo> alloc2(nmr2);

  std::deque<Foo, decltype(alloc1)> d1(10000, alloc1);
  std::deque<Foo, decltype(alloc1)> d2(1000, 42, alloc1);

  int c = 0;
  for (auto& e : d1)
    e = c++;

  for (int i = 0; i < 2000; ++i)
    d2.emplace_back(i);

#if 1
  for (int i = 0; i < 5000; ++i)
    d1.pop_back();

  for (int x = 0; x < 10; ++x)
  {
    size_t s = 10 * (1 << x) - 2;
    std::deque<Foo, decltype(alloc2)> d2(s, alloc2);
  }
#endif

  Dout(dc::notice|continued_cf, "d1:");
  for (auto& e : d1)
    Dout(dc::continued, e << ", ");
  Dout(dc::finish, "");
  Dout(dc::notice|continued_cf, "d2:");
  for (auto& e : d2)
    Dout(dc::continued, e << ", ");
  Dout(dc::finish, "");

  // Move assignment.
  Dout(dc::notice, "Swap assignment start");
  d2 = std::move(d1);
  Dout(dc::notice, "Swap assignment end");

  Dout(dc::notice|continued_cf, "d1:");
  for (auto& e : d1)
    Dout(dc::continued, e << ", ");
  Dout(dc::finish, "");
  Dout(dc::notice|continued_cf, "d2:");
  for (auto& e : d2)
    Dout(dc::continued, e << ", ");
  Dout(dc::finish, "");
#endif

  Dout(dc::notice, "Leaving main()");
}
