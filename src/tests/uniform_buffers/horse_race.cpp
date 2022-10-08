#include "sys.h"
#include "debug.h"
#include <iostream>
#include <chrono>
#include <random>
#include <array>
#include <vector>
#include <algorithm>
#include <cassert>
#include <deque>
#include <iomanip>

// Montecarlo test of the "horse race" algorithm.

static constexpr int total_number_of_resources = 8;
static constexpr int number_of_threads = 4;

class Thread;

class Resource
{
 private:
  int m_id;
  std::deque<Thread*> m_owner;

 public:
  Resource() = default;

  void init(int id)
  {
    m_id = id;
    m_owner.clear();
  }

  void print_on(std::ostream& os) const;

  bool acquire(Thread* thr);

  void release(Thread* thr);

  friend std::ostream& operator<<(std::ostream& os, Resource const& resource)
  {
    resource.print_on(os);
    return os;
  }
};

enum StepResult
{
  keep_running,
  success,
  back_off
};

class Thread
{
 private:
  int m_id;
  std::vector<Resource*> m_resources;
  int m_r;
  bool m_running;

 public:
  Thread() = default;

  void init(int id, int number_of_resources, std::array<Resource, total_number_of_resources>& resources, std::mt19937& gen);

  void reset()
  {
    DoutEntering(dc::notice, "Thread::reset()");
    for (int r = 0; r < m_resources.size(); ++r)
      m_resources[r]->release(this);
    m_r = 0;
  }

  StepResult step(int ri);
  void run_again();

  int id() const { return m_id; }

  friend std::ostream& operator<<(std::ostream& os, Thread const& thread)
  {
    os << '{';
    os << "m_id:" << thread.m_id <<
        ", m_resources:<";
    char const* prefix = "";
    for (auto rp : thread.m_resources)
    {
      os << prefix;
      rp->print_on(os);
      prefix = ", ";
    }
    os << ">, m_r:" << thread.m_r <<
        ", m_running:" << std::boolalpha << thread.m_running;
    os << '}';
    return os;
  }
};

std::array<Resource, total_number_of_resources> resources;
std::array<Thread, number_of_threads> threads;
std::array<int, number_of_threads> ri2id;
int number_of_running_threads;

void print_resources()
{
  DoutEntering(dc::notice, "print_resources()");
  for (int r = 0; r < total_number_of_resources; ++r)
    Dout(dc::notice, r << ": " << resources[r]);
}

void print_threads()
{
  DoutEntering(dc::notice, "print_threads()");
  for (int t = 0; t < number_of_threads; ++t)
    Dout(dc::notice, t << ": " << threads[t]);
  for (int ri = 0; ri < number_of_threads; ++ri)
    Dout(dc::notice, "ri2id[" << ri << "] = " << ri2id[ri]);
}

void Resource::release(Thread* thr)
{
  if (m_owner.empty())
    return;

  if (m_owner.front() == thr)
  {
    m_owner.pop_front();
    if (!m_owner.empty())
      m_owner.front()->run_again();
  }
}

bool Resource::acquire(Thread* thr)
{
  DoutEntering(dc::notice, "Resources::acquire(thr:" << thr->id() << ") [resource:" << m_id << "]");
  bool first = m_owner.empty();
  if (!first)
    Dout(dc::notice, "Failing because thread " << m_owner.front()->id() << " already acquired the resource " << m_id << ".");
  else
    Dout(dc::notice, "Success!");
  m_owner.push_back(thr);
  Dout(dc::notice|continued_cf, "m_owner now contains:<");
  char const* prefix = "";
  for (Thread* owner : m_owner)
  {
    Dout(dc::continued, prefix << owner->id());
    prefix = ", ";
  }
  Dout(dc::finish, ">");
  return first;
}

void Resource::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_id:" << m_id <<
      ", m_owner:<";
  char const* prefix = "";
  for (Thread* tp : m_owner)
  {
    os << prefix << tp->id();
    prefix = ", ";
  }
  os << ">}";
}

void Thread::init(int id, int number_of_resources, std::array<Resource, total_number_of_resources>& resources, std::mt19937& gen)
{
  assert(this == &threads[id]);
  m_id = id;
  m_resources.clear();
  m_r = 0;
  m_running = true;

  std::array<int, total_number_of_resources> rn;
  for (int i = 0; i < total_number_of_resources; ++i)
    rn[i] = i;
  std::shuffle(rn.begin(), rn.end(), gen);

  for (int r = 0; r < number_of_resources; ++r)
    m_resources.push_back(&resources[rn[r]]);

  std::sort(m_resources.begin(), m_resources.end());
}

StepResult Thread::step(int ri)
{
  DoutEntering(dc::notice, "step(ri:" << ri << ") [" << m_id << "]");
  assert(m_running);

  if (!m_resources[m_r]->acquire(this))
  {
    Dout(dc::notice, "Setting m_running to false,");
    m_running = false;
    ASSERT(number_of_running_threads > 0);
    --number_of_running_threads;
    Dout(dc::notice, "number_of_running_threads is now " << number_of_running_threads);
    if (ri != number_of_running_threads)
      std::swap(ri2id[ri], ri2id[number_of_running_threads]);
    Dout(dc::notice, "after which:");
    print_threads();
    reset();
    return back_off;
  }

  ++m_r;
  Dout(dc::notice, "Setting m_r to " << m_r << ".");

  if (m_r < m_resources.size())
  {
    Dout(dc::notice, "Returning keep_running because m_r < " << m_resources.size());
    return keep_running;     // Continue running.
  }

  Dout(dc::notice, "Returning success!");
  return success;
}

void Thread::run_again()
{
  DoutEntering(dc::notice, "=====> Thread::run_again() [" << m_id << "]");
  assert(!m_running);
  print_threads();
  Dout(dc::notice, "Looking for id " << m_id << " in ri2id:");
  int ri = number_of_threads - 1;
  for (; ri > 0; --ri)
  {
    Dout(dc::notice, "ri2id[" << ri << "] = " << ri2id[ri]);
    if (ri2id[ri] == m_id)
    {
      Dout(dc::notice, "Swapping ri " << ri << " with " << number_of_running_threads);
      if (ri != number_of_running_threads)
        std::swap(ri2id[ri], ri2id[number_of_running_threads]);
      break;
    }
  }
  ASSERT(ri > 0);
  ++number_of_running_threads;
  Dout(dc::notice, "number_of_running_threads is now " << number_of_running_threads);
  m_running = true;
  Dout(dc::notice, "Setting m_running to true, after which:");
  print_threads();
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");
  Debug(dc::notice.off());

  // --------------------------------------------------------------------------
  // Random number generator stuff.
#if 1
  std::random_device rd;
  std::mt19937::result_type seed = rd() ^ (
          (std::mt19937::result_type)
          std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()
              ).count() +
          (std::mt19937::result_type)
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch()
              ).count() );
  std::cout << "seed = " << seed << std::endl;
#else
  std::mt19937::result_type seed = 1664271846969307;
#endif
  std::mt19937 gen(seed);
  std::uniform_int_distribution<int> number_of_resources_per_thread(2, 6);
  std::uniform_int_distribution<int> random_thread(0, 1000000);
  // --------------------------------------------------------------------------

  int max_steps = 0;
  int steps;
  do
  {
    Dout(dc::notice, "Resetting everything");
    // Initialize each resource with a unique number.
    for (int r = 0; r < total_number_of_resources; ++r)
      resources[r].init(r);
    print_resources();
    for (int t = 0; t < number_of_threads; ++t)
    {
      threads[t].init(t, number_of_resources_per_thread(gen), resources, gen);
      ri2id[t] = t;
    }
    print_threads();
    steps = 1;
    number_of_running_threads = number_of_threads;
    Dout(dc::notice, "number_of_running_threads = " << number_of_running_threads);
    for (;;)
    {
      int ri;
      ri = random_thread(gen) % number_of_running_threads;
      int t = ri2id[ri];
      Dout(dc::notice, "Running thread " << t);
      StepResult sr = threads[t].step(ri);
      if (number_of_running_threads == 0)
      {
        Dout(dc::notice, "Dead lock!");
        return 0;
      }
      if (sr == success)
      {
        Dout(dc::notice, "sr == success; steps = " << steps << "; max_steps = " << max_steps);
        if (steps > max_steps)
        {
          for (int t = 0; t < number_of_threads; ++t)
            std::cout << t << ": " << threads[t] << '\n';
          std::cout << "Thread " << t << " did get a lock on all its resources after " << steps << " steps!" << '\n';
          max_steps = steps;
        }
        break;
      }
      ++steps;
    }
  }
  while (steps < 500);
}
