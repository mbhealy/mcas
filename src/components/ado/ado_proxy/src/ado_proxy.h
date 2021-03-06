/*
  Copyright [2017-2020] [IBM Corporation]
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef __ADOPROX_COMPONENT_H__
#define __ADOPROX_COMPONENT_H__

#include "ado_proto.h"
#include "docker.h"
#include <api/ado_itf.h>
#include <api/kvstore_itf.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <csignal> /* non-docker only */
#include <memory>
#include <set>

#define PROX_TO_ADO 1
#define RECV_EXIT 2
#define RECV_COMPLETE 3

namespace std
{
template <>
struct default_delete<DOCKER>
{
  void operator()(DOCKER *d);
};
}

class ADO_proxy : public Component::IADO_proxy {
public:
  static constexpr size_t MAX_ALLOWED_DEFERRED_LOCKS = 32;
  
  ADO_proxy(const uint64_t auth_id,
            Component::IKVStore * kvs,
            Component::IKVStore::pool_t pool_id,
            const std::string &pool_name,
            const size_t pool_size,
            const unsigned int pool_flags,
            const uint64_t expected_obj_count,
            const std::string &filename,
            std::vector<std::string> &args,
            std::string cores,
            int memory,
            float core_number,
            numa_node_t numa_zone);

  ADO_proxy(const ADO_proxy &) = delete;
  ADO_proxy& operator=(const ADO_proxy &) = delete;

  virtual ~ADO_proxy();

  DECLARE_VERSION(0.1f);
  DECLARE_COMPONENT_UUID(0x8a120985, 0x1253, 0x404d, 0x94d7, 0x77, 0x92, 0x75,
                         0x21, 0xa1, 0x92); //

  void *query_interface(Component::uuid_t &itf_uuid) override {
    if (itf_uuid == Component::IADO_proxy::iid()) {
      return static_cast<Component::IADO_proxy *>(this);
    } else
      return NULL; // we don't support this interface
  }

  void unload() override {
    //    kill();
    delete this;
  }

  status_t bootstrap_ado(bool opened_existing) override;

  status_t send_op_event(Component::ADO_op op) override;

  status_t send_memory_map(uint64_t token, size_t size,
                           void *value_vaddr) override;

  status_t send_work_request(const uint64_t work_request_key,
                             const char * key,
                             const size_t key_len,
                             const void * value_addr,
                             const size_t value_len,
                             const void * detached_value,
                             const size_t detached_value_len,
                             const void * invocation_data,
                             const size_t invocation_data_len,
                             const bool new_root) override;


  bool check_work_completions(uint64_t& request_key,
                              status_t& out_status,
                              Component::IADO_plugin::response_buffer_vector_t& response_buffers) override;

  status_t recv_callback_buffer(Buffer_header *& out_buffer) override;

  void free_callback_buffer(void * buffer) override;

  bool check_table_ops(const void * buffer,
                       uint64_t &work_request_id,
                       Component::ADO_op &op,
                       std::string &key,
                       size_t &value_len,
                       size_t &value_alignment,
                       void *& addr) override;

  bool check_index_ops(const void * buffer,
                       std::string& key_expression,
                       offset_t& begin_pos,
                       int& find_type,
                       uint32_t max_comp) override;

  bool check_vector_ops(const void * buffer,
                        epoch_time_t& t_begin,
                        epoch_time_t& t_end) override;

  bool check_pool_info_op(const void * buffer) override;

  bool check_iterate(const void * buffer,
                     epoch_time_t& t_begin,
                     epoch_time_t& t_end,
                     Component::IKVStore::pool_iterator_t& iterator) override;

  bool check_op_event_response(const void * buffer,
                               Component::ADO_op& op) override;

  bool check_unlock_request(const void * buffer,
                            uint64_t& work_id,
                            Component::IKVStore::key_t& key_handle) override;

  
  void send_table_op_response(const status_t s,
                              const void * value_addr = nullptr,
                              size_t value_len = 0,
                              const char * key_ptr = nullptr,
                              Component::IKVStore::key_t out_key_handle = nullptr) override;

  void send_find_index_response(const status_t status,
                                const offset_t matched_position,
                                const std::string& matched_key) override;

  void send_vector_response(const status_t status,
                            const Component::IADO_plugin::Reference_vector& rv) override;

  void send_iterate_response(const status_t rc,
                             const Component::IKVStore::pool_iterator_t iterator,
                             const Component::IKVStore::pool_reference_t reference) override;

  void send_pool_info_response(const status_t status,
                               const std::string& info) override;

  void send_unlock_response(const status_t status) override;

  bool has_exited() override;

  status_t shutdown() override;

  void add_deferred_unlock(const uint64_t work_key,
                           const Component::IKVStore::key_t key) override;

  status_t update_deferred_unlock(const uint64_t work_request_id,
                                  const Component::IKVStore::key_t key) override;

  void get_deferred_unlocks(const uint64_t work_key,
                            std::vector<Component::IKVStore::key_t> &keys) override;

  bool check_for_implicit_unlock(const uint64_t work_key,
                                 const Component::IKVStore::key_t key) override;

  void add_life_unlock(const Component::IKVStore::key_t key) override;

  status_t remove_life_unlock(const Component::IKVStore::key_t key) override;
  
  void release_life_locks() override;


  std::string ado_id() const override { return _container_id; }

  const std::string& pool_name() const override { return _pool_name; }

  Component::IKVStore::pool_t pool_id() const override { return _pool_id; }

private:
  status_t kill();
  void launch();

  uint64_t                              _auth_id;
  Component::IKVStore*                  _kvs;
  Component::IKVStore::pool_t           _pool_id;
  const std::string                     _pool_name;
  const size_t                          _pool_size;
  const unsigned int                    _pool_flags;
  const uint64_t                        _expected_obj_count;
  std::string                           _cores;
  float                                 _core_number;
  std::string                           _filename;
  std::vector<std::string>              _args;
  std::string                           _channel_name;
  std::unique_ptr<ADO_protocol_builder> _ipc;
  int                                   _memory;
  numa_node_t                           _numa;
  std::map<uint64_t, std::set<Component::IKVStore::key_t>> _deferred_unlocks;
  std::set<Component::IKVStore::key_t>  _life_unlocks;
  unsigned                              _outstanding_wr = 0;
  std::string                           _container_id;
  pid_t                                 _pid; // Non-docker only
  
  static void child_exit(int, siginfo_t *, void *);
  static sig_atomic_t _exited; // Non-docker only

  class docker_destroyer {
  public:
    void operator()(DOCKER *d) { docker_destroy(d); }
  };
  std::unique_ptr<DOCKER> docker; // Docker only
};

class ADO_proxy_factory : public Component::IADO_proxy_factory {
public:
  DECLARE_VERSION(0.1f);
  DECLARE_COMPONENT_UUID(0xfac20985, 0x1253, 0x404d, 0x94d7, 0x77, 0x92, 0x75,
                         0x21, 0xa1, 0x92); //

  void *query_interface(Component::uuid_t &itf_uuid) override {
    if (itf_uuid == Component::IADO_proxy_factory::iid()) {
      return static_cast<Component::IADO_proxy_factory *>(this);
    } else
      return NULL; // we don't support this interface
  }

  void unload() override { delete this; }

  virtual Component::IADO_proxy *
  create(const uint64_t auth_id,
         Component::IKVStore * kvs,
         Component::IKVStore::pool_t pool_id,
         const std::string &pool_name,
         const size_t pool_size,
         const unsigned int pool_flags,
         const uint64_t expected_obj_count,
         const std::string &filename,
         std::vector<std::string> &args,
         std::string cores,
         int memory,
         float cpu_num,
         numa_node_t numa_zone) override
  {
    Component::IADO_proxy *obj =
      static_cast<Component::IADO_proxy *>(new ADO_proxy(auth_id,kvs,
                                                         pool_id,
                                                         pool_name,
                                                         pool_size,
                                                         pool_flags,
                                                         expected_obj_count,
                                                         filename,
                                                         args,
                                                         cores,
                                                         memory,
                                                         cpu_num,
                                                         numa_zone));
    assert(obj);
    obj->add_ref();
    return obj;
  }
};

#endif
