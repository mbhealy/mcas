#include "ramrbtree.h"
#include <stdlib.h>
#include <regex>
#include <set>

#define SINGLE_THREADED

using namespace Component;
using namespace std;

RamRBTree::RamRBTree(const std::string& owner, const std::string& name)
	: RamRBTree{}
{
	(void)owner; // unused
	(void)name; // unused;
}

RamRBTree::RamRBTree() : _index{} {}

RamRBTree::~RamRBTree() {}

void RamRBTree::insert(const string& key)
{
  // if (!_index.insert(key).second) {
  //   throw(API_exception("insert index failed"));
  // }
  _index.insert(key);
}

void RamRBTree::erase(const std::string& key) { _index.erase(key); }

void RamRBTree::clear() { _index.clear(); }

string RamRBTree::get(offset_t position) const
{
  if (position >= _index.size()) {
    throw out_of_range("Position out of range");
  }

  auto it = _index.begin();
  advance(it, position);
  return *it;
}

size_t RamRBTree::count() const { return _index.size(); }

status_t RamRBTree::find(const std::string& key_expression,
                         offset_t           begin_position,
                         find_t             find_type,
                         offset_t&          out_matched_pos,
                         std::string&       out_matched_key,
                         unsigned           max_comparisons)
{
  if (begin_position >= _index.size()) {
    return E_FAIL;
  }

  offset_t end_position = _index.size();
  unsigned attempts =0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch" // enumeration value ‘FIND_TYPE_NONE’ not handled in switch
  switch (find_type) {
    case FIND_TYPE_REGEX:
      {
        std::regex r(key_expression);
        for (out_matched_pos = begin_position; out_matched_pos <= end_position; out_matched_pos++) {
          string key = RamRBTree::get(out_matched_pos);
          if (regex_match(key, r)) {
            out_matched_key = key;
            return S_OK;
          }
          else {
            if(++attempts > max_comparisons)
              return E_MAX_REACHED;
          }
        }
      }
      break;
    case FIND_TYPE_EXACT:
      for (out_matched_pos = begin_position; out_matched_pos <= end_position; out_matched_pos++) {
        string key = RamRBTree::get(out_matched_pos);
        if (key.compare(key_expression) == 0) {
          out_matched_key = key;
          return S_OK;
        }
        else {
          if(++attempts > max_comparisons)
            return E_MAX_REACHED;
        }
      }
      break;
    case FIND_TYPE_PREFIX:
      for (out_matched_pos = begin_position; out_matched_pos <= end_position; out_matched_pos++) {
        string key = RamRBTree::get(out_matched_pos);
        if (key.find(key_expression) != string::npos) {
          out_matched_key = key;
          return S_OK;
        }
      }
      break;
    case FIND_TYPE_NEXT:
      if(begin_position >= end_position)
        return E_FAIL;
      out_matched_key = get(begin_position);
      out_matched_pos = begin_position;
      return S_OK;
      break;
  }
#pragma GCC diagnostic pop

  return E_FAIL;
}

/**
 * Factory entry point
 *
 */
extern "C" void* factory_createInstance(Component::uuid_t component_id)
{
  if (component_id == RamRBTree_factory::component_id()) {
    return static_cast<void*>(new RamRBTree_factory());
  }
  else
    return NULL;
}
