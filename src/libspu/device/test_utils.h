// Copyright 2021 Ant Group Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <future>

#include "libspu/core/config.h"
#include "libspu/device/io.h"
#include "libspu/device/symbol_table.h"
#include "libspu/spu.h"

namespace spu::device {

// TODO: Drop me.
class LocalIo {
 public:
  LocalIo(size_t world_size, const RuntimeConfig &config)
      : symbol_tables_(world_size),
        io_client_(world_size, makeFullRuntimeConfig(config)) {
    //
  }

  void InFeed(const std::string &name, PtBufferView view, Visibility vtype,
              int owner_rank = -1) {
    auto shares = io_client_.makeShares(view, vtype, owner_rank);
    SPU_ENFORCE(shares.size() == symbol_tables_.size());
    for (size_t idx = 0; idx < symbol_tables_.size(); ++idx) {
      symbol_tables_[idx].setVar(name, shares[idx]);
    }
  }

  NdArrayRef OutFeed(const std::string &name) {
    std::vector<spu::Value> shares;
    shares.reserve(symbol_tables_.size());
    for (auto &st : symbol_tables_) {
      shares.push_back(st.getVar(name));
    }

    auto pt_type = io_client_.getPtType(shares);
    NdArrayRef ret(makePtType(pt_type), shares.front().shape());
    PtBufferView pv(ret.data(), pt_type, ret.shape(), ret.strides());

    io_client_.combineShares(shares, &pv);

    return ret;
  }

  SymbolTable *GetSymbolTable(size_t idx) { return &symbol_tables_[idx]; }

 private:
  std::vector<device::SymbolTable> symbol_tables_;
  IoClient io_client_;
};

}  // namespace spu::device
