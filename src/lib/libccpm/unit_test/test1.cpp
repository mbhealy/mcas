/* note: we do not include component source, only the API definition */
#include <common/cycles.h>
#include <common/rand.h>
#include <common/utils.h>
#include <gtest/gtest.h>

#include <ccpm/slab.h>
#include <ccpm/immutable_allocator.h>
#include <ccpm/immutable_string_table.h>
#include <ccpm/record.h>

//#define GPERF_TOOLS

#ifdef GPERF_TOOLS
#include <gperftools/profiler.h>
#endif

struct {
  uint64_t uuid;
} Options;

// The fixture for testing class Foo.
class Libccpm_test : public ::testing::Test {
 protected:
  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp()
  {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  virtual void TearDown()
  {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case
};

#if 0
TEST_F(Libccpm_test, ccpm_slab)
{
  auto ptr = aligned_alloc(4096,4096);
  ccpm::Slab<uint32_t> slab(ptr, 4096);
  slab.check();
}
#endif

#if 0
TEST_F(Libccpm_test, ccpm_immutable_allocator)
{
  size_t size = 4096;
  auto ptr = aligned_alloc(4096,size);
  ccpm::Immutable_allocator_base alloc(ptr, size);
}

TEST_F(Libccpm_test, ccpm_cowptr)
{
  using namespace ccpm;

  size_t size = 4096;
  auto ptr = aligned_alloc(4096,size);
  
  {
    Cow_value_pointer<byte> vr(ptr, size, TYPE_UNKNOWN, true);
  }
  PLOG("Cow Ptr Again:");
  {
    Cow_value_pointer<byte> vr(ptr, size, TYPE_UNKNOWN);
  }
  PLOG("ccpm_cowptr OK!");
}
#endif

// TEST_F(Libccpm_test, ccpm_record)
// {
//   using namespace ccpm;

//   size_t size = 4096;
//   auto ptr = aligned_alloc(4096,size);
  
//   {
//     Versioned_record<Immutable_allocator_base> vr(ptr, size, TYPE_UNKNOWN);
//   }
//   PLOG("Again:");
//   {
//     Versioned_record<Immutable_allocator_base> vr(ptr, size, TYPE_UNKNOWN);
//   }  
// }

#if 0
TEST_F(Libccpm_test, ccpm_immutable_string_table)
{
  size_t size = MB(4);
  auto ptr = aligned_alloc(64,size);
  void * p;
  {
    ccpm::Immutable_string_table<> st(ptr, size);    
    p = st.add_string("Hello!");
    std::cout << "Read back: (" << st.read_string(p) << ")" << std::endl;
  }

  {
    ccpm::Immutable_string_table<> st(ptr, size);    
    std::cout << "Reconstituted read back: (" << st.read_string(p) << ")" << std::endl;
  }
}
#endif

#include <ccpm/overlay_base.h>
#include <common/dump_utils.h>

TEST_F(Libccpm_test, ccpm_overlay)
{
  using namespace ccpm;

  DECLARE_OVERLAY_CHAIN(Overlay, Version, Timestamps);
  
  //  using Overlay = Opaque_value<Version<Timestamps<>>>;
  auto obj = new (malloc(sizeof(Overlay))) Overlay;

  hexdump(obj, sizeof(Overlay));
  delete obj;
}

TEST_F(Libccpm_test, ccpm_immutable_allocator)
{
  using namespace ccpm;

  size_t region_size = 0x400;
  region_vector_t regions;
  regions.push_back(::iovec{aligned_alloc(8,region_size), region_size});
  regions.push_back(::iovec{aligned_alloc(8,region_size), region_size});
  ASSERT_TRUE(regions.size() == 2);

  Immutable_allocator_base allocator(regions, nullptr, true);
  size_t inc = 0x100;
  void * ptr;
  for(unsigned i=0;i<6;i++) {
    PLOG("allocate: %p,0x%lx", ptr = allocator.allocate(inc), inc);
    //allocator.dump_info();
  }

  try {
    allocator.allocate(inc);
    ASSERT_TRUE(false); /* we should have thrown an exception by now */
  }
  catch(...) {
    /* need to add more memory */
    PLOG("ran out of memory - OK - time to add more memmory!");
    allocator.expand(::iovec{aligned_alloc(8,region_size), region_size});
  }
  
  PLOG("allocate: %p,0x%lx", ptr = allocator.allocate(inc), inc);
  PLOG("allocate: %p,0x%lx", ptr = allocator.allocate(inc), inc);
  
  //PLOG("allocate: %p,0x%lx", ptr = allocator.allocate(inc), inc);
  allocator.dump_info();
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  if (argc > 1) {
    Options.uuid = atol(argv[1]);
  }
  auto r = RUN_ALL_TESTS();

  return r;
}
